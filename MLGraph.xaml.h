#pragma once

#include "MLGraph.g.h"

struct XLNODE;
inline static unsigned long long nnn = 1;
inline unsigned long long nextn()
{
	return nnn++;
}

struct XLNODEBULLET
{
    D2F hit;
    bool S = 0;
    size_t i = 0;
    bool O = 0;
    std::vector<unsigned long long> g;
};

struct PARAM
{
    std::wstring n;
    float v = 0;
    float minv = std::numeric_limits<float>::min();
    float maxv = std::numeric_limits<float>::max();
};

struct XLNODE
{

    int tidx = -1; // idx in MLOP
    bool BufferVisible = 0;
    D2F bhit = { };
    D2F bhit2 = { };


    signed long long ShareMemory = 0;
    bool bSelected = 0;
    std::vector<PARAM> Params;


    std::wstring csv_input;
    std::wstring csv_output;
    std::vector<XLNODEBULLET> children;

    virtual bool IsInput() { return false; }
    virtual bool IsOutput() { return false; }
    virtual void Ser(XML3::XMLElement& e);
    virtual void Unser(XML3::XMLElement& e);
    bool S = 0;
    virtual std::wstring name() = 0;
    virtual std::wstring subname() = 0;
    D2F hit = {};

    virtual int nin() { return 0; }
    virtual int nout() { return 0; }
    virtual void Draw(MLOP*mlop,bool Active,ID2D1DeviceContext5* r, D2D* d2d,size_t iop);

};

enum XLNODE_TYPE
{
    TYPE_INPUT = 1,
    TYPE_ABS,TYPE_ACOS,TYPE_ACOSH, TYPE_ADD,TYPE_ASIN,TYPE_ASINH,TYPE_ATAN,TYPE_ATANH, TYPE_ATANYX,

    TYPE_BITAND,TYPE_BITCOUNT,TYPE_BITNOT,TYPE_BITOR,TYPE_BITSL,TYPE_BITSR,TYPE_BITXOR,
	TYPE_CEIL, TYPE_CLIP, TYPE_CONSTANT, TYPE_COS, TYPE_COSH, TYPE_CONVOLUTION,TYPE_CUMSUM, TYPE_CUMPROD,
    TYPE_DIVIDE,
    TYPE_ERF,TYPE_EXP,
    TYPE_FLOOR,
    TYPE_GEMM,
    TYPE_IDENTITY,
	TYPE_LAND, TYPE_LOR, TYPE_LXOR,TYPE_LNOT,TYPE_LOG,
    TYPE_MAX,TYPE_MEAN,TYPE_MIN,TYPE_MULTIPLY,
    TYPE_NEGATE,
    TYPE_POW,
    TYPE_ROUND,
    TYPE_SUBTRACT,TYPE_SQRT,TYPE_SIGN,
    TYPE_THRESHOLD,
    TYPE_OUTPUT = 999999
};

inline std::map<int, std::string> TypesToNames = {
	{TYPE_INPUT,"Input"},
	{TYPE_ABS,"Abs"},
	{TYPE_ACOS,"ACos"},
	{TYPE_ACOSH,"ACosh"},
	{TYPE_ADD,"Add"},
	{TYPE_ASIN,"ASin"},
	{TYPE_ASINH,"ASinh"},
	{TYPE_ATAN,"ATan"},
	{TYPE_ATANH,"ATanh"},
	{TYPE_ATANYX,"ATanYX"},
	{TYPE_BITAND,"BitAnd"},
	{TYPE_BITCOUNT,"BitCount"},
	{TYPE_BITNOT,"BitNot"},
	{TYPE_BITOR,"BitOr"},
	{TYPE_BITSL,"BitSL"},
	{TYPE_BITSR,"BitSR"},
	{TYPE_BITXOR,"BitXor"},
	{TYPE_CEIL,"Ceil"},
	{TYPE_CLIP,"Clip"},
	{TYPE_CONSTANT,"Constant"},
	{TYPE_COS,"Cos"},
	{TYPE_COSH,"Cosh"},
	{TYPE_CONVOLUTION,"Convolution"},
	{TYPE_CUMSUM,"CummulativeSum"},
	{TYPE_CUMPROD,"CummulativeProduct"},
	{TYPE_DIVIDE,"Divide"},
	{TYPE_ERF,"Erf"},
	{TYPE_EXP,"Exp"},
	{TYPE_FLOOR,"Floor"},
	{TYPE_GEMM,"Gemm"},
	{TYPE_IDENTITY,"Identity"},
	{TYPE_LAND,"And"},
	{TYPE_LOR,"Or"},
	{TYPE_LXOR,"Xor"},
    {TYPE_LNOT,"Not"},
    {TYPE_LOG,"Log"},
    {TYPE_MAX,"Max"},
    {TYPE_MEAN,"Mean"},
    {TYPE_MIN,"Min"},
    {TYPE_MULTIPLY,"Multiply"},
	{TYPE_NEGATE,"Negate"},
    {TYPE_POW,"Pow"},
	{TYPE_ROUND,"Round"},
    {TYPE_SUBTRACT,"Subtract"},
    {TYPE_SQRT,"Sqrt"},
	{TYPE_SIGN,"Sign"},
	{TYPE_THRESHOLD,"Threshold"},
	{TYPE_OUTPUT,"Output"}

};


struct XLNODE_ANY : public XLNODE
{
    int what = 0;
    int howi = 0;


    virtual int ninreq() 
    {
        if (what == TYPE_GEMM || what == TYPE_CONVOLUTION)
            return nin() - 1;
        return nin(); 
    }
    virtual int nin() { return howi; }
    virtual int nout() { return 1; }


    XLNODE_ANY(int NI,int w)
    {
        howi = NI;
        for (int i = 0; i < NI; i++)
        {
            XLNODEBULLET bu;
            bu.O = 0;
            children.push_back(bu);
        }

        XLNODEBULLET bu;
        bu.O = 1;
        children.push_back(bu);

        what = w;
    }

    virtual std::wstring opname()
    {
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

        if (what == TYPE_CEIL)
            return L"Ceil";
        if (what == TYPE_CLIP)
            return L"Clip";
        if (what == TYPE_CONSTANT)
            return L"Constant";
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

		if (what == TYPE_ERF)
			return L"Erf";
		if (what == TYPE_EXP)
			return L"Exp";

		if (what == TYPE_FLOOR)
			return L"Floor";

		if (what == TYPE_GEMM)
			return L"Gemm";

		if (what == TYPE_IDENTITY)
			return L"Identity";

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

        if (what == TYPE_MAX)
            return L"Max";
        if (what == TYPE_MIN)
            return L"Min";
        if (what == TYPE_MEAN)
            return L"Mean";

		if (what == TYPE_MULTIPLY)
			return L"Multiply";

        if (what == TYPE_NEGATE)
            return L"Neg";

        if (what == TYPE_POW)
            return L"Pow";

		if (what == TYPE_ROUND)
			return L"Round";

		if (what == TYPE_SUBTRACT)
			return L"Subtract";
        if (what == TYPE_SQRT)
            return L"Sqrt";

		if (what == TYPE_SIGN)
			return L"Sign";

		if (what == TYPE_THRESHOLD)
			return L"Threshold";


        return L"Unknown";
    }


    virtual std::wstring subname()
    {
        std::wstring n;
        wchar_t t[1000] = {};
        if (csv_output.length())
        {
            n += csv_output;
        }
        for (auto& p : Params)
        {
            if (p.minv == 0 && p.maxv == 1)
            {
                swprintf_s(t, 1000, L"\r\n%s: %s", p.n.c_str(), p.v == 1 ? L"True" : L"False");
            }
            else
                swprintf_s(t, 1000, L"\r\n%s: %.2f", p.n.c_str(), p.v);
            n += t;
        }
        return n;
    }

    virtual std::wstring name()
    {
        auto n = opname();
        return n;
    }



    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE::Ser(ee);
        ee.vv("ni").SetValueInt(nin());
        ee.vv("Type").SetValueInt(what);
		ee.vv("Name").SetWideValue(opname().c_str());
		for (auto& p : Params)
		{
			auto& pe = ee["Params"].AddElement("Param");
			pe.vv("Name").SetWideValue(p.n.c_str());
            pe.vv("Value").SetValueFloat(p.v);
            pe.vv("Min").SetValueFloat(p.minv);
            pe.vv("Max").SetValueFloat(p.maxv);
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
			p.v = pe.vv("Value").GetValueFloat();
			p.minv = pe.vv("Min").GetValueFloat(std::numeric_limits<float>::min());
			p.maxv = pe.vv("Max").GetValueFloat(std::numeric_limits<float>::max());
			Params.push_back(p);
        }
    }


};

struct XLNODE_CONSTANT : public XLNODE_ANY
{


    XLNODE_CONSTANT() : XLNODE_ANY(0, TYPE_CONSTANT)
    {
        Params.push_back({ L"Value",0 });
    }
    std::vector<unsigned int> tensor_dims;

    virtual bool IsInput() { return true; }

    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE_ANY::Ser(ee);
        for (auto& d : tensor_dims)
        {
            auto& de = ee["Dimensions"].AddElement("Dimension");
            de.vv("Size").SetValueInt(d);
        }
    }

    virtual void Unser(XML3::XMLElement& e)
    {
        auto& ee = e["Dimensions"];
        for (size_t i = 0; i < ee.GetChildrenNum(); i++)
        {
            auto& de = ee.GetChildren()[i];
            tensor_dims.push_back(de->vv("Size").GetValueInt());
        }
        XLNODE_ANY::Unser(e);
    }
};



struct XLNODE_OUTPUT : public XLNODE
{
    virtual int nin() { return 1; }
    virtual int nout() { return 0; }

    virtual bool IsOutput() { return true; }

    XLNODE_OUTPUT()
    {
        XLNODEBULLET bu;
        bu.O = 0;
        children.push_back(bu);
    }

    virtual std::wstring subname()
    {
        std::wstring dr;
        if (csv_output.length())
            dr += csv_output;
        return dr;
    }

    virtual std::wstring name()
    {
        std::wstring dr = L"Output";
        return dr;
    }


    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE::Ser(ee);
        ee.vv("Name").SetWideValue(L"Output");
        ee.vv("Type").SetValueInt(TYPE_OUTPUT);
    }

    virtual void Unser(XML3::XMLElement& e)
    {
        XLNODE::Unser(e);
    }


};

struct XLNODE_INPUT : public XLNODE
{
    std::vector<unsigned int> tensor_dims;

    virtual bool IsInput() { return true; }


    XLNODE_INPUT()
    {
        XLNODEBULLET bu;
        bu.O = 1;
    	children.push_back(bu);
    }

    virtual std::wstring subname()
    {
        std::wstring dr;
        for (auto& d : tensor_dims)
        {
            dr += std::to_wstring(d) + L"x";
        }
        dr.pop_back();
        if (csv_input.length())
        {
            dr += L"\r\n";
            dr += csv_input;
        }
        return dr;
    }


    virtual std::wstring name()
    {
        std::wstring dr = L"Input";
		return dr;
    }

    virtual int nin() { return 0; }
    virtual int nout() { return 1; }

    virtual void Ser(XML3::XMLElement& ee)
    {
        ee.vv("Name").SetWideValue(L"Input");
        XLNODE::Ser(ee);
        ee.vv("Type").SetValueInt(TYPE_INPUT);
        for (auto& d : tensor_dims)
        {
            auto& de = ee["Dimensions"].AddElement("Dimension");
            de.vv("Size").SetValueInt(d);
        }

    }
    virtual void Unser(XML3::XMLElement& e)
    {
        auto& ee = e["Dimensions"];
		csv_input = e.vv("CSV").GetWideValue();
        for (size_t i = 0; i < ee.GetChildrenNum(); i++)
        {
            auto& de = ee.GetChildren()[i];
            tensor_dims.push_back(de->vv("Size").GetValueInt());
        }
        XLNODE::Unser(e);
    }

};

struct XLOP : public XLNODE
{
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
		for (auto& n : nodes)
		{
			auto& ne = ee.AddElement("Node");
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
        if (!F)
            MessageBeep(0);


        if (nty == TYPE_INPUT)
        {
            auto n = std::make_shared<XLNODE_INPUT>();
            n->Unser(ne);
            return n;
        }
        else
        if (nty == TYPE_OUTPUT)
        {
            auto n = std::make_shared<XLNODE_OUTPUT>();
            n->Unser(ne);
            return n;
        }
        else
        if (nty == TYPE_CONSTANT)
        {
            auto n = std::make_shared<XLNODE_CONSTANT>();
            n->Unser(ne);
            return n;
        }
        else
        {
            int ni = ne.vv("ni").GetValueInt();
            if (ni == 0)
                ni = 1;
            auto n = std::make_shared<XLNODE_ANY>(ni, nty);
            n->Unser(ne);
            return n;
        }
    }

	virtual void Unser(XML3::XMLElement& e)
	{
        nodes.clear();
		for (size_t i = 0; i < e.GetChildrenNum(); i++)
		{
			auto& ne = e.GetChildren()[i];
			auto n = Unser2(*ne);
			nodes.push_back(n);
		}
		data_type = (DML_TENSOR_DATA_TYPE)e.vv("DataType").GetValueInt();
		Zoom = e.vv("Zoom").GetValueFloat(1.0f);
	}
};

struct XL : public XLNODE
{
    std::vector<XLOP> ops;
    virtual std::wstring name() { return L"DML"; }
    virtual std::wstring subname() { return L""; }

    virtual void Ser(XML3::XMLElement& ee)
    {
		for (auto& op : ops)
		{
			auto& oe = ee.AddElement("Operator");
			op.Ser(oe);
		}
    }
    virtual void Unser(XML3::XMLElement& e)
    {
        ops.clear();
        for (size_t i = 0; i < e.GetChildrenNum(); i++)
        {
            auto& oe = e.GetChildren()[i];
            XLOP op;
            op.Unser(*oe);
            ops.push_back(op);
        }

		for (auto& op : ops)
		{
			for (auto& n : op.nodes)
			{
				for (auto& c : n->children)
				{
                    for(auto& gg : c.g)
                        nnn = std::max((unsigned long long)abs(n->ShareMemory),std::max(nnn + 1,gg + 1));
				}
			}
		}
    }
};

inline bool Hit(float x, float y, D2D1_RECT_F rc)
{
	return x >= rc.left && x <= rc.right && y >= rc.top && y <= rc.bottom;
}

namespace winrt::DirectMLGraph::implementation
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

        ML ml;
        XL xl;
        std::stack<XML3::XMLElement> undo_list;
        std::stack<XML3::XMLElement> redo_list;
        size_t ActiveOperator2 = (size_t)-1;
        std::shared_ptr<D2D> d2d;
        MLGraph()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
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

        void Push();
        void Undo();
        void Unselect();
        void Key(long long k,bool FromCmd = 0);
        void RefreshMenu();
        void OnLoaded(IInspectable, IInspectable);
        void OnCopy(IInspectable const&, IInspectable const&);
        void OnPaste(IInspectable const&, IInspectable const&);
        void OnDelete(IInspectable const&, IInspectable const&);
        void OnUndo(IInspectable const&, IInspectable const&);
        void OnRedo(IInspectable const&, IInspectable const&);
        void OnClean(IInspectable const&, IInspectable const&);
        void OnCompile(IInspectable const&, IInspectable const&);
        void OnRun(IInspectable const&, IInspectable const&);
        void OnAddOp(IInspectable const&, IInspectable const&);
        void OnAddInput(IInspectable const&, IInspectable const&);
        void OnAddConstant(IInspectable const&, IInspectable const&);
        void OnAddOutput(IInspectable const&, IInspectable const&);
        void OnNew(IInspectable const&, IInspectable const&);
        void OnOpen(IInspectable const&, IInspectable const&);
        void OnSave(IInspectable const&, IInspectable const&);
        void OnExit(IInspectable const&, IInspectable const&);
        void OnSaveAs(IInspectable const&, IInspectable const&);
        void Import(XML3::XMLElement& e);
        void Export(XML3::XMLElement& e);
        std::wstring current_file;
        void Paint();
        void Resize();
    };
}

namespace winrt::DirectMLGraph::factory_implementation
{
    struct MLGraph : MLGraphT<MLGraph, implementation::MLGraph>
    {
    };
}
