
namespace dml
{

	inline Expression DiagonalMatrix(
		Graph& graph,
		TensorDimensions outputSizes,
		DML_TENSOR_DATA_TYPE valueDataType,
		INT offset,
		FLOAT Value
	)
	{
		detail::GraphBuilder* builder = graph.Impl();
		TensorDesc outputTensor(valueDataType, std::move(outputSizes), builder->GetTensorPolicy());

		DML_DIAGONAL_MATRIX_OPERATOR_DESC desc = {};
		desc.OutputTensor = outputTensor.AsPtr<DML_TENSOR_DESC>();
		desc.Offset = offset;
		desc.Value = Value;

		detail::NodeID node = builder->CreateOperatorNode(DML_OPERATOR_DIAGONAL_MATRIX, &desc, {});
		detail::NodeOutput* output = builder->CreateNodeOutput(node, 0, std::move(outputTensor));

		return output;
	}

	inline Expression FillValueConstant2(
		Graph& graph,
		TensorDimensions outputSizes,
		DML_TENSOR_DATA_TYPE valueDataType,
		DML_SCALAR_UNION value)
	{
		detail::GraphBuilder* builder = graph.Impl();
		TensorDesc outputTensor(valueDataType, std::move(outputSizes), builder->GetTensorPolicy());

		DML_FILL_VALUE_CONSTANT_OPERATOR_DESC desc = {};
		desc.OutputTensor = outputTensor.AsPtr<DML_TENSOR_DESC>();
		desc.ValueDataType = valueDataType;
		desc.Value = value;

		detail::NodeID node = builder->CreateOperatorNode(DML_OPERATOR_FILL_VALUE_CONSTANT, &desc, {});
		detail::NodeOutput* output = builder->CreateNodeOutput(node, 0, std::move(outputTensor));

		return output;
	}

	inline Expression OneHot2(
		Expression indices,
		Expression values,
		uint32_t outputLength,
		uint32_t axis)
	{
		detail::GraphBuilder* builder = indices.Impl()->GetGraphBuilder();
		TensorDesc indicesTensor = indices.Impl()->GetOutputDesc();
		TensorDesc valuesTensor = values.Impl()->GetOutputDesc();

		assert(axis < static_cast<uint32_t>(indicesTensor.sizes.size()));

		// The output and indices sizes must all match except for the active axis, which is supplied as outputLength.
		TensorDimensions outputSizes = indicesTensor.sizes;
		outputSizes[axis] = outputLength;

		TensorDesc outputTensor(valuesTensor.dataType, std::move(outputSizes), builder->GetTensorPolicy());

		DML_ONE_HOT_OPERATOR_DESC desc = {};
		desc.IndicesTensor = indicesTensor.AsPtr<DML_TENSOR_DESC>();
		desc.ValuesTensor = valuesTensor.AsPtr<DML_TENSOR_DESC>();
		desc.OutputTensor = outputTensor.AsPtr<DML_TENSOR_DESC>();
		desc.Axis = axis;

		detail::NodeOutput* const inputs[] = { indices.Impl(), values.Impl() };
		detail::NodeID node = builder->CreateOperatorNode(DML_OPERATOR_ONE_HOT, &desc, inputs);
		detail::NodeOutput* output = builder->CreateNodeOutput(node, 0, std::move(outputTensor));

		return output;
	}

}

