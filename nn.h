#include "dmllib.hpp"
#include "matrix2.hpp"
const wchar_t* s(size_t idx);
inline void nop()
{

}

enum class ACT_TYPE
{
	SIGMOID = 0,
	RELU = 1,
	IDENTITY = 2,
	LEAKYRELU = 3,
};

class Layer
{
public:

	bool Sel = 0;
	std::map<int, D2D1_RECT_F> DrawnNeurons;

	ACT_TYPE ActType = ACT_TYPE::SIGMOID; 
	Matrix output;
	double lr = 0.1;
	Matrix weights;
	Matrix weights_recurrent_hidden;
	Matrix weights_recurrent_output;
	Matrix biases_recurrent;
	Matrix biases;
	void Save(XML3::XMLElement& e);
	void Load(XML3::XMLElement& e);


	// DirectML structure tensors (used in old mode, now using DirectMLLib)
/*	MLBUFFER WeightsBPIn;
	MLBUFFER BiasesBPIn;
	dml::Expression T_OutputsBPIn;
	MLBUFFER B_Weights;
	MLBUFFER B_WeightsOut;
	MLBUFFER B_Biases;
	MLBUFFER B_BiasesOut;
	MLBUFFER B_Outputs;
	*/
	// A Layer is a collection of neurons
	Layer(double lrate, ACT_TYPE ActivationType, int num_neurons, int num_weights_per_neuron);




};


class DataSet
{
public:

	Matrix input;
	Matrix label;
};

class MLOP;

class NN
{
public:
	std::vector<Layer> Layers;

	NN();
	bool InitOK = 0;
	void Init();
	void RecalculateWB();
	void Save(XML3::XMLElement& e);
	void Load(XML3::XMLElement& e);

	int RNN_hidden_size = 32;

	Matrix ForwardPropagation(Matrix input);
//	Matrix ForwardPropagationBatch(Matrix input);
	Matrix ForwardPropagationRNN(const Matrix input);

	// Mean Squared Error function
	// MSE = 1/n * Σ(y - y')^2
	// y is the output, y' is the expected (label) output, n is the number of samples
	//  It's suitable for regression tasks but not typically used for classification tasks, where cross-entropy loss is more appropriate.
	float error(Matrix& out, Matrix& exp) {
		return (out - exp).square().sum() / out.cols();
	}

	void BackPropagation(Matrix label);
	void BackPropagationRNN(Matrix label);

	float accuracy = 0;
	float track_accuracy(std::vector<DataSet>& dataset);
	float track_accuracy_dml(ML& ml,std::vector<DataSet>& dataset);

	std::optional<MLOP> GetFPO(ML& ml, unsigned int batch, std::vector<DataSet>& data);
	std::optional<MLOP> GetFPORNN(ML& ml, unsigned int batch, std::vector<DataSet>& data);
	std::optional<MLOP> GetBPO(ML& ml, unsigned int batch, std::vector<DataSet>& data,MLOP& fpo);

	MLOP GetTrainOperator(ML& ml, unsigned int batch, int& starttid, std::vector<DataSet>& data, MLBUFFER& B_InputOp1, MLBUFFER& B_InputOp2, MLBUFFER& B_Label, MLBUFFER& B_Final, bool FP, bool BP, dml::Graph* g1, dml::Graph* g2);
	HRESULT dmltrain3(std::vector<DataSet> data, unsigned int batch, int numEpochs);
	void MatrixToClipboard(ML* ml, MLBUFFER& m);
	void MatrixToClipboard(Matrix& m);

	std::function<HRESULT(int iEpoch, int iDataset, int step, float totalloss, float acc, void* param)> cbf;
	void* paramcbf = 0;

	void train(std::vector<DataSet> ds, int numEpochs, bool AndAccuracy);
	void trainRNN(std::vector<DataSet> ds, int numEpochs, bool AndAccuracy);
	void test_network(std::vector<DataSet>& test_dataset);

};


extern NN nn;
struct PROJECT
{
	std::string type; // "mnist", "mnist_binary" defined
	std::array<std::wstring, 4> if_mnist;
	std::array<std::vector<unsigned char>, 4> if_mnist_binary;	
	std::vector<DataSet> data_set;
	std::vector<DataSet> test_set;
	void Save(XML3::XML& x);
};

