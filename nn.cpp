#include "pch.h"

void THROW_IF_FAILED(HRESULT hr)
{
	if (FAILED(hr)) throw;
}

void PostTrainEnd();

#include "nn.h"
bool TrainCancel = 0;


// Not shuffle for debugging
#ifdef _DEBUG
#define ML_DEBUG_1  // no shuffle
#define ML_DEBUG_2 // 6000 only of the set
//#define CLIP_DEBUG
#endif

//#define SINGLE_BUFFER
#ifdef SINGLE_BUFFER
struct MLBUFFER;
std::shared_ptr<MLBUFFER> global_buffer;
#endif

int CurrentBatch = 16;
int NumEpochsX = 10;
int UseNewDMLLib = 1;

HWND mw();

MLBUFFER debugbuffer;


enum class MODE_TAGS
{
	WEIGHTS = 1,
	BIASES = 2,
	OUTPUT = 3,
	WEIGHTSBP = 4,
	BIASESBP = 5,
	OUTPUTBP = 6,
	WEIGHTSBPOUT = 7,
	BIASESBPOUT = 8,
};

auto ltag = [](size_t i, MODE_TAGS mode) -> LPARAM
	{
		return ((i + 1) * 1000) + (int)mode;
	};




void SaveMatrix(Matrix& x, XML3::XMLElement& e)
{
	e.vv("rows").SetValueInt(x.rows());
	e.vv("cols").SetValueInt(x.cols());
	e.vv("data").SetBinaryValue((char*)x.data().data(), x.rows() * x.cols() * sizeof(float));
}

void LoadMatrix(Matrix& x, XML3::XMLElement& e)
{
	x.init(e.vv("rows").GetValueInt(0), e.vv("cols").GetValueInt(0));
	auto bd = e.vv("data").GetBinaryValue();
	memcpy(x.data().data(), bd.GetD().data(), bd.size());
}



Layer::Layer(double lrate, ACT_TYPE ActivationType, int num_neurons, int num_weights_per_neuron)
{
	lr = lrate;
	ActType = ActivationType;

	weights.init(num_weights_per_neuron, num_neurons); // 128 neurons, each has 784 weights
	if (num_neurons > 0)
		biases.init(1, num_neurons);

	//		weights.randomize();
}


void Layer::Save(XML3::XMLElement& e)
{
	e.vv("lr").SetValueDouble(lr);
	e.vv("ActType").SetValueInt((int)ActType);
	SaveMatrix(weights, e.AddElement("weights"));
	SaveMatrix(biases, e.AddElement("biases"));
}
void Layer::Load(XML3::XMLElement& e)
{
	lr = e.vv("lr").GetValueDouble(0.1);
	ActType = (ACT_TYPE)e.vv("ActType").GetValueInt(0);
	LoadMatrix(weights, *e.FindElementZ("weights", true));
	LoadMatrix(biases, *e.FindElementZ("biases", true));
}


void NN::RecalculateWB()
{
	for (size_t i = 0; i < Layers.size(); i++)
	{
		if (i == 0)
			continue;
		else
		{
			int num_neurons = Layers[i].weights.cols();
			int prev_out = Layers[i].weights.rows();
			if (i > 1)
				prev_out = Layers[i - 1].weights.cols();
			Layers[i].biases.init(1,num_neurons);
			Layers[i].weights.init(prev_out, num_neurons);
		}
	}
}

void NN::Save(XML3::XMLElement& e)
{
	for (auto& layer : Layers)
	{
		auto& l = e.AddElement("Layer");
		layer.Save(l);
	}
	e.vv("rnnh").SetValueInt(RNN_hidden_size);
}

void NN::Load(XML3::XMLElement& e)
{
	Layers.clear();
	for (auto& layer : e)
	{

		auto wr = layer["weights"].vv("rows").GetValueInt(0);
		auto wc = layer["weights"].vv("cols").GetValueInt(0);
		int numneurons = wc;
		Layer l(0.5, ACT_TYPE::SIGMOID, numneurons, wr);
		l.Load(layer);
		Layers.push_back(l);
	}
	RNN_hidden_size = e.vv("rnnh").GetValueInt(32);
}

Matrix outputs_to_matrix(const std::vector<Matrix>& outputs) {
	// Assuming each output in the vector is of size (hidden_size, 1)
	size_t T = outputs.size();                // Number of time steps
	int hidden_size = outputs[0].rows();   // Size of the hidden state

	// Create a matrix of size (T, hidden_size) to store the outputs
	Matrix result;
	result.init((int)T, hidden_size);

	// Copy each output vector into the corresponding row of the result matrix
	for (int t = 0; t < T; t++) {
		result.row(t) = outputs[t].transpose(); // Transpose to make it row-major
	}

	return result;
}

// This will be fed the input matrix of the data set, in which the rows are the time steps and the columns the data.
Matrix NN::ForwardPropagationRNN(Matrix input_sequence /* no ref */ ) {
	// Assuming input_sequence is of size (T, input_size)
	int T = input_sequence.rows();  // Number of time steps
	std::vector<Matrix> outputs;   // Store outputs for each time step

	for (size_t il = 0; il < Layers.size(); il++) {
		Layer& ly = Layers[il];

		// Reinitialize h_prev for each layer
		Matrix h_prev;
		h_prev.init(RNN_hidden_size, 1); // Initialize h_0 to zeros

		// Clear outputs for the next layer if not the first
		if (il > 0) {
			input_sequence = outputs_to_matrix(outputs);
			outputs.clear();
		}

		for (int t = 0; t < T; t++) {
			Matrix x_t = input_sequence.row(t); // Extract input at time step t

			// Compute hidden state
			Matrix add1 = x_t * ly.weights; // Input-to-hidden
			Matrix add2 = h_prev * ly.weights_recurrent_hidden; // Hidden-to-hidden
			Matrix add3 = ly.biases_recurrent; // Biases
			Matrix h_t = add1;
			h_t += add2;
			h_t += add3;

			// Apply activation function
			if (ly.ActType == ACT_TYPE::SIGMOID)
				h_t.sigmoid();
			if (ly.ActType == ACT_TYPE::RELU)
				h_t.relu();
			if (ly.ActType == ACT_TYPE::IDENTITY)
				h_t.identity();
			if (ly.ActType == ACT_TYPE::LEAKYRELU)
				h_t.leakyrelu();

			// Store output for current time step
			outputs.push_back(h_t);

			// Update h_prev for next time step
			h_prev = h_t;
		}

		ly.output = outputs_to_matrix(outputs);
	}

	// Convert outputs to matrix or return as needed
	return outputs_to_matrix(outputs);
}



Matrix NN::ForwardPropagation(Matrix input)
{
	// First layer, put the inputs as flattened
	Layers[0].output = input.flatten();

	// Next layers, forward
	for (size_t i = 1; i < Layers.size(); i++)
	{
		auto& current = Layers[i];
		auto& prev = Layers[i - 1];

		if (prev.output.cols() != current.weights.rows())
			current.output = (prev.output.flatten() * current.weights);
		else
			current.output = (prev.output * current.weights);

		current.output += current.biases;
		if (current.ActType == ACT_TYPE::SIGMOID)
			current.output.sigmoid();
		if (current.ActType == ACT_TYPE::RELU)
			current.output.relu();
		if (current.ActType == ACT_TYPE::LEAKYRELU)
			current.output.leakyrelu();
		if (current.ActType == ACT_TYPE::IDENTITY)
			current.output.identity();
	}

	return  Layers.back().output;
}
NN::NN()
{
	Init();
}


void NN::Init()
{
	if (InitOK)
		return;	
	InitOK = 1;
	Layer inputLayer(0.001f, ACT_TYPE::SIGMOID, 0, 0);
	Layer outputLayer(0.001f, ACT_TYPE::SIGMOID, 10, 128);

	outputLayer.weights.rand_he();	
	for (auto& j : outputLayer.biases.data())
		j = 0.01f;

	Layers.push_back(inputLayer);  // Input layer
	Layers.push_back(outputLayer);  // Output layer

	for (auto& ly : Layers)
	{
		ly.weights_recurrent_hidden.rand_he();
	}

}

void NN::BackPropagationRNN(Matrix label) {
	auto OurOutput = Layers.back().output; // Output of the last layer (T x output_size)

	// Loss gradient (assuming MSE)
	auto delta = OurOutput - label; // Shape: (T x output_size)

	// Loop through layers in reverse order (as in standard backprop)
	for (int i = (int)(Layers.size() - 1); i > 0; i--) {
		Layer& curr = Layers[i];
	
		// Initialize gradients for recurrent weight updates
		Matrix d_weights_recurrent_hidden  = Matrix::Zeros(curr.weights_recurrent_hidden.rows(), curr.weights_recurrent_hidden.cols());

		// Initialize gradient accumulation for hidden state (BPTT)
		Matrix d_h_next; // Gradient of next hidden state
		d_h_next.init(curr.output.rows(), curr.output.cols(), 0.0f); // Zero-initialized

		// Backpropagate **through time** (from last time step to first)
		int T = curr.output.rows(); // Number of time steps
		for (int t = T - 1; t >= 0; t--) {
			// Extract output for current time step
			Matrix h_t = curr.output.row(t);  // Shape: (1 x hidden_size)
			Matrix h_prev = (t > 0) ? curr.output.row(t - 1) : Matrix::Zeros(h_t.rows(), h_t.cols());

			// Compute gradient of loss w.r.t output
			curr.biases += delta.row(t) * ((float)-curr.lr);

			// Weight update (hidden-to-output connection)
			Matrix gradient = h_t.transpose() * delta.row(t);

			// Gradient clipping
			float clip_value = 5.0f;
			for (auto& value : gradient.data()) {
				value = std::max(-clip_value, std::min(value, clip_value));
			}
			curr.weights += gradient * ((float)-curr.lr);

			// Compute activation function derivative for hidden state
			Matrix der = h_t;
			if (curr.ActType == ACT_TYPE::SIGMOID)
				der.sigmoid_derivative();
			if (curr.ActType == ACT_TYPE::RELU)
				der.relu_derivative();
			if (curr.ActType == ACT_TYPE::LEAKYRELU)
				der.leakyrelu_derivative();
			if (curr.ActType == ACT_TYPE::IDENTITY)
				der.identity_derivative();

			// Compute the error term for the hidden state
			Matrix d_h = delta.row(t) * curr.weights.transpose();
			d_h += d_h_next * curr.weights_recurrent_hidden.transpose(); // Accumulate recurrent gradients
			d_h = d_h.multiply_inplace(der); // Element-wise multiply with activation derivative

			// Compute weight updates for recurrent connections
			d_weights_recurrent_hidden += h_prev.transpose() * d_h;

			// Pass gradient to the next time step
			d_h_next = d_h;
		}

		// Update recurrent weights after accumulating over all time steps
		curr.weights_recurrent_hidden += d_weights_recurrent_hidden * ((float)-curr.lr);

		// Compute gradient for next layer (if not the first layer)
		if (i > 1) {
			delta = delta * curr.weights.transpose();
		}
	}
}


void NN::BackPropagation(Matrix label)
{
	auto OurOutput = Layers.back().output; // 1x10

	// Calculation of the derivation of MSE, 2 is ignored for simplicity because it doesn't affect gradient descent
	auto delta = OurOutput - label; // 1x10
	for (int i = (int)(Layers.size() - 1); i > 0; i--)
	{
		Layer& curr = Layers[i];
		Layer& prev = Layers[i - 1];

		// biased += σ'(z) , delta = 1x10, 
		curr.biases += delta * ((float)-curr.lr);

		// weights += prev.Y.T * σ'(z)
		Matrix gradient = (prev.output.transpose() * delta); // 128x10, 784x128

		float clip_value = 5.0f;
		for (auto& value : gradient.data()) {
			value = std::max(-clip_value, std::min(value, clip_value));
		}
		curr.weights += gradient * ((float)-curr.lr);


		// σ'(z) = σ(z) * (1 - σ(z))

		// Sigmoid Derivative
		Matrix der = prev.output; // 1x128, 1x784
		if (prev.ActType == ACT_TYPE::SIGMOID)
			der.sigmoid_derivative();
		if (prev.ActType == ACT_TYPE::RELU)
			der.relu_derivative();
		if (prev.ActType == ACT_TYPE::LEAKYRELU)
			der.leakyrelu_derivative();
		if (prev.ActType == ACT_TYPE::IDENTITY)
			der.identity_derivative();

		// delta = (delta * prev.W.T) x σ'(z);
		// This multiplication implements the chain rule, combining the loss gradient with the derivative of the activation function for each layer.
		if (i > 1) // don't calculate for the first layer
		{
			auto fd = (delta * curr.weights.transpose());
#ifdef CLIP_DEBUG
			//				MatrixToClipboard(fd);
#endif

			delta = fd.multiply_inplace(der); // 1x128 second time
		}

	}
}

float NN::track_accuracy_dml(ML& ml, std::vector<DataSet>& dataset)
{
	if (!UseNewDMLLib)
		return track_accuracy(dataset);

	int correct_predictions = 0;

	for (size_t iData = 0; iData < dataset.size(); iData += CurrentBatch)
	{

		// upload the input
		if (CurrentBatch > 1)
		{
			std::vector<float> fullinput;
			for (unsigned int i = 0; i < (unsigned int)CurrentBatch && (i + iData) < dataset.size() ; i++)
			{
				auto& d = dataset[iData + i];
				fullinput.insert(fullinput.end(), d.input.data().begin(), d.input.data().end());
			}
			ml.ops[0].WithTag(1).buffer->Upload(&ml, fullinput.data(), fullinput.size() * 4);
		}
		else
		{
			auto& sample = dataset[iData];
			ml.ops[0].WithTag(1).buffer->Upload(&ml, sample.input.data().data(), sample.input.data().size() * 4);
		}

		// Run fpp
		ml.Run(0);

		// Download output
		std::vector<char> out;
		ml.ops[0].WithTag(ltag(Layers.size() - 1,MODE_TAGS::OUTPUT)).buffer->Download(&ml,(size_t)-1, out);

		auto bz = out.size() / CurrentBatch;
		for (int j = 0; j < CurrentBatch && (j + iData) < dataset.size(); j++)
		{
			Matrix output;
			auto& last_ly = Layers[Layers.size() - 1];
			auto num_outputs = last_ly.weights.cols();
			output.init(1, num_outputs);
			memcpy(output.data().data(), out.data() + j*bz, num_outputs * 4);


			int predicted_label = output.argmax_rows()[0];

			// Get true label (index of 1 in one-hot encoded label)
			int true_label = dataset[iData + j].label.argmax_rows()[0];

			// Count correct predictions
			if (predicted_label == true_label) {
				correct_predictions++;
			}
		}

	}

	// Compute and display accuracy
	accuracy = (float)correct_predictions / dataset.size() * 100.0f;
	//		std::cout << "Accuracy: " << accuracy << "%" << std::endl;
	return accuracy;

//	return track_accuracy(dataset);
}

float NN::track_accuracy(std::vector<DataSet>& dataset)
{
	int correct_predictions = 0;

	for (auto& sample : dataset) {
		// Forward propagate to get the network's output
		Matrix output = ForwardPropagation(sample.input);

		// Get predicted label (index of max output value)
		output = output.flatten();
		int predicted_label = output.argmax_rows()[0];

		// Get true label (index of 1 in one-hot encoded label)
		int true_label = sample.label.argmax_rows()[0];

		// Count correct predictions
		if (predicted_label == true_label) {
			correct_predictions++;
		}
	}

	// Compute and display accuracy
	accuracy = (float)correct_predictions / dataset.size() * 100.0f;
	//		std::cout << "Accuracy: " << accuracy << "%" << std::endl;
	return accuracy;
}

void NN::test_network(std::vector<DataSet>& test_dataset) {
	int correct_predictions = 0;

	for (auto& test_sample : test_dataset) {
		// Forward propagate to get the network's output
		Matrix output = ForwardPropagation(test_sample.input);

		// Get predicted label (index of max output value)
		int predicted_label = output.argmax_rows()[0];

		// Get true label (index of 1 in one-hot encoded label)
		int true_label = test_sample.label.argmax_rows()[0];

		// Count correct predictions
		if (predicted_label == true_label) {
			correct_predictions++;
		}
	}

	// Compute and display test accuracy
	accuracy = (float)correct_predictions / test_dataset.size() * 100.0f;
//	std::cout << "Test Accuracy: " << accuracy << "%" << std::endl;
}





void TensorDebug(dml::Expression&)
{
/*	auto desc = e.GetOutputDesc();
	auto sizetotal = desc.totalTensorSizeInBytes;
//	std::cout << "Shape: ";
//	for (auto size : desc.sizes) std::cout << size << " ";
//	std::cout << std::endl;
//	std::cout << "Total size: " << sizetotal << std::endl;
	auto ty = desc.dataType;
//	std::cout << "Type: " << ty << std::endl;
	if (desc.strides.has_value())
	{
	//	std::cout << "Strides: ";
//		for (auto size : desc.strides.value()) std::cout << size << " ";
//		std::cout << std::endl;
	}
//	std::cout << std::endl;
*/
}

void NN::trainRNN(std::vector<DataSet> ds, int numEpochs, bool AndAccuracy) 
{
	auto rng = std::default_random_engine{};
	HRESULT hr = S_OK;

	for (int i = 0; i < numEpochs; i++) {
#ifndef ML_DEBUG_1
		std::shuffle(std::begin(ds), std::end(ds), rng);
#endif
		if (cbf)
			hr = cbf(i, -1, 0, 0, 0, paramcbf);
		if (FAILED(hr))
			return;

		float totalLoss = 0.0;  // Initialize total loss

		for (size_t y = 0; y < ds.size(); y++) {
			// Perform forward propagation for RNN
			ForwardPropagationRNN(ds[y].input);

			// Calculate loss only after the first iteration to avoid using uninitialized values
			if (y > 0)
				totalLoss += error(Layers.back().output, ds[y].label);

			if (cbf)
				hr = cbf(i, (int)y, 1, totalLoss, 0, paramcbf);
			if (FAILED(hr))
				return;

			// Perform Backpropagation Through Time (BPTT)
			BackPropagationRNN(ds[y].label);

#ifdef CLIP_DEBUG
			for (size_t il = 0; il < Layers.size(); il++) {
				auto& l = Layers[il];
				MatrixToClipboard(l.weights);
				MatrixToClipboard(l.biases);
				MatrixToClipboard(l.output);
			}
#endif

			if (cbf)
				hr = cbf(i, (int)y, 3, totalLoss, 0, paramcbf);
			if (FAILED(hr))
				return;
		}

		if (cbf)
			hr = cbf(i, -1, 4, totalLoss / ds.size(), 0, paramcbf);
		if (FAILED(hr))
			return;

		if (AndAccuracy) {
			float acc = track_accuracy(ds);
			if (cbf)
				hr = cbf(i, -1, 5, totalLoss / ds.size(), acc, paramcbf);
			if (FAILED(hr))
				return;
		}
	}
	PostTrainEnd();
}


void NN::train(std::vector<DataSet> ds, int numEpochs, bool AndAccuracy)
{
	auto rng = std::default_random_engine{};
	HRESULT hr = S_OK;
	for (int i = 0; i < numEpochs; i++)
	{
#ifndef ML_DEBUG_1
		std::shuffle(std::begin(ds), std::end(ds), rng);
#endif
		if (cbf)
			hr = cbf(i, -1, 0, 0, accuracy, paramcbf);
		if (FAILED(hr))
			return;
		float totalLoss = 0.0;  // Initialize total loss
		for (size_t y = 0; y < ds.size(); y++)
		{
			if (y > 0)
				totalLoss += error(Layers.back().output, ds[y].label);
			if (cbf)
				hr = cbf(i, (int)y, 1, totalLoss, accuracy, paramcbf);
			if (FAILED(hr))
				return;
			ForwardPropagation(ds[y].input);
			if (cbf)
				hr = cbf(i, (int)y, 2, totalLoss, accuracy, paramcbf);
			if (FAILED(hr))
				return;
			BackPropagation(ds[y].label);
#ifdef CLIP_DEBUG
			for (size_t il = 0; il < Layers.size(); il++)
			{
				auto& l = Layers[il];
				MatrixToClipboard(l.weights);
				MatrixToClipboard(l.biases);
				MatrixToClipboard(l.output);
			}
#endif
			if (cbf)
				hr = cbf(i, (int)y, 3, totalLoss, accuracy, paramcbf);
			if (FAILED(hr))
				return;
		}
		if (cbf)
			hr = cbf(i, -1, 4, totalLoss / ds.size(), accuracy, paramcbf);
		if (FAILED(hr))
			return;

		if (AndAccuracy)
		{
			float acc = track_accuracy(ds);
			if (cbf)
				hr = cbf(i, -1, 5, totalLoss / ds.size(), acc, paramcbf);
			if (FAILED(hr))
				return;

		}

		// Print average loss for the epoch
//			std::cout << "Epoch " << i + 1 << " completed, Average Loss: "
//				<< totalLoss / ds.size() << "\n";
			// Track accuracy after each epoch
	}

	PostTrainEnd();
}


void NN::MatrixToClipboard(ML* ml, MLBUFFER& m)
{
	std::vector<char> temp;
	m.Download(ml, m.b.sz(), temp);
	if (temp.empty())
		return;
	Matrix output;
	output.init(m.GetOutputDesc().sizes[2], m.GetOutputDesc().sizes[3]);

	memcpy(output.data().data(), temp.data(), output.data().size() * 4);
	MatrixToClipboard(output);
}

void NN::MatrixToClipboard(Matrix& m)
{
	std::string str;
	for (int i = 0; i < m.rows(); i++)
	{
		for (int j = 0; j < m.cols(); j++)
		{
			str += std::to_string(m.at(i, j)) + "\t";
		}
		str += "\n";
	}
	// Copy to clipboard
	const size_t len = str.length() + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), str.c_str(), len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}


extern std::vector<CComPtr<IDXGIAdapter1>> all_adapters;

HRESULT NN::dmltrain3(std::vector<DataSet> data, unsigned int batch, int numEpochs)
{
	CoInitialize(0);
	auto rng = std::default_random_engine{};
	std::vector<char> temp;
	ML ml;
	HRESULT hr = 0;
	CComPtr<IDXGIAdapter> ad;
	auto iAdapter = SettingsX->GetRootElement().vv("iAdapter").GetValueInt(0);
	if (iAdapter > 0 && iAdapter <= all_adapters.size())
		ad = all_adapters[iAdapter - 1];
	hr = ml.On(ad);




	// Forward and Bckward Propagation Operator
	MLBUFFER B_InputOp1;
	MLBUFFER B_InputOp2;
	MLBUFFER B_Final;
	MLBUFFER B_Label;
	dml::Graph graph1(ml.dmlDevice);
	dml::Graph graph2(ml.dmlDevice);
//	int nexttid = 0;

	/*
			// Single operator, Single Graph
			// This doesn't work because output buffers overlap with input and parallelism can't be done

			ml.ops.push_back(GetTrainOperator(ml, batch, nexttid,data,B_InputOp1,B_Label,B_Final,true,true,&graph1,&graph1));
	*/

	// Two Operators, Two graphs
	if (UseNewDMLLib)
	{
		auto op1 = GetFPO(ml, batch, data);
		if (!op1)
			return E_FAIL;
		if (op1->dmlCompiledOperator == 0)
			return E_FAIL;
		ml.ops.push_back(*op1);
		auto op2 = GetBPO(ml, batch, data, *op1);
		if (!op2)
			return E_FAIL;
		if (op2->dmlCompiledOperator == 0)
			return E_FAIL;
		ml.ops.push_back(*op2);
	}
	else
	{
/*		auto fpop = GetTrainOperator(ml, batch, nexttid, data, B_InputOp1, B_InputOp2, B_Label, B_Final, true, false, &graph1, 0);
		if (fpop.dmlCompiledOperator == 0)
			return E_FAIL;
		ml.ops.push_back(fpop);
		nexttid = 0;
		auto bpop = GetTrainOperator(ml, batch, nexttid, data, B_InputOp1, B_InputOp2, B_Label, B_Final, false, true, 0, &graph2);
		if (bpop.dmlCompiledOperator == 0)
			return E_FAIL;
		ml.ops.push_back(bpop);
		*/
	}

	/*
			// Two operators, one graph
			ml.ops.push_back(GetTrainOperator(ml, batch, nexttid,data, B_InputOp1, B_Label, B_Final, true, false,&graph1,0));
			ml.ops.push_back(GetTrainOperator(ml, batch, nexttid,data, B_InputOp1, B_Label, B_Final, false, true,0, &graph1));
	*/

	// Initialize
	for (auto& op : ml.ops)
		op.CreateInitializer(ml.dmlDevice);

	// https://learn.microsoft.com/en-us/windows/ai/directml/dml-binding
	// Query the operator for the required size (in descriptors) of its binding table.
	ml.CreateHeap();

	// Create a binding table over the descriptor heap we just created.
	for (auto& op : ml.ops)
		op.CreateBindingTable(ml.dmlDevice, ml.descriptorHeap);

	// Bind Temporary and 
	for (auto& op : ml.ops)
	{
		op.tapi();
	}


	// The command recorder is a stateless object that records Dispatches into an existing Direct3D 12 command list.
	hr = ml.CreateCommandRecorder();

	// Record execution of the operator initializer.
	ml.Record(0);

	// Execute it
	ml.CloseExecuteResetWait();



	bool MustBind = 1;
	if (MustBind)
	{
#ifdef SINGLE_BUFFER
		// Swap all the buffers
		auto globalbs = global_buffer->ls;
		global_buffer = 0;
		global_buffer = std::make_shared<MLBUFFER>();
		global_buffer->Create2(ml.d3D12Device, TensorSizeAlign(globalbs), true);
		for (auto& op : ml.ops)
		{
			for (auto& bi : op.bindings_in)
			{
				auto de = (DML_BUFFER_BINDING*)bi.Desc;
				de->Buffer = global_buffer->b;
			}
			for (auto& bi : op.bindings_out)
			{
				auto de = (DML_BUFFER_BINDING*)bi.Desc;
				de->Buffer = global_buffer->b;
			}
		}
#endif
	}


	// Upload once initial weights/biases/input
	if (UseNewDMLLib)
	{
		for (size_t i = 1; i < Layers.size(); i++)
		{
			ml.ops[0].WithTag(ltag(i, MODE_TAGS::WEIGHTS)).buffer->Upload(&ml, Layers[i].weights.data().data(), Layers[i].weights.data().size() * 4);
			ml.ops[0].WithTag(ltag(i, MODE_TAGS::BIASES)).buffer->Upload(&ml, Layers[i].biases.data().data(), Layers[i].biases.data().size() * 4);
		}
	}
	else
	{
/*		for (auto& l : Layers)
		{
			if (l.B_Biases.b.b)
				l.B_Biases.Upload(&ml,l.biases.data().data(), l.biases.data().size() * 4);
			if (l.B_Weights.b.b)
				l.B_Weights.Upload(&ml,l.weights.data().data(), l.weights.data().size() * 4);
		}
*/
	}


	for (int iEpoch = 0; iEpoch < numEpochs; iEpoch++)
	{
#ifndef ML_DEBUG_1
		std::shuffle(std::begin(data), std::end(data), rng);
#endif

		float totalloss = 0;
		for (size_t iData = 0; iData < data.size(); iData += batch)
		{
			// callback
/*				if (iData > 0)
				{
					auto& bl = Layers.back();
					int rows = bl.B_Outputs.GetOutputDesc().sizes[2];
					int cols = bl.B_Outputs.GetOutputDesc().sizes[3];
					if (bl.output.rows() != rows || bl.output.cols() != cols)
						bl.output.init(rows, cols);
					bl.B_Outputs.Download(ml.d3D12Device, ml.commandList, bl.B_Outputs.ls, &ml, temp);
					bl.output.init(rows, cols);
					int size = rows * cols * 4;
					memcpy(bl.output.data().data(), temp.data(), size);
					totalloss += error(Layers.back().output, data[iData].label);
				}
*/

			if (cbf)
				hr = cbf((int)iEpoch, (int)iData, 1, totalloss, accuracy, paramcbf);
			if (FAILED(hr))
				return E_ABORT;


			// Bind and execute the operator on the GPU.
			ml.SetDescriptorHeaps();

			// Input + Data uploading
			if (batch > 1)
			{
				std::vector<float> fullinput;
				for (unsigned int i = 0; i < batch && (i + iData) < data.size() ; i++)
				{
					auto& d = data[iData + i];
					fullinput.insert(fullinput.end(), d.input.data().begin(), d.input.data().end());
				}
				if (UseNewDMLLib)
					ml.ops[0].WithTag(1).buffer->Upload(&ml, fullinput.data(), fullinput.size() * 4);
				else
					B_InputOp1.Upload(&ml, fullinput.data(), fullinput.size() * 4);


				if (1)
				{
					std::vector<float> fulllabel;
					for (unsigned int i = 0; i < batch && (i + iData) < data.size(); i++)
					{
						auto& d = data[iData + i];
						fulllabel.insert(fulllabel.end(), d.label.data().begin(), d.label.data().end());
					}
					if (UseNewDMLLib)
					{
						if (ml.ops[1].WithTag2(2))
							ml.ops[1].WithTag(2).buffer->Upload(&ml, fulllabel.data(), fulllabel.size() * 4);
					}
					else
					if (B_Label.b.b)
						B_Label.Upload(&ml,fulllabel.data(), fulllabel.size() * 4);
				}
			}
			else
			{
				if (UseNewDMLLib)
				{
					ml.ops[0].WithTag(1).buffer->Upload(&ml, data[iData].input.data().data(), data[iData].input.data().size() * 4);
					if (ml.ops[1].WithTag2(2))
						ml.ops[1].WithTag(2).buffer->Upload(&ml, data[iData].label.data().data(), data[iData].label.data().size() * 4);
				}
				else
				{
					B_InputOp1.Upload(&ml, data[iData].input.data().data(), data[iData].input.data().size() * 4);
					if (B_Label.b.b)
						B_Label.Upload(&ml, data[iData].label.data().data(), data[iData].label.data().size() * 4);
				}
			}

			// Operator 1
			if (1)
			{
				auto& op = ml.ops[0];
				op.ResetToExecute();
				op.TransitionBindings(ml.commandList);

				// Binding		
				op.Bind();

				// And temporary/persistent resources
				op.tape();

				// And run it
				ml.dmlCommandRecorder->RecordDispatch(ml.commandList, op.dmlCompiledOperator, op.dmlBindingTable);
				ml.CloseExecuteResetWait();
				ml.SetDescriptorHeaps();
			}


			// Operator 2
			if (1)
			{
				auto& op = ml.ops[1];
				op.ResetToExecute();
				op.TransitionBindings(ml.commandList);

				op.Bind();

				// And temporary/persistent resources
				op.tape();

				// And run it
				ml.dmlCommandRecorder->RecordDispatch(ml.commandList, op.dmlCompiledOperator, op.dmlBindingTable);
				ml.CloseExecuteResetWait();
				ml.SetDescriptorHeaps();
			}

			MustBind = 0;

#ifdef CLIP_DEBUG
			//				MatrixToClipboard(ml.d3D12Device, ml.commandList, &ml, debugbuffer);
			//				MatrixToClipboard(ml.d3D12Device, ml.commandList, &ml, Layers.back().B_Outputs);
#endif

			// Update Biases/Weights
//			float* pt = 0;
			for (size_t ii = 0; ii < Layers.size(); ii++)
			{
				if (ii == 0)
					continue;
				//auto& layer = Layers[ii];
				if (UseNewDMLLib)
				{
					ml.ops[1].WithTag(ltag(ii, MODE_TAGS::BIASESBPOUT)).buffer->Download(&ml,(size_t)-1, temp);
					ml.ops[0].WithTag(ltag(ii, MODE_TAGS::BIASES)).buffer->Upload(&ml, temp.data(),temp.size());

					ml.ops[1].WithTag(ltag(ii, MODE_TAGS::WEIGHTSBPOUT)).buffer->Download(&ml,(size_t)-1, temp);
					ml.ops[0].WithTag(ltag(ii, MODE_TAGS::WEIGHTS)).buffer->Upload(&ml, temp.data(), temp.size());
				}
				else
				{
/*					if (layer.B_BiasesOut.b.b)
					{
						layer.B_BiasesOut.Download(&ml, layer.B_BiasesOut.b.sz(), temp);
						pt = (float*)temp.data();
						layer.B_Biases.Upload(&ml, (float*)temp.data(), temp.size());
					}
					if (layer.B_WeightsOut.b.b)
					{
						layer.B_WeightsOut.Download(&ml, layer.B_WeightsOut.b.sz(), temp);
						pt = (float*)temp.data();
						layer.B_Weights.Upload(&ml, (float*)temp.data(), temp.size());
					}
					*/
				}
			}


#ifdef CLIP_DEBUG
			for (size_t il = 0; il < Layers.size(); il++)
			{
				auto& l = Layers[il];
				MatrixToClipboard(ml.d3D12Device, ml.commandList, &ml, l.B_Weights);
				MatrixToClipboard(ml.d3D12Device, ml.commandList, &ml, l.B_Biases);
				MatrixToClipboard(ml.d3D12Device, ml.commandList, &ml, l.B_Outputs);
			}
#endif


			// TEST after execution
#ifdef DEBUGML
				//debugbuffer.Download(ml.d3D12Device, ml.commandList, (size_t)-1, &ml, temp);
//				pt = (float*)temp.data();
//				B_Final.Download(ml.d3D12Device, ml.commandList, (size_t)-1, &ml, temp);
//				pt = (float*)temp.data();
			for (size_t i = 0; i < Layers.size(); i++)
			{
				auto& l = Layers[i];
				l.B_Outputs.Download(ml.d3D12Device, ml.commandList, (size_t)-1, &ml, temp);
				pt = (float*)temp.data();
				if (l.B_Biases.b)
					l.B_Biases.Download(ml.d3D12Device, ml.commandList, (size_t)-1, &ml, temp);
				pt = (float*)temp.data();
				if (l.B_Weights.b)
					l.B_Weights.Download(ml.d3D12Device, ml.commandList, (size_t)-1, &ml, temp);
				pt = (float*)temp.data();
			}
#endif
		}



		// Epoch end
		for (size_t i = 0 ; i < Layers.size() ; i++)
		{
			auto& l = Layers[i];
			if (UseNewDMLLib)
			{
				// Outputs
				if (auto b1 = ml.ops[0].WithTag2(ltag(i, MODE_TAGS::OUTPUT)))
				{
					auto e = b1->buffer;
					e->Download(&ml, (size_t)-1, temp);
					if ((unsigned int)l.output.rows() != e->GetOutputDesc().sizes[2] || (unsigned int)l.output.cols() != e->GetOutputDesc().sizes[3])
						l.output.init(e->GetOutputDesc().sizes[2], e->GetOutputDesc().sizes[3]);
					memcpy(l.output.data().data(), temp.data(), l.output.cols() * l.output.rows() * 4);
				}

				// Biases
				if (auto b1 = ml.ops[1].WithTag2(ltag(i, MODE_TAGS::BIASESBPOUT)))
				{
					auto e = b1->buffer;
					e->Download(&ml, (size_t)-1, temp);
					if ((unsigned int)l.biases.rows() != e->GetOutputDesc().sizes[2] || (unsigned int)l.biases.cols() != e->GetOutputDesc().sizes[3])
						l.biases.init(e->GetOutputDesc().sizes[2], e->GetOutputDesc().sizes[3]);
					memcpy(l.biases.data().data(), temp.data(), l.biases.cols() * l.biases.rows() * 4);
				}

				// Weights
				if (auto b1 = ml.ops[1].WithTag2(ltag(i, MODE_TAGS::WEIGHTSBPOUT)))
				{
					auto e = b1->buffer;
					e->Download(&ml, (size_t)-1, temp);
					if ((unsigned int)l.weights.rows() != e->GetOutputDesc().sizes[2] || (unsigned int)l.weights.cols() != e->GetOutputDesc().sizes[3])
						l.weights.init(e->GetOutputDesc().sizes[2], e->GetOutputDesc().sizes[3]);
					memcpy(l.weights.data().data(), temp.data(), l.weights.cols() * l.weights.rows() * 4);
				}
			}
			else
			{ 			
/*				// Outputs 
				if (l.B_Outputs.b.b)
				{
					l.B_Outputs.Download(&ml, l.B_Outputs.b.sz(), temp);
					if ((unsigned int)l.output.rows() != l.B_Outputs.GetOutputDesc().sizes[2] || (unsigned int)l.output.cols() != l.B_Outputs.GetOutputDesc().sizes[3])
						l.output.init(l.B_Outputs.GetOutputDesc().sizes[2], l.B_Outputs.GetOutputDesc().sizes[3]);
					memcpy(l.output.data().data(), temp.data(), l.output.cols() * l.output.rows() * 4);
				}

				// Biases
				if (l.B_BiasesOut.b.b)
				{
					l.B_BiasesOut.Download(&ml, l.B_Biases.b.sz(), temp);
					if ((unsigned int)l.biases.rows() != l.B_BiasesOut.GetOutputDesc().sizes[2] || (unsigned int)l.biases.cols() != l.B_BiasesOut.GetOutputDesc().sizes[3])
						l.biases.init(l.B_BiasesOut.GetOutputDesc().sizes[2], l.B_BiasesOut.GetOutputDesc().sizes[3]);
					memcpy(l.biases.data().data(), temp.data(), l.biases.cols() * l.biases.rows() * 4);
				}

				// Weights
				if (l.B_WeightsOut.b.b)
				{
					l.B_Weights.Download(&ml, l.B_Weights.b.sz(), temp);
					if ((unsigned int)l.weights.rows() != l.B_Weights.GetOutputDesc().sizes[2] || (unsigned int)l.weights.cols() != l.B_Weights.GetOutputDesc().sizes[3])
						l.weights.init(l.B_Weights.GetOutputDesc().sizes[2], l.B_Weights.GetOutputDesc().sizes[3]);
					memcpy(l.weights.data().data(), temp.data(), l.weights.cols() * l.weights.rows() * 4);
				}
				*/
			}

		}


		track_accuracy_dml(ml,data);
	}



	PostTrainEnd();
	return S_OK;
}


std::optional<MLOP> NN::GetFPORNN([[maybe_unused]] ML& ml, [[maybe_unused]] unsigned int batch, [[maybe_unused]] std::vector<DataSet>& data)
{
	return {};
}

std::optional<MLOP> NN::GetFPO(ML& ml, unsigned int batch, std::vector<DataSet>& data)
{
	MLOP op(&ml);
	if (data.size() == 0)
		return {};

	// Input data
	op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { batch,1,1,(unsigned int)data[0].input.cols() * data[0].input.rows()} },1);

	// Weights and biases
	for (size_t i = 1; i < Layers.size() && Layers.size() >= 3; i++)
	{
		auto& layer = Layers[i];
		op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { 1,1,(unsigned int)layer.weights.rows(),(unsigned int)layer.weights.cols()} },ltag(i, MODE_TAGS::WEIGHTS));
		op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { 1,1,1,(unsigned int)layer.biases.cols()} }, ltag(i, MODE_TAGS::BIASES));
	}

	// Forward Propagation
	for (int i = 0; i < Layers.size(); i++)
	{
		auto& cl = Layers[i];
		if (i == 0) // input layer -> first hidden
		{
			op.AddOutput(dml::Identity(op.Item(0)), ltag(i, MODE_TAGS::OUTPUT));
		}
		else
		{
			auto& outl = op.WithTag(ltag(i - 1, MODE_TAGS::OUTPUT));

			std::vector<dml::Expression> batchOutputs;
			for (unsigned int b = 0; b < batch; ++b)
			{
				// Extract the b-th sample from the batch
				auto singleInput = dml::Slice(outl, {b, 0, 0, 0}, {1, 1, 1, outl.expr.GetOutputDesc().sizes[3]}, {1, 1, 1, 1});

				// Perform forward propagation for this single sample
				auto mul1 = dml::Gemm(singleInput, op.WithTag(ltag(i, MODE_TAGS::WEIGHTS)));
				auto add1 = dml::Add(mul1, op.WithTag(ltag(i, MODE_TAGS::BIASES)));
				auto output = 
					cl.ActType == ACT_TYPE::LEAKYRELU ? dml::ActivationLeakyRelu(add1) :
					cl.ActType == ACT_TYPE::IDENTITY ? dml::ActivationIdentity(add1) :
					cl.ActType == ACT_TYPE::RELU ? dml::ActivationRelu(add1) :
					dml::ActivationSigmoid(add1);
				batchOutputs.push_back(output);
			}

			if (batch == 1)
				op.AddOutput(batchOutputs[0], ltag(i, MODE_TAGS::OUTPUT));
			else
				op.AddOutput(dml::Join(batchOutputs, 0), ltag(i, MODE_TAGS::OUTPUT));
		}
	}

	op.Build();	
	return op;
}

/*

std::optional<MLOP> NN::GetFPO(ML& ml, unsigned int batch, unsigned int time_steps, std::vector<DataSet>& data)
{
    MLOP op(&ml);
    if (data.size() == 0)
        return {};

    unsigned int input_size = (unsigned int)(data[0].input.cols() * data[0].input.rows());

    // Input data: { batch, time_steps, input_size }
    op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { batch, time_steps, 1, input_size } }, 1);

    // Initialize hidden state: { batch, 1, 1, hidden_size }
    unsigned int hidden_size = Layers[1].weights.rows(); // Assuming first hidden layer defines hidden size
    op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { batch, 1, 1, hidden_size } }, ltag(0, MODE_TAGS::HIDDEN_STATE));

    // Weights and biases
    for (size_t i = 1; i < Layers.size(); i++)
    {
        auto& layer = Layers[i];
        op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { 1,1,(unsigned int)layer.weights.rows(),(unsigned int)layer.weights.cols()} }, ltag(i, MODE_TAGS::WEIGHTS));
        op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, { 1,1,1,(unsigned int)layer.biases.cols()} }, ltag(i, MODE_TAGS::BIASES));
    }

    // Forward Propagation through time steps
    auto& input_tensor = op.WithTag(ltag(0, MODE_TAGS::OUTPUT));
    auto hidden_state = op.WithTag(ltag(0, MODE_TAGS::HIDDEN_STATE));

    for (unsigned int t = 0; t < time_steps; ++t)
    {
        // Extract the t-th time step input: { batch, 1, 1, input_size }
        auto xt = dml::Slice(input_tensor, {0, t, 0, 0}, {batch, 1, 1, input_size}, {1, 1, 1, 1});

        // Apply RNN transformation: hidden = Activation(Wx * xt + Wh * hidden_state + b)
        auto Wx_xt = dml::Gemm(xt, op.WithTag(ltag(1, MODE_TAGS::WEIGHTS)));   // Wx * xt
        auto Wh_hidden = dml::Gemm(hidden_state, op.WithTag(ltag(2, MODE_TAGS::WEIGHTS))); // Wh * h
        auto add1 = dml::Add(Wx_xt, Wh_hidden);
        auto add2 = dml::Add(add1, op.WithTag(ltag(1, MODE_TAGS::BIASES))); // + b

        // Apply activation function
        auto new_hidden_state = dml::ActivationTanh(add2);

        // Store hidden state for next time step
        hidden_state = new_hidden_state;

        // Save output for this time step
        op.AddOutput(new_hidden_state, ltag(t, MODE_TAGS::OUTPUT));
    }

    op.Build();
    return op;
}

*/
std::optional<MLOP> NN::GetBPO(ML& ml, unsigned int batch, [[maybe_unused]] std::vector<DataSet>& data, MLOP& fpo)
{
	MLOP op(&ml);

	// Input Tensor
	auto& output_layer_outputs = fpo.WithTag(ltag(Layers.size() - 1, MODE_TAGS::OUTPUT));
	op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, output_layer_outputs.expr.GetOutputDesc().sizes },1,false, BINDING_MODE::BIND_IN, output_layer_outputs.buffer->b);

	// Label
	op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, {batch,1,1,10} },2);

	// Delta Loop
	dml::Expression T_DeltaFull = op.WithTag(1).expr - op.WithTag(2).expr;

	std::vector<dml::Expression> batchGradients(Layers.size());
	std::vector<dml::Expression> batchDeltas(Layers.size());

	// Pass 1: Create input tensors
	for (int i = (int)(Layers.size() - 1); i >= 0; i--)
	{
//		Layer& curr = Layers[i];
		if (i == 0)
			break;

//		Layer& prev = Layers[i - 1];

		auto& prev_outputs = fpo.WithTag(ltag(i - 1, MODE_TAGS::OUTPUT));	
		auto& curr_biases = fpo.WithTag(ltag(i, MODE_TAGS::BIASES));
		auto& curr_weights = fpo.WithTag(ltag(i, MODE_TAGS::WEIGHTS));

		if (op.WithTag2(ltag(i - 1, MODE_TAGS::OUTPUTBP)) == 0)
			op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, prev_outputs.expr.GetOutputDesc().sizes },ltag(i - 1,MODE_TAGS::OUTPUTBP),false,BINDING_MODE::BIND_IN, prev_outputs.buffer->b);
		if (op.WithTag2(ltag(i, MODE_TAGS::BIASESBP)) == 0)
			op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, curr_biases.expr.GetOutputDesc().sizes },ltag(i,MODE_TAGS::BIASESBP), false, BINDING_MODE::BIND_IN, curr_biases.buffer->b);
		if (op.WithTag2(ltag(i, MODE_TAGS::WEIGHTSBP)) == 0)
			op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, curr_weights.expr.GetOutputDesc().sizes }, ltag(i, MODE_TAGS::WEIGHTSBP), false, BINDING_MODE::BIND_IN, curr_weights.buffer->b);
	}


	for (unsigned int iBatch = 0; iBatch < batch; iBatch++)
	{
		//				auto T_Delta1 = dml::Slice(T_DeltaFull, { iBatch, 0, 0, 0 }, T_DeltaFull.GetOutputDesc().sizes, { 1, 1, 1, 1 });
		auto T_Delta1 = (batch == 1) ? T_DeltaFull : dml::Slice(T_DeltaFull, { iBatch, 0, 0, 0 }, { 1, 1, T_DeltaFull.GetOutputDesc().sizes[2], T_DeltaFull.GetOutputDesc().sizes[3] }, { 1, 1, 1, 1 });


		// Pass 1: Calculate gradients
		for (int i = (int)(Layers.size() - 1); i >= 0; i--)
		{
//			Layer& curr = Layers[i];
			if (i == 0)
			{
				if (batchDeltas[i].Impl() == 0)
					batchDeltas[i] = T_Delta1;
				else
					batchDeltas[i] = batchDeltas[i] + T_Delta1;
				break;
			}
			Layer& prev = Layers[i - 1];
			// Matrix gradient = (prev.output.transpose() * delta); // 128x10, 784x128

			auto& prev_outputs = op.WithTag(ltag(i - 1, MODE_TAGS::OUTPUTBP));
			auto& curr_weights = op.WithTag(ltag(i, MODE_TAGS::WEIGHTSBP));

			auto PrevO = (batch == 1) ? prev_outputs : dml::Slice(prev_outputs, { iBatch, 0, 0, 0 }, { 1, 1, prev_outputs.expr.GetOutputDesc().sizes[2], prev_outputs.expr.GetOutputDesc().sizes[3] }, { 1, 1, 1, 1 });
			auto bp_gradientunclipped = dml::Gemm(PrevO, T_Delta1, {}, DML_MATRIX_TRANSFORM_TRANSPOSE, DML_MATRIX_TRANSFORM_NONE); // 128x10, 784x128 for MNIST
			// Clip the gradient
			// float clip_value = 5.0f; 				for (auto& value : gradient.data()) { 						value = std::max(-clip_value, std::min(value, clip_value)); } 	
			auto bp_clippedgradient = dml::Clip(bp_gradientunclipped, -5.0f, 5.0f);

			if (batchGradients[i].Impl() == 0)
				batchGradients[i] = bp_clippedgradient;
			else
				batchGradients[i] = batchGradients[i] + bp_clippedgradient;


			// And the derivative
			auto bp_der1 = dml::Identity(PrevO);

			dml::Expression bp_der2;
			if (prev.ActType == ACT_TYPE::SIGMOID)
			{
				// Sigmoid Derivative
				auto bp_dx = dml::ActivationSigmoid(bp_der1);
				auto bp_one1 = ml.ConstantValueTensor(*op.graph, 1.0f, bp_dx.GetOutputDesc().sizes);
				auto bp_oneMinusSigmoid = bp_one1 - bp_dx;
				bp_der2 = bp_dx * bp_oneMinusSigmoid;
			}
			if (prev.ActType == ACT_TYPE::IDENTITY)
			{
				// Idenity derivative
				bp_der2 = ml.ConstantValueTensor(*op.graph, 1.0f, bp_der1.GetOutputDesc().sizes);
			}
			if (prev.ActType == ACT_TYPE::RELU)
			{
				// Relu derivative
				auto bp_dx = dml::ActivationRelu(bp_der1);
				auto bp_zeroTensor = ml.ConstantValueTensor(*op.graph, 0.0f, bp_dx.GetOutputDesc().sizes);
				auto bp_reluMask = dml::GreaterThan(bp_dx, bp_zeroTensor);
				bp_der2 = dml::Cast(bp_reluMask, DML_TENSOR_DATA_TYPE_FLOAT32);
			}
			if (prev.ActType == ACT_TYPE::LEAKYRELU)
			{
				// Leaky Relu derivative
				float alpha = 0.01f;  

				// Compute LeakyReLU activation
				auto bp_dx = dml::ActivationLeakyRelu(bp_der1, alpha);

				// Create zero tensor for comparison
				auto bp_zeroTensor = ml.ConstantValueTensor(*op.graph, 0.0f, bp_dx.GetOutputDesc().sizes);

				// Create mask: 1 where x > 0, 0 where x <= 0
				auto bp_reluMask = dml::GreaterThan(bp_dx, bp_zeroTensor);

				// Convert mask to float (1.0 for x > 0, 0.0 for x ≤ 0)
				auto bp_reluMaskFloat = dml::Cast(bp_reluMask, DML_TENSOR_DATA_TYPE_FLOAT32);

				// Create tensor filled with alpha (for x ≤ 0 case)
				auto bp_alphaTensor = ml.ConstantValueTensor(*op.graph, alpha, bp_dx.GetOutputDesc().sizes);

				// Compute final derivative: (1.0 * mask) + (alpha * (1 - mask))
				bp_der2 = dml::Add(
					dml::Multiply(bp_reluMaskFloat, bp_der1),  // 1.0 * gradient where x > 0
					dml::Multiply(dml::Subtract(bp_zeroTensor, bp_reluMaskFloat), bp_alphaTensor)  // alpha * gradient where x <= 0
				);

			}

			auto bp_Delta2 = dml::Gemm(T_Delta1, curr_weights, {}, DML_MATRIX_TRANSFORM_NONE, DML_MATRIX_TRANSFORM_TRANSPOSE);
			auto T_Delta2 = bp_Delta2 * bp_der2;
			if (batchDeltas[i].Impl() == 0)
				batchDeltas[i] = T_Delta1;
			else
				batchDeltas[i] = batchDeltas[i] + T_Delta1;
			T_Delta1 = T_Delta2;

		}
	}



	// Accumulate gradients
	if (batch > 1)
	{
		for (int i = 1; i < Layers.size(); i++) {
			batchGradients[i] = batchGradients[i] * (1.0f / batch);
		}
	}
	// and deltas
	if (batch > 1)
	{
		for (int i = 1; i < Layers.size(); i++) {
			batchDeltas[i] = batchDeltas[i] * (1.0f / batch);
		}
	}



	// Phase 2 : Update Biases / Weights
	for (int i = (int)(Layers.size() - 1); i >= 0; i--)
	{
		Layer& curr = Layers[i];
		auto T_Delta1 = batchDeltas[i];

		if (i == 0)
		{
			op.AddOutput(dml::Identity(T_Delta1), 3);
			break;
		}

		// 	curr.biases += delta * ((float)-curr.lr);
		auto bp_mincurrlr = ml.ConstantValueTensor(*op.graph, (float)-curr.lr, T_Delta1.GetOutputDesc().sizes);
		auto bp_mul2 = T_Delta1 * bp_mincurrlr;

		auto& curr_biases = fpo.WithTag(ltag(i, MODE_TAGS::BIASES));
		auto& curr_weights = fpo.WithTag(ltag(i, MODE_TAGS::WEIGHTS));
		if (op.WithTag2(ltag(i,MODE_TAGS::BIASESBP)) == 0)	
			op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, curr_biases.expr.GetOutputDesc().sizes }, ltag(i, MODE_TAGS::BIASESBP), false, BINDING_MODE::BIND_IN, curr_biases.buffer->b);

		op.AddOutput(op.WithTag(ltag(i,MODE_TAGS::BIASESBP)) + bp_mul2, ltag(i, MODE_TAGS::BIASESBPOUT));

		// curr.weights += gradient * ((float)-curr.lr);
		auto bp_mincurrlr2 = ml.ConstantValueTensor(*op.graph, (float)-curr.lr, batchGradients[i].GetOutputDesc().sizes);
		auto bp_mul3 = batchGradients[i] * bp_mincurrlr2;

		if (op.WithTag2(ltag(i, MODE_TAGS::WEIGHTSBP)) == 0)
			op.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32,curr_weights.expr.GetOutputDesc().sizes }, ltag(i, MODE_TAGS::WEIGHTSBP), false, BINDING_MODE::BIND_IN, curr_weights.buffer->b);

		op.AddOutput(op.WithTag(ltag(i, MODE_TAGS::WEIGHTSBP)) + bp_mul3, ltag(i, MODE_TAGS::WEIGHTSBPOUT));
	}


	op.Build();
	return op;
}




std::vector<DataSet> parse_mnist(const std::vector<unsigned char>& images,
	const std::vector<unsigned char>& labels,
	int numSamples)
{
	const int imageSize = 28 * 28;  // Each image is 28x28 pixels
	const int imageHeaderSize = 16; // Header size for images file
	const int labelHeaderSize = 8;  // Header size for labels file

	std::vector<DataSet> dataset;

	for (int i = 0; i < numSamples; i++)
	{
		// Parse image data (skip header)
		Matrix img(28, 28);  // Assuming Matrix is a class with (rows, cols) constructor
		int imageStart = imageHeaderSize + i * imageSize;

		for (int row = 0; row < 28; row++) {
			for (int col = 0; col < 28; col++) {
				img.set(row, col, images[imageStart + row * 28 + col] / 255.0f);  // Normalize to [0, 1]
			}
		}

		// Parse label data (skip header)
		int label = labels[labelHeaderSize + i];
		Matrix lbl(1, 10, 0);  // One-hot encode label: 1 row, 10 columns, initialized to 0
		lbl.set(0, label, 1.0f);  // Set the correct label index to 1

		// Create a DataSet object and add it to the vector
		DataSet data;
		data.input = img;
		data.label = lbl;
		dataset.push_back(data);
	}

	return dataset;
}
bool LoadFile(const wchar_t* f, std::vector<unsigned char>& d);

extern std::wstring fil;
void PROJECT::Save(XML3::XML& x)
{
	nn.Save(x.GetRootElement()["network"]);
	if (type == "mnist")
	{
		auto& data0 = x.GetRootElement()["externaldata"]["data"];
		data0.vv("type").SetValue("mnist");
		data0.vv("input").SetWideValue(if_mnist[0].c_str());
		data0.vv("inputlabels").SetWideValue(if_mnist[1].c_str());
		data0.vv("test").SetWideValue(if_mnist[2].c_str());
		data0.vv("testlabels").SetWideValue(if_mnist[3].c_str());
	}
	if (type == "mnist_binary")
	{
		auto& data0 = x.GetRootElement()["externaldata"]["data"];
		data0.vv("type").SetValue("mnist_binary");
		data0.vv("input").SetBinaryValue((const char*)if_mnist_binary[0].data(),if_mnist_binary[0].size());
		data0.vv("inputlabels").SetBinaryValue((const char*)if_mnist_binary[1].data(), if_mnist_binary[1].size());
		data0.vv("test").SetBinaryValue((const char*)if_mnist_binary[2].data(), if_mnist_binary[2].size());
		data0.vv("testlabels").SetBinaryValue((const char*)if_mnist_binary[3].data(), if_mnist_binary[3].size());

	}
}

PROJECT project;


void InitProject()
{
nn.Init();
}

void SaveProjectToFile(const wchar_t* f)
{
	XML3::XML x;
	project.Save(x);
	x.Save(f);
}

HRESULT ForCallback(int iEpoch, int iDataset, int step, float totalloss, float acc,void* param);
extern std::vector<CComPtr<IDXGIAdapter1>> all_adapters;

void TestAcc(int gpu)
{
	ML ml;
	HRESULT hr = 0;
	if (gpu)
	{
		CComPtr<IDXGIAdapter> ad;
		auto iAdapter = SettingsX->GetRootElement().vv("iAdapter").GetValueInt(0);
		if (iAdapter > 0 && iAdapter <= all_adapters.size())
			ad = all_adapters[iAdapter - 1];
		hr = ml.On(ad);
	}
	if (UseNewDMLLib && gpu)
	{
		auto op1 = nn.GetFPO(ml, CurrentBatch, project.data_set);
		if (op1)
		{
			// Upload weights/biases
			ml.ops.push_back(*op1);
			auto& Layers = nn.Layers;
			for (size_t i = 1; i < Layers.size(); i++)
			{
				ml.ops[0].WithTag(ltag(i, MODE_TAGS::WEIGHTS)).buffer->Upload(&ml, Layers[i].weights.data().data(), Layers[i].weights.data().size() * 4);
				ml.ops[0].WithTag(ltag(i, MODE_TAGS::BIASES)).buffer->Upload(&ml, Layers[i].biases.data().data(), Layers[i].biases.data().size() * 4);
			}
		}
	}
	float acc = 0;
	if (ml.ops.size() != 1)
		acc = nn.track_accuracy(project.data_set);
	else
	{
		ml.Prepare();
		acc = nn.track_accuracy_dml(ml, project.data_set);
	}
	wchar_t msg[100] = {};
	swprintf_s(msg, 100, L"Accuracy: %f", acc);
	MessageBox(mw(), msg, 0, 0);
}

void StartTrain(int m,void* p)
{
	nn.cbf = ForCallback;
	nn.paramcbf = p;
	TrainCancel = 0;
	if (m == 0)
	{
		std::thread t(&NN::train, (NN*)&nn, project.data_set,NumEpochsX, 1);
		t.detach();
	}
	if (m == 1)
	{
		std::thread t(&NN::dmltrain3, (NN*)&nn, project.data_set, CurrentBatch, NumEpochsX);
		t.detach();
	}
	if (m == 2)
	{
		std::thread t(&NN::trainRNN, (NN*)&nn, project.data_set, NumEpochsX, 1);
		t.detach();
	}
}


void LoadNetworkFile()
{
	XML3::XML x(fil.c_str());
	if (x.GetRootElement().GetChildrenNum() == 0)
		return;

	nn.Load(x.GetRootElement()["network"]);

	auto ed = x.GetRootElement().FindElementZ("externaldata");
	if(ed)
	{
		for (auto& d : *ed)
		{
			if (d.vv("type").GetValue() == "mnist")
			{
				std::array<std::wstring, 4> files;
				files[0] = d.vv("input").GetWideValue();
				files[1] = d.vv("inputlabels").GetWideValue();
				files[2] = d.vv("test").GetWideValue();
				files[3] = d.vv("testlabels").GetWideValue();

				std::vector<unsigned char> idx1ubyte;
				std::vector<unsigned char> idx3ubyte;
				LoadFile(files[0].c_str(), idx3ubyte);
				LoadFile(files[1].c_str(), idx1ubyte);
#ifdef ML_DEBUG_2
				project.data_set = parse_mnist(idx3ubyte, idx1ubyte, 6000);
#else
				project.data_set = parse_mnist(idx3ubyte, idx1ubyte, (int)(idx1ubyte.size() - 8));
#endif

				std::vector<unsigned char> idx1tbyte;
				std::vector<unsigned char> idx3tbyte;
				LoadFile(files[2].c_str(), idx3tbyte);
				LoadFile(files[3].c_str(), idx1tbyte);

#ifdef ML_DEBUG_2
				project.test_set = parse_mnist(idx3tbyte, idx1tbyte, 1000);
#else
				project.test_set = parse_mnist(idx3tbyte, idx1tbyte, (int)(idx1tbyte.size() - 8));
#endif

				project.if_mnist = files;
				project.type = "mnist";

			}
			if (d.vv("type").GetValue() == "mnist_binary")
			{
				auto b2 = d.vv("input").GetBinaryValue();
				auto b1 = d.vv("inputlabels").GetBinaryValue();
				auto t2 = d.vv("test").GetBinaryValue();
				auto t1 = d.vv("testlabels").GetBinaryValue();

				std::vector<unsigned char> idx1ubyte;
				std::vector<unsigned char> idx3ubyte;
				idx1ubyte.resize(b1.size());
				memcpy(idx1ubyte.data(), b1.p(), b1.size());
				idx3ubyte.resize(b2.size());
				memcpy(idx3ubyte.data(), b2.p(), b2.size());

#ifdef ML_DEBUG_2
				project.data_set = parse_mnist(idx3ubyte, idx1ubyte, 6000);
#else
				project.data_set = parse_mnist(idx3ubyte, idx1ubyte, (int)(idx1ubyte.size() - 8));
#endif

				std::vector<unsigned char> idx1tbyte;
				std::vector<unsigned char> idx3tbyte;
				idx1tbyte.resize(t1.size());
				memcpy(idx1tbyte.data(), t1.p(), t1.size());
				idx3tbyte.resize(t2.size());
				memcpy(idx3tbyte.data(), t2.p(), t2.size());

#ifdef ML_DEBUG_2
				project.test_set = parse_mnist(idx3tbyte, idx1tbyte, 1000);
#else
				project.test_set = parse_mnist(idx3tbyte, idx1tbyte, (int)(idx1tbyte.size() - 8));
#endif

				project.if_mnist_binary[0] = idx3ubyte;
				project.if_mnist_binary[1] = idx1ubyte;
				project.if_mnist_binary[2] = idx3tbyte;
				project.if_mnist_binary[3] = idx1tbyte;
				project.type = "mnist_binary";

			}
		}
	}

}

#include "imdb.h"

std::wstring TempFile3();

DWORD Run(const wchar_t* y, bool W, DWORD flg)
{
	PROCESS_INFORMATION pInfo = { 0 };
	STARTUPINFO sInfo = { 0 };

	sInfo.cb = sizeof(sInfo);
	wchar_t yy[1000];
	swprintf_s(yy, 1000, L"%s", y);
	CreateProcess(0, yy, 0, 0, 0, flg, 0, 0, &sInfo, &pInfo);
	SetPriorityClass(pInfo.hProcess, IDLE_PRIORITY_CLASS);
	SetThreadPriority(pInfo.hThread, THREAD_PRIORITY_IDLE);
	if (W)
		WaitForSingleObject(pInfo.hProcess, INFINITE);
	DWORD ec = 0;
	GetExitCodeProcess(pInfo.hProcess, &ec);
	CloseHandle(pInfo.hProcess);
	CloseHandle(pInfo.hThread);
	return ec;
}

void mnist_test(const wchar_t* out)
{

	std::vector<std::wstring> mnist_files = {
	L"train-images.idx3-ubyte",L"train-labels.idx1-ubyte",
	L"t10k-images.idx3-ubyte",L"t10k-labels.idx1-ubyte",
	};

	std::vector<wchar_t> a(10000);
	GetModuleFileName(0, a.data(), 10000);
	auto a2 = wcsrchr(a.data(), L'\\');
	if (a2)
		*a2 = 0;
	SetCurrentDirectory(a.data());


	auto tf = TempFile3();
	DeleteFile(tf.c_str());
	SHCreateDirectory(0, tf.c_str());
	auto tf2 = tf;
	tf2 += L"\\mnist.exe";
	CopyFile(L"mnist.exe", tf2.c_str(), false);
	SetCurrentDirectory(tf.c_str());

	auto root = tf;
	tf2 += L" -y";
	Run(tf2.c_str(), 1, CREATE_NO_WINDOW);



	std::vector<unsigned char> idx1ubyte;
	std::vector<unsigned char> idx3ubyte;
	LoadFile(mnist_files[0].c_str(), idx3ubyte);
	LoadFile(mnist_files[1].c_str(), idx1ubyte);
	auto dataset = parse_mnist(idx3ubyte, idx1ubyte, 6000);
	//	auto dataset = parse_mnist(idx3ubyte, idx1ubyte, 5000);

	std::vector<unsigned char> idx1tbyte;
	std::vector<unsigned char> idx3tbyte;
	LoadFile(mnist_files[2].c_str(), idx3tbyte);
	LoadFile(mnist_files[3].c_str(), idx1tbyte);
	auto test_dataset = parse_mnist(idx3tbyte, idx1tbyte, 1000);

	if (1)
	{
		// Each neuron has some weights and a bias
		nn.Layers.clear();

		Layer inputLayer(0.001f,ACT_TYPE::SIGMOID,0,0); 
		Layer hiddenLayer(0.001f, ACT_TYPE::RELU,128,784); // Relu
		Layer outputLayer(0.001f, ACT_TYPE::SIGMOID,10,128);      // Sigmoid

		// Random weights/biases for layers
		hiddenLayer.weights.rand_he();
		outputLayer.weights.rand_he();

		for (auto& j : hiddenLayer.biases.data())
			j = 0.01f;
		for (auto& j : outputLayer.biases.data())
			j = 0.01f;

		nn.Layers.push_back(inputLayer);  // Input layer
		nn.Layers.push_back(hiddenLayer);  // Input layer
		nn.Layers.push_back(outputLayer);  // Output layer

		DeleteFile(out);
		XML3::XML x(out);
		nn.Save(x.GetRootElement()["network"]);
		auto& data0 = x.GetRootElement()["externaldata"]["data"];
		data0.vv("type").SetValue("mnist_binary");

		for (int i = 0; i < 4; i++)
		{
			wchar_t b[1000];
			GetFullPathName(mnist_files[i].c_str(), 1000, b, 0);	
			mnist_files[i] = b;	
		}
		data0.vv("input").SetBinaryValue((const char*)idx3ubyte.data(), idx3ubyte.size());
		data0.vv("inputlabels").SetBinaryValue((const char*)idx1ubyte.data(), idx1ubyte.size());
		data0.vv("test").SetBinaryValue((const char*)idx3tbyte.data(), idx3tbyte.size());
		data0.vv("testlabels").SetBinaryValue((const char*)idx1tbyte.data(), idx1tbyte.size());
		x.Save();

		// Non random init for debugging
/*		for (size_t i = 1; i < nn.Layers.size(); i++)
		{
			auto& layer = nn.Layers[i];
			for (size_t i = 0; i < layer.weights.data().size(); i++)
				layer.weights.data()[i] = 0.1f;
			for (size_t i = 0; i < layer.biases.data().size(); i++)
				layer.biases.data()[i] = 0.2f;
		}
*/
//		nn.load_weights_and_biases("weights.bin");


		// first dataset all 1 for debugging
	//	for (auto& e : dataset[0].input.data())
		//	e = 1.0f;
//		nn.dmltrain2(dataset, 10);
//		nn.train(dataset, 10);


//		nn.test_network(test_dataset);

		// Save it
//		nn.save_weights_and_biases("weights.bin");
	}


	SetCurrentDirectory(a.data());

}


NN nn;
