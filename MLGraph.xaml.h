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

struct XLNODE
{

    int tidx = 0; // idx in MLOP


	std::vector<XLNODEBULLET> children;

    virtual bool IsInput() { return false; }
    virtual bool IsOutput() { return false; }
    virtual void Ser(XML3::XMLElement& e);
    virtual void Unser(XML3::XMLElement& e);
    bool S = 0;
    virtual std::wstring name() = 0;
    D2F hit = {};

    virtual int nin() { return 0; }
    virtual int nout() { return 0; }
    virtual void Draw(ID2D1DeviceContext5* r, D2D* d2d);

};

enum XLNODE_TYPE
{
    TYPE_INPUT = 1,
    TYPE_ABS,TYPE_ACOS,TYPE_ACOSH,TYPE_ASIN,TYPE_ASINH,TYPE_ATAN,TYPE_ATANH,

    TYPE_BITAND,

    TYPE_NEGATE,

    TYPE_ADD = 50000, 
    TYPE_ATANYX,

    TYPE_OUTPUT = 999999
};

template <int NI = 1>
struct XLNODE_ANY : public XLNODE
{
    int what = 0;

    virtual int nin() { return NI; }
    virtual int nout() { return 1; }

    XLNODE_ANY(int w)
    {
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

    virtual std::wstring name()
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

        if (what == TYPE_NEGATE)
            return L"Neg";

        return L"Unknown";
    }


    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE::Ser(ee);
        ee.vv("Type").SetValueInt(what);
    }

    virtual void Unser(XML3::XMLElement& e)
    {
        XLNODE::Unser(e);
    }


};

using XLNODE_11 = XLNODE_ANY<1>;
using XLNODE_21 = XLNODE_ANY<2>;
using XLNODE_31 = XLNODE_ANY<3>;
using XLNODE_41 = XLNODE_ANY<4>;
using XLNODE_51 = XLNODE_ANY<5>;


struct XLNODE_OUTPUT : public XLNODE
{
    virtual int nin() { return 1; }
    virtual int nout() { return 0; }

    std::wstring csv_output;
    virtual bool IsOutput() { return true; }

    XLNODE_OUTPUT()
    {
        XLNODEBULLET bu;
        bu.O = 0;
        children.push_back(bu);
    }

    virtual std::wstring name()
    {
        std::wstring dr = L"Output\r\n";
        if (csv_output.length())
        {
            dr += L"\r\n";
            dr += csv_output;
        }
        return dr;
    }


    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE::Ser(ee);
        ee.vv("CSV").SetWideValue(csv_output.c_str());
        ee.vv("Type").SetValueInt(TYPE_OUTPUT);
    }

    virtual void Unser(XML3::XMLElement& e)
    {
		csv_output = e.vv("CSV").GetWideValue();
        XLNODE::Unser(e);
    }


};

struct XLNODE_INPUT : public XLNODE
{
    std::vector<unsigned int> tensor_dims;

    virtual bool IsInput() { return true; }

    std::wstring csv_input;

    XLNODE_INPUT()
    {
        XLNODEBULLET bu;
        bu.O = 1;
    	children.push_back(bu);
    }

    virtual std::wstring name()
    {
        std::wstring dr = L"Input\r\n";
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

    virtual int nin() { return 0; }
    virtual int nout() { return 1; }

    virtual void Ser(XML3::XMLElement& ee)
    {
        XLNODE::Ser(ee);
        ee.vv("Type").SetValueInt(TYPE_INPUT);
		ee.vv("CSV").SetWideValue(csv_input.c_str()); 
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
    std::vector<std::shared_ptr<XLNODE>> nodes;
	DML_TENSOR_DATA_TYPE data_type = DML_TENSOR_DATA_TYPE_FLOAT32;

    virtual std::wstring name() { return L"Operator"; }

    virtual void Ser(XML3::XMLElement& ee)
    {
		ee.vv("DataType").SetValueInt((int)data_type);
		for (auto& n : nodes)
		{
			auto& ne = ee.AddElement("Node");
			n->Ser(ne);
		}
    }
	virtual void Unser(XML3::XMLElement& e)
	{
        nodes.clear();
		for (size_t i = 0; i < e.GetChildrenNum(); i++)
		{
			auto& ne = e.GetChildren()[i];
			int nty = ne->vv("Type").GetValueInt();
            if (nty == TYPE_INPUT)
			{
				auto n = std::make_shared<XLNODE_INPUT>();
				n->Unser(*ne);
				nodes.push_back(n);
			}
            else
            if (nty == TYPE_OUTPUT)
            {
                auto n = std::make_shared<XLNODE_OUTPUT>();
                n->Unser(*ne);
                nodes.push_back(n);
            }
            else
            if (nty == TYPE_ADD || nty == TYPE_ATANYX || nty == TYPE_BITAND)
            {
                auto n = std::make_shared<XLNODE_21>(nty);
                n->Unser(*ne);
                nodes.push_back(n);
            }
            else
			{
				auto n = std::make_shared<XLNODE_11>(nty);
				n->Unser(*ne);
				nodes.push_back(n);
			}
		}
		data_type = (DML_TENSOR_DATA_TYPE)e.vv("DataType").GetValueInt();
	}
};

struct XL : public XLNODE
{
    std::vector<XLOP> ops;
    virtual std::wstring name() { return L"DML"; }

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
                        nnn = std::max(nnn + 1,gg + 1);
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
        size_t ActiveOperator = (size_t)-1;
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
        void Key(long long k);
        void RefreshMenu();
        void OnLoaded(IInspectable, IInspectable);
        void OnUndo(IInspectable const&, IInspectable const&);
        void OnRedo(IInspectable const&, IInspectable const&);
        void OnCompile(IInspectable const&, IInspectable const&);
        void OnRun(IInspectable const&, IInspectable const&);
        void OnAddOp(IInspectable const&, IInspectable const&);
        void OnAddInput(IInspectable const&, IInspectable const&);
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
