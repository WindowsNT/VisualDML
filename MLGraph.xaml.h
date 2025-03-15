#pragma once

#include "MLGraph.g.h"


inline int MAX_OP_TYPES = 13;
inline std::wstring optypes[] = { L"",L"Float32",L"Float16",L"UInt32",L"UInt16",L"UInt8",L"Int32",L"Int16",L"Int8",L"Float64",L"UInt64",L"Int64",L"UInt4",L"Int4" };
inline std::wstring optypes2[] = { L"",L"DML_TENSOR_DATA_TYPE_FLOAT32",L"DML_TENSOR_DATA_TYPE_FLOAT16",L"DML_TENSOR_DATA_TYPE_UINT32",L"DML_TENSOR_DATA_TYPE_UINT16",L"DML_TENSOR_DATA_TYPE_UINT8",L"DML_TENSOR_DATA_TYPE_INT32",L"DML_TENSOR_DATA_TYPE_INT16",L"DML_TENSOR_DATA_TYPE_INT8",L"DML_TENSOR_DATA_TYPE_FLOAT64",L"DML_TENSOR_DATA_TYPE_UINT64",L"DML_TENSOR_DATA_TYPE_INT64",L"DML_TENSOR_DATA_TYPE_UINT4",L"DML_TENSOR_DATA_TYPE_INT4" };

struct XLNODE;
inline unsigned long long nnn = 1;
inline unsigned long long nextn()
{
	return nnn++;
}

struct CONDITION
{
    std::string expression;

	void Ser(XML3::XMLElement& e)
	{
		e.vv("expression").SetValue(expression.c_str());
	}
	void Unser(XML3::XMLElement& e)
	{
		expression = e.vv("expression").GetValue();
	}   
};

struct CONNECTION
{
    unsigned long long key = 0;
    std::vector<CONDITION> conditions;

	void Ser(XML3::XMLElement& e)
	{
		e.vv("key").SetValueULongLong(key);
		for (auto& c : conditions)
		{
			auto& ce = e.AddElement("Condition");
			c.Ser(ce);
		}
	}

	void Unser(XML3::XMLElement& e)
	{
		key = e.vv("key").GetValueULongLong(0);
		for (auto& c : e)
		{
			CONDITION cc;
			cc.Unser(c);
			conditions.push_back(cc);
		}
	}
};

struct XLNODEBULLET
{
    D2F hit;
    bool S = 0;
    size_t i = 0;
    bool O = 0;
    std::wstring name;
    std::vector<CONNECTION> g;
};

struct VARIABLE
{
    std::wstring n;
    std::wstring orv;
    std::wstring runv;

    void Ser(XML3::XMLElement& e)
    {
        e.vv("n").SetWideValue(n.c_str());
        e.vv("v").SetWideValue(orv.c_str());
    }

    void Unser(XML3::XMLElement& e)
    {
        n = e.vv("n").GetWideValue();
        orv = e.vv("v").GetWideValue();
    }
};

struct VARIABLECHANGE
{
	std::wstring n; 
    int Type = 0; // 0 abs, 1 relative
    float V = 0.0f; 

    std::wstring str()
    {
        std::wstring d;
        d += n;
        d += L" ";
        if (Type == 0)
        {
            d += L"= ";
            d += std::to_wstring(fabs(V));
        }
        else
        {
            if (V < 0)
                d += L"-= ";
            else
                d += L"+= ";
            d += std::to_wstring(fabs(V));
        }
        return d;
    }

	void Ser(XML3::XMLElement& e)
	{
		e.vv("n").SetWideValue(n.c_str());
		e.vv("Type").SetValueInt(Type);
		e.vv("V").SetValueFloat(V);
	}

	void Unser(XML3::XMLElement& e)
	{
		n = e.vv("n").GetWideValue();
		Type = e.vv("Type").GetValueInt(0);
		V = e.vv("V").GetValueFloat(0);
	}
};

struct JUMP
{
    std::vector<CONDITION> conditions;
    size_t iNodeJumpTo = 0;
};

struct PARAM
{
    std::wstring n;
    std::wstring v;
    std::wstring save_v;
    float minv = std::numeric_limits<float>::min();
    float maxv = std::numeric_limits<float>::max();
	std::vector<std::wstring> list_names;  

    const wchar_t* w()
    {
        return v.c_str();
    }
    operator const wchar_t* ()
    {
        return w();
    }
    operator float()
    {
        if (v.empty())
            return 0;
        try
        {
            return std::stof(v.c_str());
        }
        catch (...)
        {
            return 0;
        }
    }
    operator double()
    {
        if (v.empty())
            return 0;
        try
        {
            return std::stod(v.c_str());
        }
		catch (...)
		{
			return 0;
		}
    }
    operator int()
    {
        if (v.empty())
            return 0;
        try {
            return std::stoi(v.c_str());
        }
        catch (...)
        {
            return 0;
        }
    }
    operator unsigned int()
    {
        if (v.empty())
            return 0;
        try
        {
            return std::stoul(v.c_str());
        }
        catch (...)
        {
            return 0;
        }
    }
    operator bool()
	{
        if (v.empty())
            return 0;
        try
        {
            return std::stoi(v.c_str()) == 1;
        }
		catch (...)
		{
			return 0;
		}
	}
    operator long long()
    {
        if (v.empty())
            return 0;
        try
        {
            return std::stoll(v.c_str());
        }
		catch (...)
		{
			return 0;
		}
    }
    operator unsigned long long()
    {
        if (v.empty())
            return 0;
        try
        {
            return std::stoull(v.c_str());
        }
        catch (...)
        {
            return 0;
        }
    }
};

struct XLNODE
{

    std::string code;
    std::vector<int> tidxs; // idx in MLOP, most nodes have one but some like BatchOperationGraidents have multiple
    bool BufferVisible = 0;
    D2F bhit = { };
    D2F bhit2 = { };

    int OpType = DML_TENSOR_DATA_TYPE_FLOAT32;


    signed long long ShareMemory = 0;
    bool bSelected = 0;
    std::vector<PARAM> Params;


    FMAP mapin;
    //FMAP mapout;

    std::wstring csv_input;
    std::wstring csv_output;
    std::vector<XLNODEBULLET> children;
    unsigned long long min_key(struct XLOP* par);



    virtual bool IsInput() { return false; }
    virtual bool AsksType() { return false; }
    virtual bool IsOutput() { return false; }
    virtual void Ser(XML3::XMLElement& e);
    virtual void Unser(XML3::XMLElement& e);
    bool S = 0;
    virtual std::wstring name() = 0;
    virtual std::wstring subname() = 0;
    D2F hit = {};

    virtual int nin() { return 0; }
    virtual int nout() { return 0; }
    virtual void Draw(MLOP*mlop,bool Active,bool Enabled,ID2D1DeviceContext5* r, size_t iop,size_t inod);

};

enum XLNODE_TYPE
{
    TYPE_INPUT = 1,
    TYPE_ACT_IDENTITY,TYPE_ACT_CELU,TYPE_ACT_ELU,TYPE_ACT_GELU,TYPE_ACT_HARDMAX,TYPE_ACT_HARDSIGMOID,TYPE_ACT_LEAKYRELU,TYPE_ACT_LINEAR,TYPE_ACT_LOGSOFTMAX,TYPE_ACT_PRELU, TYPE_ACT_PSOFTPLUS, TYPE_ACT_RELU, TYPE_ACT_SELU, TYPE_ACT_STANH, TYPE_ACT_SHRINK, TYPE_ACT_SIGMOID, TYPE_ACT_SOFTMAX, TYPE_ACT_SOFTPLUS, TYPE_ACT_SOFTSIGN, TYPE_ACT_TANH, TYPE_ACT_TRELU,
    TYPE_ABS,TYPE_ACOS,TYPE_ACOSH, TYPE_ADD,TYPE_ASIN,TYPE_ASINH,TYPE_ATAN,TYPE_ATANH, TYPE_ATANYX,TYPE_AVERAGEPOOLING,
	TYPE_BITAND, TYPE_BITCOUNT, TYPE_BITNOT, TYPE_BITOR, TYPE_BITSL, TYPE_BITSR, TYPE_BITXOR, TYPE_BATCHNORMALIZATION, TYPE_BATCHNORMALIZATIONGRAD,TYPE_BATCHNORMALIZATIONTRAINING, TYPE_BATCHNORMALIZATIONTRAININGGRAD,
	TYPE_CAST,TYPE_CEIL, TYPE_CLIP, TYPE_CLIPGRAD, TYPE_CONSTANT, TYPE_CONVOLUTIONINTEGER,TYPE_COS, TYPE_COSH, TYPE_CONVOLUTION,TYPE_CUMSUM, TYPE_CUMPROD,
    TYPE_DIVIDE,TYPE_DEPTHTOSPACE,TYPE_DEQUANTIZE,TYPE_DEQUANTIZELINEAR,TYPE_DIAGONALMATRIX,TYPE_DIFFERENCESQUARE,
    TYPE_ERF,TYPE_EXP,TYPE_EQUALS,
    TYPE_FLOOR,
	TYPE_GATHER,TYPE_GATHERELEMENTS,TYPE_GATHERND,TYPE_GEMM, TYPE_GREATERTHAN, TYPE_GREATERTHANOREQUAL,TYPE_GRU,
    TYPE_IDENTITY,TYPE_IF,TYPE_ISINFINITY,TYPE_ISNAN,
    TYPE_JOIN,
	TYPE_LAND, TYPE_LOR, TYPE_LXOR, TYPE_LNOT, TYPE_LOG, TYPE_LESSTHAN, TYPE_LESSTHANOREQUAL, TYPE_LOCALRESPONSENORMALIZATION,
    TYPE_MAX,TYPE_MAXPOOLING,TYPE_MEAN,TYPE_MEANVARIANCENORMALIZATION, TYPE_MIN,TYPE_MULTIPLY,TYPE_MODULUSFLOOR,TYPE_MODULUSTRUNCATE,
    TYPE_NEGATE,TYPE_NONZEROCOORDINATES,
    TYPE_ONEHOT,
    TYPE_PADDING,TYPE_POW,
    TYPE_QUANTIZELINEAR,TYPE_QUANTIZEDLINEARCONVOLUTION,
    TYPE_RANDOMGENERATOR,TYPE_REINTERPRET,TYPE_RECIP,TYPE_REDUCE,TYPE_RESAMPLE,TYPE_RESAMPLEGRAD, TYPE_REVERSESUBSEQUENCES,TYPE_ROUND,TYPE_ROIALIGN,TYPE_ROIALIGNGRAD,
    TYPE_SCATTERELEMENTS,TYPE_SLICE,TYPE_SLICEGRAD,TYPE_SUBTRACT,TYPE_SQRT,TYPE_SIGN,TYPE_SPACETODEPTH,
    TYPE_THRESHOLD,TYPE_TOPK,
    TYPE_UPSAMLPLE2D,
    TYPE_VALUESCALE2D,
    TYPE_OUTPUT = 999999
};

inline std::map<int, std::string> TypesToNames = {
    {TYPE_INPUT,"Input"},

    {TYPE_ACT_IDENTITY,"ActivationIdentity"},
    {TYPE_ACT_CELU,"ActivationCelu"},
    {TYPE_ACT_ELU,"ActivationElu"},
    {TYPE_ACT_GELU,"ActivationGelu"},
    {TYPE_ACT_HARDMAX,"ActivationHardmax"},
    {TYPE_ACT_HARDSIGMOID,"ActivationHardSigmoid"},
    {TYPE_ACT_LEAKYRELU,"ActivationLeakyRelu"},
    {TYPE_ACT_LINEAR,"ActivationLinear"},
    {TYPE_ACT_LOGSOFTMAX,"ActivationLogSoftmax"},
    {TYPE_ACT_PRELU,"ActivationParameterizedRelu"},
    {TYPE_ACT_PSOFTPLUS,"ActivationParametricSoftplus"},
    {TYPE_ACT_RELU,"ActivationRelu"},
    {TYPE_ACT_SELU,"ActivationScaledElu"},
    {TYPE_ACT_STANH,"ActivationScaledTanh"},
    {TYPE_ACT_SHRINK,"ActivationShrink"},
    {TYPE_ACT_SIGMOID,"ActivationSigmoid"},
    {TYPE_ACT_SOFTMAX,"ActivationSoftmax"},
    {TYPE_ACT_SOFTPLUS,"ActivationSoftplus"},
    {TYPE_ACT_SOFTSIGN,"ActivationSoftsign"},
    {TYPE_ACT_TANH,"ActivationTanh"},
    {TYPE_ACT_TRELU,"ActivationThresholdedRelu"},

    {TYPE_ABS,"Abs"},
    {TYPE_ACOS,"ACos"},
    {TYPE_ACOSH,"ACosh"},
    {TYPE_ADD,"Add"},
    {TYPE_ASIN,"ASin"},
    {TYPE_ASINH,"ASinh"},
    {TYPE_ATAN,"ATan"},
    {TYPE_ATANH,"ATanh"},
    {TYPE_ATANYX,"ATanYX"},
    {TYPE_AVERAGEPOOLING,"AveragePooling"},

    {TYPE_BITAND,"BitAnd"},
    {TYPE_BITCOUNT,"BitCount"},
    {TYPE_BITNOT,"BitNot"},
    {TYPE_BITOR,"BitOr"},
    {TYPE_BITSL,"BitSL"},
    {TYPE_BITSR,"BitSR"},
    {TYPE_BITXOR,"BitXor"},
    {TYPE_BATCHNORMALIZATION,"BatchNormalization"},
    {TYPE_BATCHNORMALIZATIONGRAD,"BatchNormalizationGrad"},
    {TYPE_BATCHNORMALIZATIONTRAINING,"BatchNormalizationTraining"},
    {TYPE_BATCHNORMALIZATIONTRAININGGRAD,"BatchNormalizationTrainingGrad"},
    {TYPE_CAST,"Cast"},
    {TYPE_CEIL,"Ceil"},
    {TYPE_CLIP,"Clip"},
    {TYPE_CLIPGRAD,"ClipGrad"},
    {TYPE_CONSTANT,"Constant"},
	{TYPE_CONVOLUTIONINTEGER,"ConvolutionInteger"},
    {TYPE_COS,"Cos"},
    {TYPE_COSH,"Cosh"},
    {TYPE_CONVOLUTION,"Convolution"},
    {TYPE_CUMSUM,"CummulativeSum"},
    {TYPE_CUMPROD,"CummulativeProduct"},
    {TYPE_DIVIDE,"Divide"},
    {TYPE_DEPTHTOSPACE,"DepthToSpace"},
    {TYPE_DEQUANTIZE,"Dequantize"},
    {TYPE_DEQUANTIZELINEAR,"DequantizeLinear"},
	{TYPE_DIAGONALMATRIX,"DiagonalMatrix"},
    {TYPE_DIFFERENCESQUARE,"DifferenceSquare"},

    {TYPE_ERF,"Erf"},
    {TYPE_EXP,"Exp"},
    {TYPE_EQUALS,"Equals"},
    {TYPE_FLOOR,"Floor"},
    {TYPE_GATHER,"Gather"},
    {TYPE_GATHERELEMENTS,"GatherElements"},
    {TYPE_GATHERND,"GatherND"},
    {TYPE_GEMM,"Gemm"},
    {TYPE_GREATERTHAN,"GreaterThan"},
    {TYPE_GREATERTHANOREQUAL,"GreaterThanOrEqual"},
	{TYPE_GRU,"Gru"},
    {TYPE_IDENTITY,"Identity"},
    {TYPE_IF,"If"},
    {TYPE_ISINFINITY,"IsInfinity"},
    {TYPE_ISNAN,"IsNan"},
    {TYPE_JOIN,"Join"},
    {TYPE_LAND,"And"},
    {TYPE_LOR,"Or"},
    {TYPE_LXOR,"Xor"},
    {TYPE_LNOT,"Not"},
    {TYPE_LOG,"Log"},
    {TYPE_LESSTHAN,"LessThan"},
    {TYPE_LESSTHANOREQUAL,"LessThanOrEqual"},
    {TYPE_LOCALRESPONSENORMALIZATION,"LocalResponseNormalization"},

    {TYPE_MAX,"Max"},
    {TYPE_MAXPOOLING,"MaxPooling"},
    {TYPE_MEAN,"Mean"},
    {TYPE_MEANVARIANCENORMALIZATION,"MeanVarianceNormalization"},
    {TYPE_MIN,"Min"},
    {TYPE_MULTIPLY,"Multiply"},
    {TYPE_MODULUSFLOOR,"ModulusFloor"},
    {TYPE_MODULUSTRUNCATE,"ModulusTruncate"},
    {TYPE_NEGATE,"Negate"},
    {TYPE_NONZEROCOORDINATES,"NonZeroCoordinates"},
    {TYPE_ONEHOT,"OneHot"},
	{TYPE_PADDING,"Padding"},
    {TYPE_POW,"Pow"},
    {TYPE_QUANTIZELINEAR,"QuantizeLinear"},
    {TYPE_QUANTIZEDLINEARCONVOLUTION,"QuantizedLinearConvolution"},

    {TYPE_RANDOMGENERATOR,"RandomGenerator"},
    {TYPE_REINTERPRET,"Reinterpret"},
    {TYPE_RECIP,"Recip"},
    {TYPE_REDUCE,"Reduce"},
    {TYPE_RESAMPLE,"Resample"},
    {TYPE_RESAMPLE,"ResampleGrad"},
	{TYPE_REVERSESUBSEQUENCES,"ReverseSubsequences" },
	{TYPE_ROUND,"Round"},
	{ TYPE_ROIALIGN,"RoiAlign" },
	{ TYPE_ROIALIGNGRAD,"RoiAlignGrad" },
	{TYPE_SCATTERELEMENTS,"ScatterElements" },
	{TYPE_SLICE,"Slice"},
	{TYPE_SLICEGRAD,"SliceGrad" },
    {TYPE_SUBTRACT,"Subtract"},
    {TYPE_SQRT,"Sqrt"},
	{TYPE_SIGN,"Sign"},
	{TYPE_SPACETODEPTH,"SpaceToDepth" },
	{TYPE_THRESHOLD,"Threshold"},
	{ TYPE_TOPK,"TopK" },
	{TYPE_UPSAMLPLE2D,"Upsample2D" },
	{TYPE_VALUESCALE2D,"ValueScale2D" },

	{TYPE_OUTPUT,"Output"}

};


struct XLNODE_ANY : public XLNODE
{
    int what = 0;
    int howi = 0;
    int howo = 1;

    std::any MultipleOpOutputData;

    virtual bool AsksType() {
        if (what == TYPE_INPUT || what == TYPE_CAST || what == TYPE_LESSTHAN || what == TYPE_LESSTHANOREQUAL || what == TYPE_GREATERTHAN || what == TYPE_GREATERTHANOREQUAL || what == TYPE_REINTERPRET || what == TYPE_ISINFINITY || what == TYPE_ISNAN || what == TYPE_REDUCE || what == TYPE_QUANTIZELINEAR || what == TYPE_QUANTIZEDLINEARCONVOLUTION)
            return true;
        return false;
    }

    virtual bool IsInput() 
    { 
        if (what == TYPE_CONSTANT || what == TYPE_INPUT || what == TYPE_DIAGONALMATRIX)
            return true;
        return false; 
    }

    virtual bool IsOutput() { 
        if (what == TYPE_OUTPUT)
            return true; 
        return false;
    }

    virtual int ninreq() 
    {
        if (what == TYPE_GEMM || what == TYPE_CONVOLUTION)
            return nin() - 1;
        if (what == TYPE_JOIN)
            return 1;
        if (what == TYPE_CONVOLUTIONINTEGER)
            return 2;
        if (what == TYPE_DEQUANTIZE)
            return 2;
        if (what == TYPE_QUANTIZEDLINEARCONVOLUTION)
            return 5;
        if (what == TYPE_MEANVARIANCENORMALIZATION)
            return 1;
        if (what == TYPE_ROIALIGNGRAD)
            return 3;
		if (what == TYPE_GRU)
			return 3;
        return nin();
    }
    virtual int nin() { return howi; }
    virtual int nout() { return howo; }


    void LoadNames()
    {
        std::vector<std::wstring> Names = GetNames(what);
        for (int i = 0; i < children.size(); i++)
        {
            auto& bu = children[i];
            if (Names.size() > i)
                bu.name = Names[i];
        }
    }

    XLNODE_ANY(int NI, int w, int NO = 1)
    {
        howi = NI;
        howo = NO;
        for (int i = 0; i < NI; i++)
        {
            XLNODEBULLET bu;
            bu.O = 0;
            children.push_back(bu);
        }

        for (int i = 0; i < NO; i++)
        {
            XLNODEBULLET bu;
            bu.O = 1;
            children.push_back(bu);
        }
        what = w;
        LoadNames();
    }

    std::vector<std::wstring> GetNames(int w)
    {
		std::vector<std::wstring> dr;
        if (howi <= 1 && howo <= 1)
            return dr;

        if (w == TYPE_BATCHNORMALIZATION)
        {
			return { L"Input",L"Mean",L"Variance",L"Scale",L"Bias"};
        }
        if (w == TYPE_BATCHNORMALIZATIONGRAD)
        {
            return { L"Input",L"Input Gradient",L"Mean",L"Variance",L"Scale",L"Gradient",L"Bias Gradient",L"Scale Gradient"};
        }
        if (w == TYPE_BATCHNORMALIZATIONTRAINING)
        {
            return { L"Input",L"Scale",L"Bias",L"Output",L"Mean",L"Variance"};
        }
        if (w == TYPE_BATCHNORMALIZATIONTRAININGGRAD)
        {
            return { L"Input",L"Input Gradient",L"Mean",L"Variance",L"Scale",L"Gradient",L"Bias Gradient",L"Scale Gradient" };
        }
        if (w == TYPE_CLIPGRAD)
        {
            return { L"Input",L"Input Gradient"};
        }
		if (w == TYPE_CONVOLUTION)
		{
			return { L"Input",L"Filter",L"Bias" };
		}
        if (w == TYPE_CONVOLUTIONINTEGER)
        {
            return { L"Input",L"Filter",L"Input ZP",L"Filter ZP"};
        }
        if (w == TYPE_DEQUANTIZELINEAR)
		{
			return { L"Input",L"Scale",L"Zero Point"};
		}
		if (w == TYPE_GATHER)
		{
			return { L"Input",L"Indices" };
		}
		if (w == TYPE_GATHERELEMENTS)
		{
			return { L"Input",L"Indices" };
		}
		if (w == TYPE_GATHERND)
		{
			return { L"Input",L"Indices" };
		}
        if (w == TYPE_GEMM)
        {
            return { L"Matrix 1",L"Matrix 2",L"Optional C" };
        }
        if (w == TYPE_GRU)
        {
			return { L"Input",L"Weights",L"Recurrence",L"Bias",L"Hidden Init",L"Sequence Lengths" };
        }
        if (w == TYPE_MAXPOOLING)
        {
            return { L"Input",L"Values",L"Indices" };
        }
		if (w == TYPE_MEANVARIANCENORMALIZATION)
		{
            return { L"Input",L"Scale",L"Bias" };
		}
        if (w == TYPE_NONZEROCOORDINATES)
        {
            return { L"Input",L"Count",L"Coordinates" };
        }
        if (w == TYPE_ONEHOT)
        {
            return { L"Indices",L"Values" };
        }
        if (w == TYPE_QUANTIZELINEAR)
        {
            return { L"Input",L"Scale",L"Zero Point" };
        }
		if (w == TYPE_QUANTIZEDLINEARCONVOLUTION)
		{
			return { L"Input",L"Scale",L"Filter",L"Filter Scale",L"Output Scale",L"Input ZP",L"Filter ZP",L"Output ZP"};
		}
        if (w == TYPE_RANDOMGENERATOR)
        {
            return { L"Input",L"Values",L"State"};
        }
		if (w == TYPE_ROIALIGN)
		{
            return { L"Input",L"Roi",L"Batch Indices" };
		}
        if (w == TYPE_ROIALIGNGRAD)
        {
            return { L"Input",L"Gradient",L"Roi",L"Batch Indices",L"Gradient",L"ROI Gradient"};
        }

		if (w == TYPE_REVERSESUBSEQUENCES)
		{
			return { L"Input",L"Sequence Lengths" };
		}
        if (w == TYPE_SCATTERELEMENTS)
        {
            return { L"Input",L"Indices",L"Updates"};
        }
        if (w == TYPE_TOPK)
        {
            return { L"Input",L"Value",L"Index" };
        }

        return dr;
    }

    virtual std::wstring opname()
    {
        if (what == TYPE_INPUT)
            return L"Input";
		if (what == TYPE_ACT_IDENTITY)
			return L"ActivationIdentity";
		if (what == TYPE_ACT_CELU)
			return L"ActivationCelu";
		if (what == TYPE_ACT_ELU)
			return L"ActivationElu";
        if (what == TYPE_ACT_GELU)
            return L"ActivationGelu";
		if (what == TYPE_ACT_HARDMAX)
			return L"ActivationHardmax";
		if (what == TYPE_ACT_HARDSIGMOID)
			return L"ActivationHardSigmoid";
		if (what == TYPE_ACT_LEAKYRELU)
			return L"ActivationLeakyRelu";
		if (what == TYPE_ACT_LINEAR)
			return L"ActivationLinear";
		if (what == TYPE_ACT_LOGSOFTMAX)
			return L"ActivationLogSoftmax";
		if (what == TYPE_ACT_PRELU)
			return L"ActivationParameterizedRelu";
		if (what == TYPE_ACT_PSOFTPLUS)
			return L"ActivationParametricSoftplus";
		if (what == TYPE_ACT_RELU)
			return L"ActivationRelu";
		if (what == TYPE_ACT_SELU)
			return L"ActivationScaledElu";
		if (what == TYPE_ACT_STANH)
			return L"ActivationScaledTanh";
		if (what == TYPE_ACT_SHRINK)
			return L"ActivationShrink";
		if (what == TYPE_ACT_SIGMOID)
			return L"ActivationSigmoid";
		if (what == TYPE_ACT_SOFTMAX)
			return L"ActivationSoftmax";
		if (what == TYPE_ACT_SOFTPLUS)
			return L"ActivationSoftplus";
		if (what == TYPE_ACT_SOFTSIGN)
			return L"ActivationSoftsign";
		if (what == TYPE_ACT_TANH)
			return L"ActivationTanh";
		if (what == TYPE_ACT_TRELU)
			return L"ActivationThresholdedRelu";


        if (what == TYPE_ABS)
			return L"Abs";
        if (what == TYPE_ACOS)
            return L"ACos";
        if (what == TYPE_ACOSH)
            return L"ACosh";
        if (what == TYPE_ADD)
            return L"Add";
        if (what == TYPE_ASIN)
            return L"ASin";
        if (what == TYPE_ASINH)
            return L"ASinh";
        if (what == TYPE_ATAN)
            return L"ATan";
        if (what == TYPE_ATANH)
            return L"ATanh";
        if (what == TYPE_ATANYX)
            return L"ATanYX";
		if (what == TYPE_AVERAGEPOOLING)
			return L"AveragePooling";



        if (what == TYPE_BITAND)
            return L"BitAnd";
        if (what == TYPE_BITCOUNT)
            return L"BitCount";
        if (what == TYPE_BITNOT)
            return L"BitNot";
        if (what == TYPE_BITOR)
            return L"BitOr";
		if (what == TYPE_BITSL)
			return L"BitSL";
		if (what == TYPE_BITSR)
			return L"BitSR";
		if (what == TYPE_BITXOR)
			return L"BitXor";
		if (what == TYPE_BATCHNORMALIZATION)
			return L"BatchNormalization";
		if (what == TYPE_BATCHNORMALIZATIONGRAD)
			return L"BatchNormalizationGrad";
		if (what == TYPE_BATCHNORMALIZATIONTRAINING)
			return L"BatchNormalizationTraining";
		if (what == TYPE_BATCHNORMALIZATIONTRAININGGRAD)
			return L"BatchNormalizationTrainingGrad";

		if (what == TYPE_CAST)
			return L"Cast";
        if (what == TYPE_CEIL)
            return L"Ceil";
        if (what == TYPE_CLIP)
            return L"Clip";
		if (what == TYPE_CLIPGRAD)
			return L"ClipGrad";

        if (what == TYPE_CONSTANT)
            return L"Constant";
		if (what == TYPE_CONVOLUTIONINTEGER)
			return L"ConvolutionInteger";
        if (what == TYPE_COS)
            return L"Cos";
        if (what == TYPE_COSH)
            return L"Cosh";
		if (what == TYPE_CONVOLUTION)
			return L"Convolution";
		if (what == TYPE_CUMSUM)
			return L"CummulativeSum";
		if (what == TYPE_CUMPROD)
			return L"CummulativeProduct";

		if (what == TYPE_DIVIDE)
			return L"Divide";
		if (what == TYPE_DEPTHTOSPACE)
			return L"DepthToSpace";
		if (what == TYPE_DEQUANTIZE)
			return L"Dequantize";
		if (what == TYPE_DEQUANTIZELINEAR)
			return L"DequantizeLinear";
		if (what == TYPE_DIAGONALMATRIX)
			return L"DiagonalMatrix";
		if (what == TYPE_DIFFERENCESQUARE)
			return L"DifferenceSquare";


		if (what == TYPE_ERF)
			return L"Erf";
		if (what == TYPE_EXP)
			return L"Exp";
		if (what == TYPE_EQUALS)
			return L"Equals";

		if (what == TYPE_FLOOR)
			return L"Floor";

		if (what == TYPE_GATHER)
			return L"Gather";
		if (what == TYPE_GATHERELEMENTS)
			return L"GatherElements";
		if (what == TYPE_GATHERND)
			return L"GatherND";
		if (what == TYPE_GEMM)
			return L"Gemm";
		if (what == TYPE_GREATERTHAN)
			return L"GreaterThan";
		if (what == TYPE_GREATERTHANOREQUAL)
			return L"GreaterThanOrEqual";
		if (what == TYPE_GRU)
			return L"Gru";

            
		if (what == TYPE_IDENTITY)
			return L"Identity";
		if (what == TYPE_IF)
			return L"If";
		if (what == TYPE_ISINFINITY)
			return L"IsInfinity";
		if (what == TYPE_ISNAN)
			return L"IsNan";

		if (what == TYPE_JOIN)
			return L"Join";

        if (what == TYPE_LAND)
            return L"And";
        if (what == TYPE_LOR)
            return L"Or";
        if (what == TYPE_LXOR)
            return L"Xor";
        if (what == TYPE_LNOT)
            return L"Not";
        if (what == TYPE_LOG)
            return L"Log";
		if (what == TYPE_LESSTHAN)
			return L"LessThan";
		if (what == TYPE_LESSTHANOREQUAL)
			return L"LessThanOrEqual";
		if (what == TYPE_LOCALRESPONSENORMALIZATION)
			return L"LocalResponseNormalization";


        if (what == TYPE_MAX)
            return L"Max";
        if (what == TYPE_MIN)
            return L"Min";
		if (what == TYPE_MAXPOOLING)
			return L"MaxPooling";
        if (what == TYPE_MEAN)
            return L"Mean";
		if (what == TYPE_MEANVARIANCENORMALIZATION)
			return L"MeanVarianceNormalization";

		if (what == TYPE_MULTIPLY)
			return L"Multiply";
		if (what == TYPE_MODULUSFLOOR)
			return L"ModulusFloor";
		if (what == TYPE_MODULUSTRUNCATE)
			return L"ModulusTruncate";

        if (what == TYPE_NEGATE)
            return L"Neg";
		if (what == TYPE_NONZEROCOORDINATES)
			return L"NonZeroCoordinates";   

		if (what == TYPE_ONEHOT)
			return L"OneHot";

		if (what == TYPE_PADDING)
			return L"Padding";
        if (what == TYPE_POW)
            return L"Pow";

		if (what == TYPE_QUANTIZELINEAR)
			return L"QuantizeLinear";
		if (what == TYPE_QUANTIZEDLINEARCONVOLUTION)
			return L"QuantizedLinearConvolution";


        if (what == TYPE_RANDOMGENERATOR)
            return L"RandomGenerator";
		if (what == TYPE_REINTERPRET)
			return L"Reinterpret";
		if (what == TYPE_ROUND)
			return L"Round";
		if (what == TYPE_RESAMPLE)
			return L"Resample";
        if (what == TYPE_RESAMPLEGRAD)
            return L"ResampleGrad";
		if (what == TYPE_REVERSESUBSEQUENCES)
			return L"ReverseSubsequences";
		if (what == TYPE_REDUCE)
			return L"Reduce";
		if (what == TYPE_RECIP)
			return L"Recip";
		if (what == TYPE_ROIALIGN)
			return L"RoiAlign";
		if (what == TYPE_ROIALIGNGRAD)
			return L"RoiAlignGrad";

		if (what == TYPE_SCATTERELEMENTS)
			return L"ScatterElements";
		if (what == TYPE_SLICE)
			return L"Slice";
		if (what == TYPE_SLICEGRAD)
			return L"SliceGrad";
		if (what == TYPE_SUBTRACT)
			return L"Subtract";
		if (what == TYPE_SPACETODEPTH)
			return L"SpaceToDepth"; 
        if (what == TYPE_SQRT)
            return L"Sqrt";

		if (what == TYPE_SIGN)
			return L"Sign";

		if (what == TYPE_THRESHOLD)
			return L"Threshold";
		if (what == TYPE_TOPK)
			return L"TopK";
		if (what == TYPE_UPSAMLPLE2D)
			return L"Upsample2D";
		if (what == TYPE_VALUESCALE2D)
			return L"ValueScale2D";

        if (what == TYPE_OUTPUT)
            return L"Output";

        return L"Unknown";
    }


    std::vector<std::string> tensor_dims2; // for inputs

	void SetTensorDims(std::vector<unsigned int> d)
	{
		tensor_dims2.clear();
		for (auto& s : d)
		{
			tensor_dims2.push_back(std::to_string(s));
		}
	}

    std::vector<unsigned int> tensor_dims()
    {
        std::vector<unsigned int> d;
		for (auto& s : tensor_dims2)
		{
			d.push_back(std::stoi(s));
		}
		return d;
    }

    virtual std::wstring subname()
    {
        std::wstring n;
        wchar_t t[1000] = {};
        if (csv_input.length())
        {
            n += L"\r\n";
            n += csv_input;
        }
        if (csv_output.length())
        {
            n += L"\r\n";
            n += csv_output;
        }
        for (auto& p : Params)
        {
            if (p.list_names.size() && (int)p < p.list_names.size())
            {
                swprintf_s(t, 1000, L"\r\n%s: %s", p.n.c_str(), p.list_names[p].c_str());
            }
            else
            {
                bool N = 0;
                try
                {
                    [[maybe_unused]] float j = std::stof(p.w());
                    N = 1;
                }
                catch (...)
                {

                }
                if (N == 0 || (p.minv <= -1 && p.maxv <= -1))
                {
                    swprintf_s(t, 1000, L"\r\n%s: %s", p.n.c_str(), p.w());
                }
                else
                    if (p.minv == 0 && p.maxv == 1)
                    {
                        swprintf_s(t, 1000, L"\r\n%s: %s", p.n.c_str(), p ? L"True" : L"False");
                    }
                    else
                        swprintf_s(t, 1000, L"\r\n%s: %.2f", p.n.c_str(), (float)p);
            }
            n += t;
        }
        for (auto& v : VariableChanges)
        {
			n += L"\r\n";
			n += v.str();
        }
        return n;
    }

    virtual std::wstring name()
    {
        auto n = opname();
        if (AsksType())
        {
            n += L"\r\n";
            n += optypes[OpType];
        }
        if (IsInput())
        {
			n += L"\r\n";
			n += TensorToString(tensor_dims(),L"x");
        }
        return n;
    }

    std::vector<VARIABLECHANGE> VariableChanges;
	JUMP Jump;


    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE::Ser(ee);
        ee.vv("ni").SetValueInt(nin());
        ee.vv("no").SetValueInt(nout());
        ee.vv("Type").SetValueInt(what);
		ee.vv("Name").SetWideValue(opname().c_str());
		for (auto& p : Params)
		{
			auto& pe = ee["Params"].AddElement("Param");
			pe.vv("Name").SetWideValue(p.n.c_str());
            pe.vv("Value").SetWideValue(p.w());
            pe.vv("Min").SetValueFloat(p.minv);
            pe.vv("Max").SetValueFloat(p.maxv);
			for (auto& s : p.list_names)
				pe["list"].AddElement("l").vv("n").SetWideValue(s.c_str());
		}

        for (auto& d : tensor_dims2)
        {
            auto& de = ee["Dimensions"].AddElement("Dimension");
            de.vv("Size").SetValue(d);
        }
		for (auto& vc : VariableChanges)
		{
			auto& vce = ee["VariableChanges"].AddElement("VariableChange");
			vc.Ser(vce);
		}
        if (Jump.iNodeJumpTo)
        {
			auto& je = ee["Jump"];
			je.vv("iNodeJumpTo").SetValueULongLong(Jump.iNodeJumpTo);
            for (auto& co : Jump.conditions)
            {
				co.Ser(je["Conditions"].AddElement("Condition"));   
            }
        }
    }

    virtual void Unser(XML3::XMLElement& e)
    {
        XLNODE::Unser(e);
        Params.clear();
        for (auto& pe : e["Params"])
        {
            PARAM p;
			p.n = pe.vv("Name").GetWideValue();
			p.v = pe.vv("Value").GetWideValue();
			p.minv = pe.vv("Min").GetValueFloat(std::numeric_limits<float>::min());
			p.maxv = pe.vv("Max").GetValueFloat(std::numeric_limits<float>::max());
			for (auto& le : pe["list"])
				p.list_names.push_back(le.vv("n").GetWideValue());
			Params.push_back(p);
        }

        auto& ee = e["Dimensions"];
        for (size_t i = 0; i < ee.GetChildrenNum(); i++)
        {
            auto& de = ee.GetChildren()[i];
            tensor_dims2.push_back(de->vv("Size").GetValue());
        }

		VariableChanges.clear();
		for (auto& ve : e["VariableChanges"])
		{
			VARIABLECHANGE vc;
			vc.Unser(ve);
			VariableChanges.push_back(vc);
		}

		auto& je = e["Jump"];
		Jump.iNodeJumpTo = je.vv("iNodeJumpTo").GetValueULongLong();
		Jump.conditions.clear();
        for (auto& ce : je["Conditions"])
        {
            CONDITION co;
			co.Unser(ce);
			Jump.conditions.push_back(co);
        }


        LoadNames();
    }
};



struct XLOP : public XLNODE
{
    bool Active = 1;
    bool Visible = 1;
    float Zoom = 1.0f;
    std::vector<std::shared_ptr<XLNODE>> nodes;
	DML_TENSOR_DATA_TYPE data_type = DML_TENSOR_DATA_TYPE_FLOAT32;

    virtual std::wstring name() { return L"Operator"; }
    virtual std::wstring subname() { return L""; }

    virtual void Ser(XML3::XMLElement& ee)
    {
		ee.vv("DataType").SetValueInt((int)data_type);
		ee.vv("Zoom").SetValueFloat(Zoom);
        ee.vv("Active").SetValueInt(Active);
		for (auto& n : nodes)
		{
			auto& ne = ee["Nodes"].AddElement("Node");
			n->Ser(ne);
		}
    }

    std::shared_ptr<XLNODE> Unser2(XML3::XMLElement& ne)
    {
        int nty = ne.vv("Type").GetValueInt();
        auto namety = ne.vv("Name").GetValue();

        bool F = 0;
        for (auto& nt : TypesToNames)
        {
            if (nt.second == namety)
            {
                F = 1;
                if (nty != nt.first)
                    nty = nt.first;
                break;
            }
        }


        if (nty == TYPE_INPUT)
        {
            auto n = std::make_shared<XLNODE_ANY>(0,TYPE_INPUT,1);
            n->Unser(ne);
            return n;
        }
        else
        if (nty == TYPE_CONSTANT)
        {
            auto n = std::make_shared<XLNODE_ANY>(0, TYPE_CONSTANT,1);
            n->Unser(ne);
            return n;
        }
        else
        if (nty == TYPE_DIAGONALMATRIX)
        {
            auto n = std::make_shared<XLNODE_ANY>(0, TYPE_DIAGONALMATRIX,1);
            n->Unser(ne);
            return n;
        }
        else
        if (nty == TYPE_OUTPUT)
        {
            auto n = std::make_shared<XLNODE_ANY>(1,TYPE_OUTPUT,0);
            n->Unser(ne);
            return n;
        }
        else
        {
            int ni = ne.vv("ni").GetValueInt();
            if (ni == 0)
                ni = 1;
            int no = ne.vv("no").GetValueInt(1);
            if (no == 0)
                no = 1;
            auto n = std::make_shared<XLNODE_ANY>(ni, nty,no);
            n->Unser(ne);
            return n;
        }
    }

	virtual void Unser(XML3::XMLElement& e)
	{
        nodes.clear();
		for (size_t i = 0; i < e["Nodes"].GetChildrenNum(); i++)
		{
			auto& ne = e["Nodes"].GetChildren()[i];
			auto n = Unser2(*ne);
			nodes.push_back(n);
		}
		data_type = (DML_TENSOR_DATA_TYPE)e.vv("DataType").GetValueInt();
		Zoom = e.vv("Zoom").GetValueFloat(1.0f);
		Active = e.vv("Active").GetValueInt(1);
	}
};

struct XL : public XLNODE
{
    std::wstring n;
    std::shared_ptr<ML> ml;
    std::shared_ptr<std::thread> running;

    std::vector<XLOP> ops;
    std::vector<VARIABLE> variables;
    virtual std::wstring name() { return L"DML"; }
    virtual std::wstring subname() { return L""; }


    std::string GenerateCode();

    virtual void Ser(XML3::XMLElement& ee)
    {
		ee.vv("n").SetWideValue(n.c_str());
		for (auto& op : ops)
		{
			auto& oe = ee["Operators"].AddElement("Operator");
			op.Ser(oe);
		}

        for (auto& v : variables)
        {
            auto& nv = ee["Variables"].AddElement("Variable");
            v.Ser(nv);
        }

    }
    virtual void Unser(XML3::XMLElement& e)
    {
		n = e.vv("n").GetWideValue();
        ops.clear();
        for (size_t i = 0; i < e["Operators"].GetChildrenNum(); i++)
        {
            auto& oe = e["Operators"].GetChildren()[i];
            XLOP op;
            op.Unser(*oe);
            ops.push_back(op);
        }

        variables.clear();
        for (auto& ve : e["Variables"])
        {
            VARIABLE v;
            v.Unser(ve);
            variables.push_back(v);
        }



		for (auto& op : ops)
		{
			for (auto& no : op.nodes)
			{
				for (auto& c : no->children)
				{
                    for(auto& gg : c.g)
                        nnn = std::max((unsigned long long)abs(no->ShareMemory),std::max(nnn + 1,gg.key + 1));
                }
			}
		}

    }
};

struct PROJECT
{
	std::vector<XL> xls;
    size_t iActive = 0;
    XL& xl()
    {
        if (xls.empty())
        {
            xls.push_back(XL());
            return xls[0];
        }
		if (iActive < xls.size())
			return xls[iActive]; 
        return xls[0];
    }

	void Ser(XML3::XMLElement& e, int idx = -1)
	{
		for (size_t ii = 0 ; ii < xls.size() ; ii++)
		{
            auto& xl = xls[ii];
			if (idx >= 0 && idx != ii)
				continue;
			auto& xe = e["Pages"].AddElement("Page");
			xl.Ser(xe);
		}
	}

	void Unser(XML3::XMLElement& e)
	{
		xls.clear();
		for (size_t i = 0; i < e["Pages"].GetChildrenNum(); i++)
		{
			auto& xe = e["Pages"].GetChildren()[i];
			XL xl;
			xl.Unser(*xe);
			xls.push_back(xl);
		}
	}
};




namespace winrt::VisualDML::implementation
{
    struct MLGraph : MLGraphT<MLGraph>
    {
        winrt::event_token PropertyChanged(winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
        {
            return m_propertyChanged.add(handler);
        }
        void PropertyChanged(winrt::event_token const& token) noexcept
        {
            m_propertyChanged.remove(token);
        }
        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        void Refresh(std::vector<std::wstring> strs);
        void Refresh(const wchar_t* s = L"");
        void FullRefresh();

        PROJECT prj;
        std::stack<XML3::XMLElement> undo_list;
        std::stack<XML3::XMLElement> redo_list;
        size_t ActiveOperator2 = (size_t)-1;
        MLGraph()
        {
        }



        long long _wnd = 0;
        long long wnd()
        {
            return _wnd;
        }
        void wnd(long long value)
        {
            _wnd = value;
        }


        std::wstring _i1;
		winrt::hstring i1()
		{
			return _i1.c_str();
		}   
        void i1(winrt::hstring const& value)
        {
			if (_i1 != value)
			{
				_i1 = value;
				m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"i1"));
			}
        }


        std::wstring _i0;
        winrt::hstring i0()
        {
            return _i0.c_str();
        }
        void i0(winrt::hstring const& value)
        {
            if (_i0 != value)
            {
                _i0 = value;
                m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"i0"));
            }
        }


        void Input_Completed(IInspectable, IInspectable);
        void Input2_Completed(IInspectable, IInspectable);

        void Push();
        void Undo();
        void Unselect();
        void Key(long long k,bool FromCmd = 0);
        void RefreshMenu();

        void OnExample(const char*);
        void OnExample1(IInspectable, IInspectable);
        void OnExample2(IInspectable, IInspectable);
        void OnExample3(IInspectable, IInspectable);

        void OnLoaded(IInspectable, IInspectable);
        void OnCopy(IInspectable const&, IInspectable const&);
        void OnPaste(IInspectable const&, IInspectable const&);
        void OnDelete(IInspectable const&, IInspectable const&);
        void OnUndo(IInspectable const&, IInspectable const&);
        void OnRedo(IInspectable const&, IInspectable const&);
        void OnClean(IInspectable const&, IInspectable const&);
        void OnGC(IInspectable const&, IInspectable const&);
        void OnGCV(IInspectable const&, IInspectable const&);
        void OnCompile(IInspectable const&, IInspectable const&);
        void OnRun(IInspectable const&, IInspectable const&);
        void OnStop(IInspectable const&, IInspectable const&);
        void Run();
        void Stop();
        HRESULT Compile();
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyout BuildNodeRightMenu(XL& xl, std::shared_ptr<XLNODE> nd, int Type, std::function<void(const winrt::Windows::Foundation::IInspectable, const winrt::Windows::Foundation::IInspectable)> fooo);
        void Clean();
        void Dirty(bool Cl = 1);
        void OnAddOp(IInspectable const&, IInspectable const&);
        void OnAddSet(IInspectable const&, IInspectable const&);
        XL ADefXL();

        void Tip(const wchar_t*);
        void OnAddVariable(IInspectable const&, IInspectable const&);
        void OnAddInput(IInspectable const&, IInspectable const&);
        void OnAddConstant(IInspectable const&, IInspectable const&);
        void OnAddOutput(IInspectable const&, IInspectable const&);
        void OnNew(IInspectable const&, IInspectable const&);
        void OnOpen(IInspectable const&, IInspectable const&);
        void OnSave(IInspectable const&, IInspectable const&);
        void OnSaveSet(IInspectable const&, IInspectable const&);
        void OnExit(IInspectable const&, IInspectable const&);
        void OnSaveAs(IInspectable const&, IInspectable const&);
        void Import(XML3::XMLElement& e);
        void Export(XML3::XMLElement& e,int idx = -1);
        std::wstring current_file;
        void Paint();
        void Resize();
        void Finished();
        void LoadAdapters();

        DXGI_QUERY_VIDEO_MEMORY_INFO vmi = {};
        DXGI_ADAPTER_DESC1 vdesc = {};
        void UpdateVideoMemory();

    };
}

namespace winrt::VisualDML::factory_implementation
{
    struct MLGraph : MLGraphT<MLGraph, implementation::MLGraph>
    {
    };
}
