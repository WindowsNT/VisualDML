#include "pch.h"
#include "MLGraph.xaml.h"
#if __has_include("MLGraph.g.cpp")
#include "MLGraph.g.cpp"
#endif

#include "dmlextensions.hpp"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

extern std::map<HWND, winrt::Windows::Foundation::IInspectable> windows;
std::vector<CComPtr<IDXGIAdapter1>> all_adapters;

extern POINT MouseHoverPT;
void ClearRectsAndTips();
void AddRectAndTip(D2D1_RECT_F r, const wchar_t* t);

std::shared_ptr<XLNODE> MovingNodeP = nullptr;
int MovingNode = 0;
int WhatInput = 0;
PARAM* WhatParam = 0;
XLNODE* WhatNode = 0;
VARIABLE* WhatVariable = 0;
XLNODEBULLET* WhatBullet = 0;
CONNECTION* WhatConnection = 0;
CONDITION* WhatCondition = 0;
float ScaleX = 1.0f;
bool MustStop = 0;

class PUSHPOPVAR
{
public:
    XL* xl;
    int State = 0;
    PUSHPOPVAR(XL* x)
    {
        xl = x;
        On();
    }
    void On()
    {
        if (State == 1)
            return;
        State = 1;
        // Replace variables
        for (auto& op : xl->ops)
        {
            for (auto& node : op.nodes)
            {
                if (auto it = std::dynamic_pointer_cast<XLNODE>(node))
                {
                    for (auto& p : it->Params)
                    {
                        p.save_v = p.v;
                        for (auto& v : xl->variables)
                        {

                            auto n = v.n;
                            while (p.v.find(n) != std::string::npos)
                            {
                                p.v.replace(p.v.find(n), n.length(), v.orv);
                            }
                        }
                    }
                }
            }
        }
    }

    ~PUSHPOPVAR()
    {
        Off();
    }

    void Off()
    {
        // Replace variables
        if (State == 0)
            return;
        State = 0;
        for (auto& op : xl->ops)
        {
            for (auto& node : op.nodes)
            {
                if (auto it = std::dynamic_pointer_cast<XLNODE>(node))
                {
                    for (auto& p : it->Params)
                    {
                        p.v = p.save_v;
                    }
                }
            }
        }
    }

};


D2D1_POINT_2F red_from = { 0,0 }, red_to = { 0,0 };
void XLNODE::Ser(XML3::XMLElement& e)
{
    // Hit
    e.vv("xx").SetValueFloat(hit.left);
    e.vv("yy").SetValueFloat(hit.top);
    e.vv("CSV").SetWideValue(csv_output.c_str());
    e.vv("CSVI").SetWideValue(csv_input.c_str());
	e.vv("bf").SetValueInt(BufferVisible);
	e.vv("sm").SetValueLongLong(ShareMemory);  
	e.vv("optype").SetValueInt(OpType); 

    auto& ch = e["Children"];
    for (auto& c : children)
    {
        auto& ce = ch.AddElement("Child");
        ce.vv("idx").SetValueULongLong(c.i);
        ce.vv("io").SetValueULongLong(c.O);


        // Connections conditions
		auto& Connections = ce["Connections"];
		for (auto& g : c.g)
		{
			auto& ge = Connections.AddElement("Connection");
			ge.vv("key").SetValueULongLong(g.key);
			auto& Conditions = ge["Conditions"];
			for (auto& cond : g.conditions)
			{
                cond.Ser(Conditions.AddElement("Condition"));
			}
		}

/*        std::wstring ggs;
		for (auto& gg : c.g)
		{
			ggs += std::to_wstring(gg.key) + L",";
		}
		if (ggs.length())
			ggs.pop_back();
        ce.vv("u").SetWideValue(ggs.c_str());
*/
    }

}


void XLNODE::Unser(XML3::XMLElement& e)
{
    hit.left = e.vv("xx").GetValueFloat();
    csv_output = e.vv("CSV").GetWideValue();
	csv_input = e.vv("CSVI").GetWideValue();
    hit.top = e.vv("yy").GetValueFloat();
	BufferVisible = e.vv("bf").GetValueInt();
	ShareMemory = e.vv("sm").GetValueLongLong();
	OpType = e.vv("optype").GetValueInt(DML_TENSOR_DATA_TYPE_FLOAT32);

    children.clear();
    auto& ch = e["Children"];
    for (size_t i = 0; i < ch.GetChildrenNum(); i++)
    {
        auto& ce = ch.GetChildren()[i];
        XLNODEBULLET bu;
		bu.i = ce->vv("idx").GetValueULongLong();
		bu.O = (bool)ce->vv("io").GetValueULongLong();
        std::vector<std::wstring> split(const std::wstring & s, wchar_t delim);

        // Conditions
		auto ggs = ce->vv("u").GetWideValue();
        if (ggs.length())
        {
            auto gg = split(ggs, L',');
            for (auto& g : gg)
            {
                CONNECTION conn;
                conn.key = std::stoull(g);
                bu.g.push_back(conn);
            }
        }
        else
        {
            // New method of conditionsaving
			auto& Connections = (*ce)["Connections"];
            for (size_t ii = 0; ii < Connections.GetChildrenNum(); ii++)
            {
				auto& ge = Connections.GetChildren()[ii];
				CONNECTION conn;
				conn.key = ge->vv("key").GetValueULongLong();
				auto& Conditions = (*ge)["Conditions"];
                for (size_t j = 0; j < Conditions.GetChildrenNum(); j++)
                {
                    auto& ce2 = Conditions.GetChildren()[j];
                    CONDITION cond;
					cond.Unser(*ce2);
                    conn.conditions.push_back(cond);
                }
				bu.g.push_back(conn);
            }

        }
        


        children.push_back(bu);
    }
}


/*unsigned long long XLNODE::min_key(XLOP* par)
{
    unsigned long long minKeyA = (unsigned long long) - 1;
    for (auto& c : children)
    {
        for (auto& k : c.g)
        {
            if (k.key < minKeyA)
            {
                // Check if used
                for (auto& no : par->nodes)
                {

                }
                minKeyA = k.key;
            }
        }
    }
    return minKeyA;
}
*/

void XLNODE::Draw(MLOP* mlop,bool Active,bool Enabled,ID2D1DeviceContext5* r, size_t iop, [[maybe_unused]] size_t inod)
{
    TEXTALIGNPUSH tep(d2d->Text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    TEXTALIGNPUSH tep2(d2d->Text2, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    const auto name1 = name();
    const auto name2 = subname();

    auto msr1 = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text, name1.c_str());
    if (std::get<0>(msr1) <= 100)
		std::get<0>(msr1) = 100;
    auto msr2 = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text2, name2.c_str());
    if (name2.length() == 0)
        std::get<1>(msr2) = 0;

    auto DefBrush = Enabled ? d2d->BlackBrush : d2d->SnapBrush2;


    wchar_t hdr[100] = {};
    wchar_t ftr[200] = {};
#ifdef _DEBUG
    swprintf_s(hdr, 100, L"OP %zi N %zi", iop + 1, inod + 1);
#else
    swprintf_s(hdr, 100, L"OP %zi", iop + 1);
#endif
    
	auto ant = dynamic_cast<XLNODE_ANY*>(this);
    if (ant)
    {
        if (ant->Jump.iNodeJumpTo)
            wcscat_s(hdr, 100, L" [J]");
    }

    if (tidxs.size() > 0 && mlop && Enabled)
    {
        //* Multiple outputs visibility
        int ct = 0;
        for (auto& a_tidx : tidxs)
        {
            if (mlop->Count() > a_tidx)
            {
                auto& it = mlop->Item(a_tidx);
                if (ct > 0)
					swprintf_s(ftr + wcslen(ftr), 10, L"\r\n");
                try
                {
                    auto buf = it.expr.GetOutputDesc();
                    //            DML_BUFFER_TENSOR_DESC* buf = (DML_BUFFER_TENSOR_DESC*)desc.Desc;
                    for (UINT i = 0; i < buf.sizes.size(); i++)
                    {
                        if (i != buf.sizes.size() - 1)
                            swprintf_s(ftr + wcslen(ftr), 10, L"%ix", buf.sizes[i]);
                        else
                            swprintf_s(ftr + wcslen(ftr), 10, L"%i", buf.sizes[i]);
                    }
                    swprintf_s(ftr + wcslen(ftr), 10, L" %s", optypes[buf.dataType].c_str());
                }
                catch (...)
                {

                }
                ct++;
            }
        }
    }


    auto msrheader = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text, hdr);
	auto msrfooter = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text2, ftr);

	float toth = std::get<1>(msrheader) + std::get<1>(msrfooter) + 10;
    if (wcslen(ftr) == 0)
		toth = std::get<1>(msrheader);

    float hmarg = 10 + toth + std::max(nin(), nout()) * 10;
    D2D1_RECT_F rtext = { hit.left + 10, hit.top + hmarg, hit.left + 10 + std::get<0>(msr1) + std::get<0>(msr2), hit.top + hmarg + std::get<1>(msr1) + std::get<1>(msr2) };
    hit.right = rtext.right + 10;
    hit.bottom = rtext.bottom + hmarg;
    if (1)
    {
        if (name2.length())
        {
            TEXTALIGNPUSH tep3(d2d->Text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            r->DrawTextW(name1.c_str(), (UINT32)name1.length(), d2d->Text, rtext, DefBrush);
        }
        else
            r->DrawTextW(name1.c_str(), (UINT32)name1.length(), d2d->Text, rtext, DefBrush);
    }
    if (name2.length())
    {
        TEXTALIGNPUSH tep3(d2d->Text2, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_FAR);
        r->DrawTextW(name2.c_str(), (UINT32)name2.length(), d2d->Text2, rtext, DefBrush);
    }
//    r->DrawRectangle(rtext, Active ? d2d->RedBrush : d2d->BlackBrush);
    D2D1_ROUNDED_RECT rr = { hit, 10,10 };
    if (IsInput())
    {
        d2d->CyanBrush->SetOpacity(0.2f);
        d2d->RedBrush->SetOpacity(0.2f);
        r->FillRoundedRectangle(rr, S ? d2d->RedBrush : d2d->CyanBrush);
        d2d->RedBrush->SetOpacity(1.0f);
        d2d->CyanBrush->SetOpacity(1.0f);
    }
    else
    if (IsOutput())
    {
        d2d->GreenBrush->SetOpacity(0.2f);
        d2d->RedBrush->SetOpacity(0.2f);
        r->FillRoundedRectangle(rr, S ? d2d->RedBrush : d2d->GreenBrush);
        d2d->RedBrush->SetOpacity(1.0f);
        d2d->GreenBrush->SetOpacity(1.0f);
    }
    else
    {
        if (S)
        {
            d2d->RedBrush->SetOpacity(0.2f);
            r->FillRoundedRectangle(rr, S ? d2d->RedBrush : d2d->GreenBrush);
            d2d->RedBrush->SetOpacity(1.0f);
        }
    }

    if (wcslen(hdr))
    {
		D2D1_POINT_2F left = { hit.left, hit.top + std::get<1>(msrheader) + 5};
		D2D1_POINT_2F right = { hit.right, left.y };
		r->DrawLine(left, right, DefBrush,0.3f);
        auto fr = D2D1_RECT_F({ hit.left, hit.top, hit.right, hit.top + std::get<1>(msrheader) + 5 });
        //r->FillRoundedRectangle(D2D1_ROUNDED_RECT{ fr, 5, 5 }, d2d->SnapBrush2);
		r->DrawTextW(hdr, (UINT32)wcslen(hdr), d2d->Text, fr, Active ? d2d->RedBrush : DefBrush);
    }

    if (wcslen(ftr))
    {
        D2D1_POINT_2F left = { hit.left + 5, hit.bottom - std::get<1>(msrfooter) - 5 };
        D2D1_POINT_2F right = { hit.right - 5, left.y };
        r->DrawLine(left, right, DefBrush, 0.3f);
        r->DrawTextW(ftr, (UINT32)wcslen(ftr), d2d->Text2, D2D1_RECT_F({ hit.left + 5, hit.bottom, hit.right - 5, hit.bottom - std::get<1>(msrfooter) - 5 }), DefBrush);
    }

    r->DrawRoundedRectangle(rr, S ? d2d->RedBrush : DefBrush);

	children.resize(nin() + nout());    

    float elr = 7.5f;

    for (int i = 0; i < 2; i++)
    {
        size_t Total_Bullets = i == 0 ? nin() : nout();
        float HPerBullet = 15.0f;
        float TotalHeight = Total_Bullets * HPerBullet;
        float NextY = hit.top + 5;
        float FullHeight = hit.Height();
        NextY = (FullHeight - TotalHeight) / 2.0f + hit.top;
        for (auto& ch : children)
        {
            // Calculate the bullet's position
            if (ch.O == (bool)i)
            {
                ch.hit.top = NextY;
				ch.hit.bottom = NextY + (HPerBullet - 5);
                ch.hit.left = hit.left;
                if (i == 1)
                    ch.hit.left = hit.right - elr / 2.0f;
                ch.hit.right = ch.hit.left + elr / 2.0f;
				NextY += HPerBullet;
            }
        }
    }

    for (int i = 0; i < (nin() + nout()); i++)
    {
        auto& ch = children[i];
        // bullet draw with centering
        // calculate top
        D2D1_ELLIPSE el = { { ch.hit.MiddleX(),ch.hit.MiddleY()}, elr, elr };
        bool Red = ch.S;
        if (Hit(red_to.x, red_to.y, ch.hit) && red_to.x > 0 && red_to.y > 0 && MovingNode != 2)
        {
            Red = 1;
        }

		auto xn = dynamic_cast<XLNODE_ANY*>(this);
        bool OptionalIn = 0;
		if (i < nin() && xn &&  i >= xn->ninreq())
		{
    		OptionalIn = 1;
		}

        bool Connected = false;
        r->FillEllipse(el, Red ? d2d->RedBrush :  Connected ? d2d->CyanBrush : OptionalIn ? d2d->SnapBrush2 : DefBrush);
        ch.hit = D2D1_RECT_F({ el.point.x - el.radiusX, el.point.y - el.radiusY, el.point.x + el.radiusX, el.point.y + el.radiusY });

    }


/*    if (nin() && 0)
    {
        for (int i = 0; i < nin(); i++)
        {
			auto& ch = children[i];
            // bullet draw with centering
            // calculate top
            D2D1_ELLIPSE el = { { ch.hit.MiddleX(),ch.hit.MiddleY()}, elr, elr};
            bool Red = ch.S;
			if (Hit(red_to.x, red_to.y, ch.hit) && red_to.x > 0 && red_to.y > 0 && MovingNode != 2)
			{
				Red = 1;
			}
            bool Connected = false;
            r->FillEllipse(el, Red ? d2d->RedBrush : Connected ? d2d->CyanBrush : DefBrush);
            ch.hit = D2D1_RECT_F({ el.point.x - el.radiusX, el.point.y - el.radiusY, el.point.x + el.radiusX, el.point.y + el.radiusY });
        }

    }

    if (nout() && 0)
    {
        for (int i = 0; i < nout(); i++)
        {
			auto& ch = children[i + nin()];

            // bullet draw with centering
            // calculate top
            float tc = hit.top + (hit.bottom - hit.top) / 2.0f;
            if (i == 1)
            {

            }
            else
            {
                tc -= (nout() - 1) * 10 / 2;
            }


            D2D1_ELLIPSE el = { { hit.right, tc }, elr, elr };
            r->FillEllipse(el, ch.S ? d2d->RedBrush : DefBrush);
            ch.hit =  D2D1_RECT_F({ el.point.x - el.radiusX, el.point.y - el.radiusY, el.point.x + el.radiusX, el.point.y + el.radiusY });
        }
    }
    */

    if (BufferVisible || IsInput() || IsOutput())
    {
        bhit.left = hit.MiddleX() - 10;
		bhit.right = hit.MiddleX() + 10;
        bhit.top = hit.bottom - 5;
		bhit.bottom = hit.bottom + 5;
		D2D1_ROUNDED_RECT rr4 = { bhit, 5,5 };
        if (ShareMemory >= 0)   
    		r->FillRoundedRectangle(rr4, bSelected ? d2d->RedBrush : ShareMemory > 0 ? d2d->CyanBrush : DefBrush);
    }

    if (IsInput())
    {
        bhit2.left = hit.MiddleX() - 10;
        bhit2.right = hit.MiddleX() + 10;
        bhit2.top = hit.top - 5;
        bhit2.bottom = hit.top + 5;
        D2D1_ROUNDED_RECT rr4 = { bhit2, 5,5 };
        bool Red = 0;
        if (Hit(red_to.x, red_to.y, bhit2) && red_to.x > 0 && red_to.y > 0 && MovingNode == 2)
        {
            Red = 1;
        }
        r->FillRoundedRectangle(rr4, Red ? d2d->RedBrush : ShareMemory < 0 ? d2d->CyanBrush : DefBrush);
    }
}






winrt::Microsoft::UI::Xaml::Controls::MenuFlyout BuildTensorMenu(std::function<void(const winrt::Windows::Foundation::IInspectable,const winrt::Windows::Foundation::IInspectable)> fooo)
{
    winrt::Microsoft::UI::Xaml::Controls::MenuFlyout r1;

    auto SepIf = [&]()
        {
            if (r1.Items().Size())
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
                r1.Items().Append(s);
            }
        };



    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Input"); O.Click(fooo);
        r1.Items().Append(O);
        SepIf();
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"Activation");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationCelu"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationElu"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationGelu"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationHardmax"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationHardSigmoid"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationIdentity"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationLeakyRelu"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ActivationLinear"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        for (auto& j : { L"ActivationLogSoftmax",L"ActivationParameterizedRelu",L"ActivationParametricSoftplus", L"ActivationRelu", L"ActivationScaledElu", L"ActivationScaledTanh", L"ActivationShrink", L"ActivationSigmoid", L"ActivationSoftmax", L"ActivationSoftplus", L"ActivationSoftsign", L"ActivationTanh", L"ActivationThresholdedRelu" })
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(j); Neg.Click(fooo);
            A.Items().Append(Neg);
        }





        r1.Items().Append(A);
        SepIf();
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"Comparison");
        
      
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Equals"); N.Click(fooo);
            A.Items().Append(N);
        } 
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"GreaterThan"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"GreaterThanOrEqual"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"If"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"IsInfinity"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"IsNan"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"LessThan"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"LessThanOrEqual"); N.Click(fooo);
            A.Items().Append(N);
        }

        r1.Items().Append(A);
        SepIf();
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"Batch and Gradients");


        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BatchNormalization"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BatchNormalizationTraining"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BatchNormalizationGrad"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BatchNormalizationTrainingGrad"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"ClipGrad"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ResampleGrad"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"SliceGrad"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"RoiAlignGrad"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }


        r1.Items().Append(A);
        SepIf();
    }



    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"A");


		winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Abs; Abs.Text(L"Abs"); Abs.Click(fooo);
		A.Items().Append(Abs);   
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ACos; ACos.Text(L"ACos"); ACos.Click(fooo);
        A.Items().Append(ACos);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ACosh; ACosh.Text(L"ACosh"); ACosh.Click(fooo);
        A.Items().Append(ACosh);


        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Add; Add.Text(L"Add"); Add.Click(fooo);
        A.Items().Append(Add);


        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"And"); N.Click(fooo);
            A.Items().Append(N);
        }
        
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ASin; ASin.Text(L"ASin"); ASin.Click(fooo);
        A.Items().Append(ASin);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ASinh; ASinh.Text(L"ASinh"); ASinh.Click(fooo);
        A.Items().Append(ASinh);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ATan; ATan.Text(L"ATan"); ATan.Click(fooo);
        A.Items().Append(ATan);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ATanh; ATanh.Text(L"ATanh"); ATanh.Click(fooo);
        A.Items().Append(ATanh);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem ATanYX; ATanYX.Text(L"ATanYX"); ATanYX.Click(fooo);
        A.Items().Append(ATanYX);
		winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem AveragePool; AveragePool.Text(L"AveragePooling"); AveragePool.Click(fooo);
		A.Items().Append(AveragePool);

        

          


        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"B");

        
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem BitAnd; BitAnd.Text(L"BitAnd"); BitAnd.Click(fooo);
        A.Items().Append(BitAnd);
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BitCount"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BitNot"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BitOr"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BitShiftLeft"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BitShiftRight"); b3.Click(fooo);
            A.Items().Append(b3);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem b3; b3.Text(L"BitXor"); b3.Click(fooo);
            A.Items().Append(b3);
        }


        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"C");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Cast"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Ceil"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Clip"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Constant"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"ConvolutionInteger"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Cos"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Cosh"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Convolution"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
			winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"CummulativeProduct"); N.Click(fooo);
			A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"CummulativeSum"); N.Click(fooo);
            A.Items().Append(N);

        }
        r1.Items().Append(A);




    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"D");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Divide"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"DepthToSpace"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Dequantize"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"DequantizeLinear"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"DiagonalMatrix"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"DifferenceSquare"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }




        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"E");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Erf"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Exp"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
      

        r1.Items().Append(A);
    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"F");

        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Floor"); Neg.Click(fooo);
        A.Items().Append(Neg);

        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"G");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Gather"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"GatherElements"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"GatherND"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Gemm"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Gru"); N.Click(fooo);
            A.Items().Append(N);
        }


        r1.Items().Append(A);
    }



    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"I");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Identity"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }


        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"J");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Join"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }


        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"L");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Log"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"LocalResponseNormalization"); Neg.Click(fooo);
            A.Items().Append(Neg);

        }
     

        r1.Items().Append(A);
    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"M");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Min"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"MaxPooling"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Mean"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"MeanVarianceNormalization"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Max"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }


        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Multiply"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ModulusFloor"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ModulusTruncate"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }


        r1.Items().Append(A);
    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"N");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Neg"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"NonZeroCoordinates"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Not"); N.Click(fooo);
            A.Items().Append(N);
        }


        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"O");
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"OneHot"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Or"); N.Click(fooo);
            A.Items().Append(N);
        }
        r1.Items().Append(A);

    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"P");

//        dml::

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Padding"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Pow"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"Q"); 
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"QuantizeLinear"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"QuantizedLinearConvolution"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"R");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"RandomGenerator"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Reinterpret"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Recip"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Reduce"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Resample"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ReverseSubsequences"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"RoiAlign"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Round"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        r1.Items().Append(A);
    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"S");

        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"ScatterElements"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Sign"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Slice"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"SpaceToDepth"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Subtract"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Sqrt"); Neg.Click(fooo);
            A.Items().Append(Neg);
        }

        r1.Items().Append(A);
    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"T");
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Threshold"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"TopK"); N.Click(fooo);
            A.Items().Append(N);
        }
        r1.Items().Append(A);

    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"U");
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Upsample2D"); N.Click(fooo);
            A.Items().Append(N);
        }
        r1.Items().Append(A);

    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"V");
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"ValueScale2D"); N.Click(fooo);
            A.Items().Append(N);
        }
        r1.Items().Append(A);

    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"X");
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Xor"); N.Click(fooo);
            A.Items().Append(N);
        }
        r1.Items().Append(A);

    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
        r1.Items().Append(s);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Output"); O.Click(fooo);
        r1.Items().Append(O);
    }

    return r1;
}


extern std::wstring fil;
DWORD MainTID = 0;


winrt::Microsoft::UI::Xaml::Controls::MenuFlyout BuildNodeRightMenuCondition(std::shared_ptr<XLNODE> nd,XLNODEBULLET* conn, std::function<void(const winrt::Windows::Foundation::IInspectable, const winrt::Windows::Foundation::IInspectable)> fooo)
{
    wchar_t a[1000] = {};
    winrt::Microsoft::UI::Xaml::Controls::MenuFlyout r1;
    for (size_t i = 0 ; i < conn->g.size() ; i++)
    {
        auto& g = conn->g[i];
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
#ifdef _DEBUG
        swprintf_s(a, 100,L"Connection %llu",g.key);
#else
        swprintf_s(a, 100,L"Connection %zi", i + 1);
#endif
        A.Text(a);
		for (size_t ii = 0; ii < g.conditions.size(); ii++)
		{
            auto& cond = g.conditions[ii];
			winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(XML3::XMLU(cond.expression.c_str()).wc()); O.Click(fooo);
            swprintf_s(a, 1000, L"%llu", ((i + 1) * 100) + (ii + 1));
			O.Tag(winrt::box_value(a)); 
			if (conn->g.size() == 1)
				r1.Items().Append(O);
            else
    			A.Items().Append(O);
		}
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
            if (conn->g.size() == 1)
                r1.Items().Append(s);
            else
                A.Items().Append(s);

            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(XML3::XMLU(L"Add condition...").wc()); O.Click(fooo);
            swprintf_s(a, 1000, L"%llu", ((i + 1) * 100) + 0);
            O.Tag(winrt::box_value(a));
            if (conn->g.size() == 1)
                r1.Items().Append(O);
            else
                A.Items().Append(O);

            if (g.conditions.size() > 0)
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O2; O2.Text(XML3::XMLU(L"Clear conditions").wc()); O2.Click(fooo);
                swprintf_s(a, 1000, L"%llu", ((i + 1) * 100) + 99);
                O2.Tag(winrt::box_value(a));
                if (conn->g.size() == 1)
                    r1.Items().Append(O2);
                else
                    A.Items().Append(O2);
            }
        }
		if (conn->g.size() > 1)
			r1.Items().Append(A);   
    }
    return r1;
}

namespace winrt::VisualDML::implementation
{

    winrt::Microsoft::UI::Xaml::Controls::MenuFlyout MLGraph::BuildNodeRightMenu(XL& xl, std::shared_ptr<XLNODE> nd, int Type, std::function<void(const winrt::Windows::Foundation::IInspectable, const winrt::Windows::Foundation::IInspectable)> fooo)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyout r1;

        auto SepIf = [&]()
            {
                if (r1.Items().Size())
                {
                    winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
                    r1.Items().Append(s);
                }
            };

        if (Type == 1)
        {
            SepIf();
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Tensor shape..."); O.Click(fooo);
            r1.Items().Append(O);
        }
        if (Type == 1)
        {
            SepIf();
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
            A.Text(L"Input");

            if (1)
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"CSV input..."); O.Click(fooo);
                A.Items().Append(O);
            }
            if (1)
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Binary input..."); O.Click(fooo);
                A.Items().Append(O);
            }
            if (1)
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"\"Random\""); O.Click(fooo);
                A.Items().Append(O);
            }
            if (1)
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"\"Sequence\""); O.Click(fooo);
                A.Items().Append(O);
            }
            if (1)
            {
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Clear input"); O.Click(fooo);
                A.Items().Append(O);
            }

            r1.Items().Append(A);
        }

        if (nd->AsksType())
        {
            SepIf();
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
            A.Text(L"Type");
            for (int i = 1; i <= MAX_OP_TYPES; i++)
            {
                winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem O; O.Text(optypes[i]); O.Click(fooo);
                if (nd->OpType == i)
                    O.IsChecked(true);
                A.Items().Append(O);
            }
            r1.Items().Append(A);
        }

        if (nd->Params.size())
        {
            SepIf();
            for (size_t i = 0; i < nd->Params.size(); i++)
            {
                if (nd->Params[i].list_names.size())
                {
                    winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
                    A.Text(nd->Params[i].n.c_str());

                    for (size_t ii = 0; ii < nd->Params[i].list_names.size(); ii++)
                    {
                        winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem O;
                        O.Text(nd->Params[i].list_names[ii].c_str());
                        ULARGE_INTEGER ul = {};
                        ul.LowPart = (int)i + 2000;
                        ul.HighPart = (int)ii + 1;
                        O.Tag(winrt::box_value(ul.QuadPart));
                        O.Click(fooo);
                        if ((size_t)nd->Params[i] == ii)
                            O.IsChecked(true);
                        A.Items().Append(O);

                    }
                    r1.Items().Append(A);
                }
                else
                    if (nd->Params[i].minv == 0 && nd->Params[i].maxv == 1)
                    {
                        winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem O;
                        O.Text(nd->Params[i].n.c_str());
                        O.Tag(winrt::box_value(i + 2000));
                        O.Click(fooo);
                        if ((int)nd->Params[i] == 1)
                            O.IsChecked(true);
                        r1.Items().Append(O);
                    }
                    else
                    {
                        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O;
                        O.Text(nd->Params[i].n.c_str());
                        O.Tag(winrt::box_value(i + 2000));
                        O.Click(fooo);
                        r1.Items().Append(O);
                    }
            }
        }

        if (Type == 2)
        {
        }

        if (Type == 1 || Type == 2 || Type == 3)
        {
            if (Type != 1 && Type != 2)
            {
                SepIf();
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
                A.Text(L"Output");

                if (1)
                {
                    winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"CSV output..."); O.Click(fooo);
                    A.Items().Append(O);
                }
                if (1)
                {
                    winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Binary output..."); O.Click(fooo);
                    A.Items().Append(O);
                }
                if (1)
                {
                    winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"\"Screen\""); O.Click(fooo);
                    A.Items().Append(O);
                }
                if (1)
                {
                    winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Clear output"); O.Click(fooo);
                    A.Items().Append(O);
                }

                r1.Items().Append(A);
            }
            if (Type != 1 && Type != 3 && nd->nout() == 1)
            {
                SepIf();
                winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem O1; O1.Text(L"Visible Buffer"); O1.Click(fooo); O1.IsChecked(nd->BufferVisible);
                r1.Items().Append(O1);
            }
            if (xl.variables.size())
            {
                SepIf();
                winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
                A.Text(L"Variable Change");

                for (size_t iv = 0; iv < xl.variables.size(); iv++)
                {
                    winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem O1; O1.Text(xl.variables[iv].n);

                    // Check if there already
                    bool F = 0;
					auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nd);
                    if (it)
                    {
                        for (size_t i = 0; i < it->VariableChanges.size(); i++)
                        {
                            if (it->VariableChanges[i].n == xl.variables[iv].n)
                            {
                                F = 1;
                                break;
                            }
                        }
                    }
					O1.IsChecked(F);
                    O1.Click([&,iv,nd](IInspectable wh, IInspectable)
                        {
                            auto mf = wh.as< ToggleMenuFlyoutItem>();
                            if (!mf.IsChecked())
                            {
                                Push();
                                Dirty(1);
                                auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nd);
                                if (it)
                                {
                                    for (size_t i = 0; i < it->VariableChanges.size(); i++)
                                    {
                                        if (it->VariableChanges[i].n == xl.variables[iv].n)
                                        {
											it->VariableChanges.erase(it->VariableChanges.begin() + i);
                                            break;
                                        }
                                    }
                                }
                                FullRefresh();
                            }
                            else
                            {
                                Refresh({ L"i1",L"i0" });
                                WhatInput = 11;
                                WhatNode = nd.get();
                                WhatVariable = &xl.variables[iv];
                                auto sp = Content().as<Panel>();
                                auto ct = sp.FindName(L"InputVariableChange").as<ContentDialog>();
                                ct.ShowAsync();
                            }

                        });
                    A.Items().Append(O1);
                }

                r1.Items().Append(A);
            }
        }

        return r1;
    }


    void MLGraph::FullRefresh()
    {
		if (GetCurrentThreadId() != MainTID)
		{
            void PostUpdateScreen();
            PostUpdateScreen();
			return;
		}   
        Refresh();
        RefreshMenu();
        Paint();
    }

    void MLGraph::Key(long long k,bool FromCmd)
    {
        [[maybe_unused]] bool Shift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
        [[maybe_unused]] bool Control = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
        [[maybe_unused]] bool Alt = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

        auto& xl = prj.xl();

        if (k == 0x30)
        {
            if (!Control)
                xl.ops[ActiveOperator2].Zoom = 1.0f;
            Paint();
            return;
        }
        if (k == 187)
        {
            if (!Control)
            {
                xl.ops[ActiveOperator2].Zoom += 0.1f;
            }
            Paint();
            return;
        }
        if (k == 189)
        {
            if (!Control)
            {
                if (xl.ops[ActiveOperator2].Zoom > 0.1f)
                    xl.ops[ActiveOperator2].Zoom -= 0.1f;
            }
            Paint();
            return;
        }

        if (k >= 0x31 && k <= 0x39)
        {
            if (Alt && !Shift && Control)
            {
				if (prj.xls.size() > (unsigned long long)(k - 0x31))
				{
					prj.iActive = k - 0x31;
                    FullRefresh();
                }
                return;
            }

            if (Alt && !Shift && !Control)
            {
                if (xl.ops.size() > (unsigned long long)(k - 0x31))
                {
                    xl.ops[k - 0x31].Visible = !xl.ops[k - 0x31].Visible;
                    FullRefresh();
                }
                return;
            }
            if (!Alt && !Shift && Control)
            {
                if (xl.ops.size() > (unsigned long long)(k - 0x31))
                {
					ActiveOperator2 = k - 0x31;
                    Tip(L"Operator activated");
                    FullRefresh();
                }
                return;
            }
            if (!Alt && Shift && !Control)
            {
				if (xl.ops.size() > (unsigned long long)(k - 0x31))
				{
                    Dirty(1);
                    Push();
                    xl.ops[k - 0x31].Active = !xl.ops[k - 0x31].Active;
                    FullRefresh();
				}
				return;
            }
        }
        if (k == VK_DELETE && FromCmd)
        {
            bool P1 = 0;
            for (size_t i = 0; i < xl.ops.size(); i++)
            {
                auto& op = xl.ops[i];
                for (size_t ii = 0; ii < op.nodes.size(); ii++)
                {
                    if (op.nodes[ii]->bSelected)
                    {
                        if (!P1)
                            Push();
                        P1 = 1;
                        auto c = op.nodes[ii]->ShareMemory;
                        op.nodes[ii]->ShareMemory = 0;

                        for (auto& op2 : xl.ops)
                        {
                            for (auto& n2 : op2.nodes)
                            {
                                if (n2->ShareMemory == -c)
                                {
                                    Dirty(1);
                                    n2->ShareMemory = 0;
                                }
                            }
                        }
                        FullRefresh();
                        return;
                    }

                    if (op.nodes[ii]->S)
                    {
                        if (!P1)
                            Push();
                        P1 = 1;
                        Dirty(1);
                        op.nodes.erase(op.nodes.begin() + ii);
                        if (op.nodes.empty() && xl.ops.size() > 1)
                        {
                            xl.ops.erase(xl.ops.begin() + i);
                            return;
                        }
                        FullRefresh();
                        ii--;
                        continue;
                    }
                    for (size_t iii = 0; iii < op.nodes[ii]->children.size(); iii++)
                    {
                        if (op.nodes[ii]->children[iii].S)
                        {
                            if (!P1)
                                Push();
                            Dirty(1);
                            P1 = 1;
                            op.nodes[ii]->children[iii].g.clear();
                            FullRefresh();
                            return;
                        }
                    }
                }

            }
        }
    }

    void MLGraph::Unselect()
    {
        auto& xl = prj.xl();
        for (auto& op : xl.ops)
        {
            op.S = 0;
			for (auto& nod : op.nodes)
			{
				nod->S = 0;
				for (auto& ch : nod->children)
				{
					ch.S = 0;
				}
				nod->bSelected = 0;
			}
        }
    }

    XL MLGraph::ADefXL()
    {
        XLOP xlop;
        XL xln;

        // Add some defaults
        if (1)
        {
            auto t = std::make_shared<XLNODE_ANY>(0,TYPE_INPUT,1);
            t->hit = D2D1_RECT_F({ 100,100,100,100 });
            t->tensor_dims2 = { "10","10" };
            xlop.nodes.push_back(t);
        }
        if (1)
        {
            auto t = std::make_shared<XLNODE_ANY>(1,TYPE_OUTPUT,0);
            t->hit = D2D1_RECT_F({ 400,400,100,100 });
            xlop.nodes.push_back(t);
        }


        xln.ops.push_back(xlop);
        return xln;
    }



    void MLGraph::OnLoaded(IInspectable, IInspectable)
    {
        ActiveOperator2 = 0;

        auto sp = Content().as<Panel>();
        auto scp = sp.FindName(L"scp").as<SwapChainPanel>();
        auto scv = sp.FindName(L"scv").as<ScrollView>();
        static int Once = 0;
        if (Once)
            return;
        Once = 1;

        if (fil.length())
        {
			current_file = fil;
			XML3::XML x(fil.c_str());
			prj.Unser(x.GetRootElement());

        }
        else
        {
            // Add a default
            prj.xls.clear();


            XL xln = ADefXL();
            prj.xls.push_back(xln);
            prj.iActive = prj.xls.size() - 1;


        }
        auto& xl = prj.xl();
        if (xl.ops.empty())
            xl.ops.push_back(XLOP());


        scp.PointerMoved([this](IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& a)
            {
                bool Left = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                if (!Left)
                {
/*                    if (MouseHoverPT.x > 0)
                    {
                        POINT pt2 = {};
                        GetCursorPos(&pt2);
                        if (pt2.x != MouseHoverPT.x || pt2.y != MouseHoverPT.y)
                        {
                            MouseHoverPT.x = 0;
                            MouseHoverPT.y = 0;
                            //                    WUIEnableDisableTooltip(nullptr);
                              //              ctip[0] = 0;
                        }
                    }
                    else
                    {
                        TRACKMOUSEEVENT tme = {};
                        tme.cbSize = sizeof(tme);
                        tme.hwndTrack = (HWND)wnd();
                        tme.dwFlags = TME_HOVER;
                        tme.dwHoverTime = HOVER_DEFAULT;
                        TrackMouseEvent(&tme);
                    }
*/
                    return;
                }
                auto scp = sender.as<SwapChainPanel>();
                auto pt = a.GetCurrentPoint(scp);
                auto pos = pt.Position();
                auto& xl = prj.xl();


                for (size_t i = 0; i < xl.ops.size(); i++)
                {
                    auto& op = xl.ops[i];
                    for (size_t ii = 0; ii < op.nodes.size(); ii++)
                    {
                        for (size_t iii = 0; iii < op.nodes[ii]->children.size(); iii++)
                        {
                            if (op.nodes[ii]->children[iii].S)
                            {
                                // Line between this and cursor
								D2D1_POINT_2F from = { op.nodes[ii]->children[iii].hit.MiddleX(), op.nodes[ii]->children[iii].hit.MiddleY()};
								D2D1_POINT_2F to = { pos.X, pos.Y };
                                red_from = from;
								red_to = to;

                                Paint();
                                return;
                            }
                            if (op.nodes[ii]->bSelected && MovingNode == 2)
                            {
                                D2D1_POINT_2F from = { op.nodes[ii]->bhit.MiddleX(), op.nodes[ii]->bhit.MiddleY() };
                                D2D1_POINT_2F to = { pos.X, pos.Y };
                                red_from = from;
                                red_to = to;

                                Paint();
                                return;

                            }
                        }
                        if (op.nodes[ii]->S && MovingNode == 1)
                        {
							auto wi = op.nodes[ii]->hit.right - op.nodes[ii]->hit.left;
							auto he = op.nodes[ii]->hit.bottom - op.nodes[ii]->hit.top;

							auto le = pos.X - wi / 2;
							auto to = pos.Y - he / 2;

							op.nodes[ii]->hit = D2D1_RECT_F({ le,to,le + wi,to + he });
                            Paint();
                            return;
                        }
                    }
                }

            });

		scp.PointerReleased([this](IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& a)
			{
                auto scp = sender.as<SwapChainPanel>();
                auto& xl = prj.xl();
                auto pt = a.GetCurrentPoint(scp);
                static auto pos = pt.Position();
                pos = pt.Position();
                if (xl.ops.empty())
                    return;

                if (MovingNode == 2 && MovingNodeP)
                {
                    for (auto& op : xl.ops)
                    {
                        for (auto& nod : op.nodes)
                        {
                            if (nod == MovingNodeP)
                                continue;
							if (Hit(pos.X, pos.Y, nod->bhit2))
							{
								Push();
                                Dirty(1);
                                if (MovingNodeP->ShareMemory == 0)
                                    MovingNodeP->ShareMemory = nextn();
								nod->ShareMemory = -MovingNodeP->ShareMemory;
								FullRefresh();
								return;
							}
                        }
                    }
                }

                for (auto& op : xl.ops)
                {
                    for (auto& nod : op.nodes)
                    {
                        for (auto& ch : nod->children)
                        {
                            if (ch.S && ch.O == 1)
                            {
                                // Connected now
                                for (auto& nod2 : op.nodes)
                                {
                                    for (auto& ch2 : nod2->children)
                                    {
                                        if (ch2.O == 0 && Hit(pos.X, pos.Y, ch2.hit))
                                        {
                                            Push();
                                            Dirty(1);
//                                            ch2.g.clear();
                                            auto ne = nextn();
                                            CONNECTION conn;
                                            conn.key = ne;
                                            ch2.g.push_back(conn);
                                            ch.g.push_back(conn);
                                            nod->S = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                MovingNode = 0;
                red_from = {};
                red_to = {};
                Paint();
            });

        scp.PointerPressed([this](IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& a)
            {
                Tip(0);
                [[maybe_unused]] bool Shift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
                [[maybe_unused]] bool Control = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
                [[maybe_unused]] bool Alt = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);

                bool Right = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
                bool Left = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                if (!Left && !Right)
                    return;
                auto scp = sender.as<SwapChainPanel>();
                auto pt = a.GetCurrentPoint(scp);
                static auto pos = pt.Position();
                pos = pt.Position();
                auto& xl = prj.xl();

                if (Right)
                {
                    for (size_t i = 0; i < xl.ops.size(); i++)
                    {
                        auto& op = xl.ops[i];
                        if (op.Visible == 0)
                            continue;
                        for (size_t ii = 0; ii < op.nodes.size(); ii++)
                        {
                            auto& nod = op.nodes[ii];
                            for (size_t iii = 0; iii < nod->nout(); iii++)
                            {
                                auto& ch = nod->children[iii + nod->nin()];
                                if (Hit(pos.X, pos.Y, ch.hit))
                                {
                                    if (ch.O)
                                    {
                                        // Conditions
                                        if (ch.g.empty())
                                            return;
                                        auto m = BuildNodeRightMenuCondition(nod, &ch,[this, i, ii,iii](const winrt::Windows::Foundation::IInspectable from, const winrt::Windows::Foundation::IInspectable)
                                            {
                                                auto& xl = prj.xl();
                                                auto& op = xl.ops[i];
                                                auto& nod = op.nodes[ii];
												auto& conn = nod->children[iii + nod->nin()];
                                                MenuFlyoutItem m = from.as<MenuFlyoutItem>();
                                                auto t = m.Text();
                                                auto tag = m.Tag();
                                                unsigned long long tagn = _wtoi(winrt::unbox_value<winrt::hstring>(tag).c_str());
												int i1 = (int)(tagn / 100);
												int i2 = (int)(tagn % 100);
                                                if (t == L"Add condition..." && i2 == 0)
                                                {
                                                    WhatInput = 21;
													_i0 = L"Enter Condition:";
													_i1 = L"";
													WhatNode = nod.get();
													WhatBullet = &conn;
													WhatConnection = &conn.g[i1 - 1];
													Refresh({ L"i1",L"i0" });
													auto sp = Content().as<Panel>();
													auto ct = sp.FindName(L"Input1").as<ContentDialog>();
													ct.ShowAsync();
                                                    return;
                                                }
                                                if (t == L"Clear conditions" && i2 == 99)
                                                {
                                                    WhatNode = nod.get();
                                                    WhatBullet = &conn;
                                                    WhatConnection = &conn.g[i1 - 1];
                                                    Push();
                                                    Dirty(1);
                                                    WhatConnection->conditions.clear();
                                                    FullRefresh();
                                                    return;
                                                }

                                                 // edit existing condition
												WhatNode = nod.get();
												WhatBullet = &conn;
												WhatConnection = &conn.g[i1 - 1];
												WhatCondition = &WhatConnection->conditions[i2 - 1];
												WhatInput = 22;
												_i0 = L"Edit Condition:";
												_i1 = XML3::XMLU(WhatCondition->expression.c_str());
												Refresh({ L"i1",L"i0" });
												auto sp = Content().as<Panel>();
												auto ct = sp.FindName(L"Input1").as<ContentDialog>();
												ct.ShowAsync();


                                            });
                                        m.ShowAt(scp, pos);
                                        return;
                                    }
                                    return;
                                }
                            }

                            if (Hit(pos.X, pos.Y, nod->hit))
                            {
                                int ty = 2;
								if (nod->IsInput())
									ty = 1;
                                if (nod->IsOutput())
                                    ty = 3;
                                auto m = BuildNodeRightMenu(xl,nod,ty,[this, i,ii](const winrt::Windows::Foundation::IInspectable from, const winrt::Windows::Foundation::IInspectable)
                                    {
                                        auto& xl = prj.xl();
                                        auto& op = xl.ops[i];
                                        auto& nod = op.nodes[ii];
                                        MenuFlyoutItem m = from.as<MenuFlyoutItem>();
                                        auto t = m.Text();
                                        std::vector<wchar_t> fnx(10000);
										auto tag = m.Tag();
                                        if (tag)
                                        {
                                            // unbox it
											auto i3 = winrt::unbox_value<size_t>(tag);
                                            if (i3 >= 0x100000000)
                                            {
                                                Push();
                                                Dirty(1);
                                                ULARGE_INTEGER ul;
												ul.QuadPart = i3;
                                                auto low = ul.LowPart;
                                                auto pidx = low - 2000;
                                                auto high = ul.HighPart - 1;
                                                nod->Params[pidx].v = std::to_wstring(high);
                                                FullRefresh();
                                            }
                                            else
                                            if (i3 >= 2000)
                                            {
                                                auto pidx = i3 - 2000;
                                                if (nod->Params[pidx].minv == 0 && nod->Params[pidx].maxv == 1)
                                                {
                                                    Push();
                                                    Dirty(1);
                                                    if ((int)nod->Params[pidx] == 0)
														nod->Params[pidx].v = L"1";
													else
														nod->Params[pidx].v = L"0";
													FullRefresh();
                                                }
                                                else
                                                if (nod->Params[pidx].minv <= -1 && nod->Params[pidx].maxv <=  -1) 
                                                {
                                                    WhatInput = 2;
                                                    _i0 = nod->Params[pidx].n;
                                                    _i1 = nod->Params[pidx].v;
                                                    WhatParam = &nod->Params[pidx];
                                                    Refresh({ L"i1",L"i0" });
                                                    auto sp = Content().as<Panel>();
                                                    auto ct = sp.FindName(L"Input1").as<ContentDialog>();
                                                    ct.ShowAsync();

                                                }
                                                else
                                                {
                                                    WhatInput = 2;
                                                    _i0 = nod->Params[pidx].n;
                                                    if (nod->Params[pidx].minv != std::numeric_limits<float>::min() && nod->Params[pidx].maxv != std::numeric_limits<float>::max())
                                                    {
                                                        _i0  += L" (";
														_i0 += std::to_wstring(nod->Params[pidx].minv);
														_i0 += L" - ";
														_i0 += std::to_wstring(nod->Params[pidx].maxv);
														_i0 += L")";
                                                    }
                                                    _i1 = nod->Params[pidx].v;
                                                    WhatParam = &nod->Params[pidx];
                                                    Refresh({ L"i1",L"i0" });
                                                    auto sp = Content().as<Panel>();
                                                    auto ct = sp.FindName(L"Input1").as<ContentDialog>();
                                                    ct.ShowAsync();
                                                }
                                            }
                                        }

                                        for (size_t ig = 1; ig <= MAX_OP_TYPES; ig++)
                                        {
											if (t == optypes[ig])
											{
												Push();
                                                Dirty(1);
                                                nod->OpType = (int)ig;
												FullRefresh();
												return;
											}
                                        }

                                        if (t == L"Tensor shape...")
                                        {
                                            WhatInput = 6;
                                            _i0 = L"Enter Input Tensor Shape:";
                                            if (auto inp = std::dynamic_pointer_cast<XLNODE_ANY>(nod))
                                            {
                                                _i1 = L"";
                                                for (auto& s : inp->tensor_dims())
                                                {
                                                    _i1 += std::to_wstring(s);
                                                    _i1 += L"x";
                                                }
                                            }
											if (_i1.size())
												_i1.pop_back(); 
                                            WhatNode = nod.get();
                                            Refresh({ L"i1",L"i0" });
                                            auto sp = Content().as<Panel>();
                                            auto ct = sp.FindName(L"Input1").as<ContentDialog>();
                                            ct.ShowAsync();
                                            return;
                                        }
                                        if (t == L"Clear input")
                                        {
                                            Push();
                                            auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nod);
											it->csv_input.clear();
											FullRefresh();
                                        }
                                        if (t == L"Binary input...")
                                        {
                                            auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nod);
                                            wcscpy_s(fnx.data(), 10000, it->csv_input.c_str());
                                            OPENFILENAME of = { 0 };
                                            of.lStructSize = sizeof(of);
                                            of.hwndOwner = (HWND)wnd();
                                            of.lpstrFilter = L"*.*\0*.*\0\0";
                                            of.lpstrFile = fnx.data();
                                            of.nMaxFile = 10000;
                                            of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                                            if (!GetOpenFileName(&of))
                                                return;
                                            Push();
                                            it->csv_input = fnx.data();
                                            FullRefresh();
                                        }

                                        if (t == L"CSV input...")
                                        {
											auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nod);
											wcscpy_s(fnx.data(), 10000, it->csv_input.c_str()); 
                                            OPENFILENAME of = { 0 };
                                            of.lStructSize = sizeof(of);
                                            of.hwndOwner = (HWND)wnd();
                                            of.lpstrFilter = L"*.csv\0*.csv\0\0";
                                            of.lpstrFile = fnx.data();
                                            of.nMaxFile = 10000;
                                            of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                                            if (!GetOpenFileName(&of))
                                                return;
                                            Push();
                                            it->csv_input = fnx.data();
                                            FullRefresh();
                                        }
                                        if (t == L"\"Random\"")
                                        {
                                            Push();
                                            auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nod);
                                            it->csv_input = L"\"Random\"";
                                            FullRefresh();
                                        }
                                        if (t == L"\"Sequence\"")
                                        {
                                            Push();
                                            auto it = std::dynamic_pointer_cast<XLNODE_ANY>(nod);
                                            it->csv_input = L"\"Sequence\"";
                                            FullRefresh();
                                        }
                                        if (t == L"Visible Buffer")
                                        {
											nod->BufferVisible = !nod->BufferVisible;
											nod->ShareMemory = 0;
                                            Dirty(1);
                                            FullRefresh();
                                            return;
                                        }
                                        if (t == L"CSV output...")
                                        {
                                            auto it = std::dynamic_pointer_cast<XLNODE>(nod);
                                            wcscpy_s(fnx.data(), 10000, it->csv_output.c_str());
                                            OPENFILENAME of = { 0 };
                                            of.lStructSize = sizeof(of);
                                            of.hwndOwner = (HWND)wnd();
                                            of.lpstrFilter = L"*.csv\0*.csv\0\0";
                                            of.lpstrFile = fnx.data();
											of.lpstrDefExt = L"csv";    
                                            of.nMaxFile = 10000;
                                            of.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                                            if (!GetSaveFileName(&of))
                                                return;
                                            Push();
                                            it->csv_output = fnx.data();
                                            FullRefresh();
                                        }
                                        if (t == L"\"Screen\"")
                                        {
                                            auto it = std::dynamic_pointer_cast<XLNODE>(nod);
                                            Push();
                                            it->csv_output = L"\"Screen\"";
                                            FullRefresh();
                                        }
                                        if (t == L"Binary output...")
                                        {
                                            auto it = std::dynamic_pointer_cast<XLNODE>(nod);
                                            wcscpy_s(fnx.data(), 10000, it->csv_output.c_str());
                                            OPENFILENAME of = { 0 };
                                            of.lStructSize = sizeof(of);
                                            of.hwndOwner = (HWND)wnd();
                                            of.lpstrFilter = L"*.*\0*.*\0\0";
                                            of.lpstrFile = fnx.data();
                                            of.nMaxFile = 10000;
                                            of.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                                            if (!GetSaveFileName(&of))
                                                return;
                                            Push();
                                            it->csv_output = fnx.data();
                                            FullRefresh();
                                        }
                                        if (t == L"Clear output")
                                        {
                                            Push();
                                            nod->csv_output.clear();
                                            FullRefresh();
                                        }

                                    });
                                m.ShowAt(scp, pos);
                                return;
                            }
                        }

						if (i != ActiveOperator2)
							continue;

                        auto m = BuildTensorMenu([this,i](const winrt::Windows::Foundation::IInspectable from, const winrt::Windows::Foundation::IInspectable)
                            {
                                auto& xl = prj.xl();
                                auto& op = xl.ops[i];

                                MenuFlyoutItem m = from.as<MenuFlyoutItem>();
                                auto t = m.Text();
                                if (t == L"Input")
                                {
                                    OnAddInput({}, {});
                                }

                                if (t == L"ActivationCelu")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_CELU);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"1";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationElu")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_ELU);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"1";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationGelu")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_GELU);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationHardmax")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_HARDMAX);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationHardSigmoid")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_HARDSIGMOID);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"0.2";
                                    node->Params[1].n = L"Beta";
                                    node->Params[1].v = L"0.5";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationIdentity")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_IDENTITY);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationLeakyRelu")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_LEAKYRELU);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"0.01";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationLinear")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_LINEAR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"0";
                                    node->Params[1].n = L"Beta";
                                    node->Params[1].v = L"0";
                                    Push();
                                    op.nodes.push_back(node);
                                }
								if (t == L"ActivationLogSoftmax")
								{
									auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_LOGSOFTMAX);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
									op.nodes.push_back(node);
								}
								if (t == L"ActivationParameterizedRelu")
								{
									auto node = std::make_shared<XLNODE_ANY>(2, TYPE_ACT_PRELU);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
								}
                                if (t == L"ActivationParametricSoftplus")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_PSOFTPLUS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"0";
                                    node->Params[1].n = L"Beta";
                                    node->Params[1].v = L"0";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationRelu")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_RELU);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationScaledElu")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_SELU);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"1.67326319217681884765625";
                                    node->Params[1].n = L"Gamma";
                                    node->Params[1].v = L"1.05070102214813232421875";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ActivationScaledTanh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_STANH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Alpha";
                                    node->Params[0].v = L"1.0";
                                    node->Params[1].n = L"Beta";
                                    node->Params[1].v = L"0.5";
                                    Push();
                                    op.nodes.push_back(node);
                                }
								if (t == L"ActivationShrink")
								{
									auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_SHRINK);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Bias";
                                    node->Params[0].v = L"0";
                                    node->Params[1].n = L"Threshold";
                                    node->Params[1].v = L"0.5";
                                    Push();
                                    op.nodes.push_back(node);
								}
                                if (t == L"ActivationSigmoid")
                                {
									auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_SIGMOID);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
								if (t == L"ActivationSoftmax")
								{
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_SOFTMAX);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
								}
								if (t == L"ActivationSoftplus")
								{
									auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_SOFTPLUS);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Steepness";
                                    node->Params[0].v = L"1.0";
                                    Push();
                                    op.nodes.push_back(node);
								}
                                if (t == L"ActivationSoftsign")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_SOFTSIGN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
								if (t == L"ActivationTanh")
								{
    								auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_TANH);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
								}
								if (t == L"ActivationThresholdedRelu")
								{
									auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ACT_TRELU);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
									node->Params.resize(1);
									node->Params[0].n = L"Alpha";
									node->Params[0].v = L"1";
                                    Push();
                                    op.nodes.push_back(node);
								}





                                if (t == L"Abs")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_ABS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ACos")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_ACOS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ACosh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_ACOSH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"And")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_LAND);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"ASin")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ASIN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ASinh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ASINH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ATan")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ATAN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ATanh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ATANH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ATanYX")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2,TYPE_ATANYX);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"AveragePooling")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_AVERAGEPOOLING);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;

                                    //expr = dml::AveragePooling(mop.Item(whati[0]), TensorFromString<unsigned int>(it->Params[0]), TensorFromString<unsigned int>(it->Params[1]), TensorFromString<unsigned int>(it->Params[2]), TensorFromString<unsigned int>(it->Params[3]), TensorFromString<unsigned int>(it->Params[4]), (bool)it->Params[5], TensorFromString<unsigned int>(it->Params[6]));
									node->Params.resize(7); 
									node->Params[0].n = L"Strides";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;

									node->Params[1].n = L"Window Sizes";
									node->Params[1].minv = -1;
									node->Params[1].maxv = -1;

									node->Params[2].n = L"Start Pad";
									node->Params[2].minv = -1;
									node->Params[2].maxv = -1;

									node->Params[3].n = L"End Pad";
									node->Params[3].minv = -1;
									node->Params[3].maxv = -1;

									node->Params[4].n = L"Dilations";
									node->Params[4].minv = -1;
									node->Params[4].maxv = -1;

									node->Params[5].n = L"Include Padding";
									node->Params[5].v = L"0";
									node->Params[5].minv = 0;
									node->Params[5].maxv = 1;


									node->Params[6].n = L"Output Sizes";
									node->Params[6].minv = -1;
									node->Params[6].maxv = -1;
                                        


                                    Push();
                                    op.nodes.push_back(node);

                                }

                                if (t == L"Add")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_ADD);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"BitAnd")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITAND);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitCount")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_BITAND);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitNot")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_BITNOT);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitOr")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitShiftLeft")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITSL);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitShiftRight")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITSR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitXor")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITXOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BatchNormalization")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(5, TYPE_BATCHNORMALIZATION);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Spatial";
                                    node->Params[0].v = L"0.0";
                                    node->Params[1].n = L"Epsilon";
                                    node->Params[1].v = L"0.0";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BatchNormalizationGrad")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(5, TYPE_BATCHNORMALIZATIONGRAD,3);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Epsilon";
                                    node->Params[0].v = L"0.0";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BatchNormalizationTraining")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_BATCHNORMALIZATIONTRAINING, 3);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Epsilon";
                                    node->Params[0].v = L"0.0";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BatchNormalizationTrainingGrad")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(5, TYPE_BATCHNORMALIZATIONTRAININGGRAD, 3);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Epsilon";
                                    node->Params[0].v = L"0.0";
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Cast")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_CAST);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Ceil")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_CEIL);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Clip")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_CLIP);

                                    node->Params.resize(2);
                                    node->Params[0].n = L"Min";
                                    node->Params[0].n = L"Max";
                                    node->Params[1].v = L"1";

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ClipGrad")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_CLIPGRAD);

                                    node->Params.resize(2);
                                    node->Params[0].n = L"Min";
                                    node->Params[0].n = L"Max";
                                    node->Params[1].v = L"1";

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Constant")
                                {
                                    OnAddConstant({}, {});
                                }
                                if (t == L"DiagonalMatrix")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(0, TYPE_DIAGONALMATRIX, 1);
                                    node->Params.push_back({ L"Value",L"0" });
                                    node->Params.push_back({ L"Offset",L"0" });
                                    node->hit = D2D1_RECT_F({ 10,10,100,100 });
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
									node->SetTensorDims({ 10,10 });   
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ConvolutionInteger")
                                {
									auto node = std::make_shared<XLNODE_ANY>(4, TYPE_CONVOLUTIONINTEGER);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;

                                    node->Params.resize(6);
                                    for (int ii = 0; ii < 6; ii++)
                                    {
                                        node->Params[ii].minv = -1;
                                        node->Params[ii].maxv = -1;
                                        if (ii == 4)
                                        {
                                            node->Params[ii].minv = 0;
                                            node->Params[ii].maxv = 1;
                                        }
                                    }
                                    node->Params[0].n = L"Strides";
                                    node->Params[1].n = L"Dilations";
                                    node->Params[2].n = L"Start Pad";
                                    node->Params[3].n = L"End Pad";
                                    node->Params[4].n = L"Group Count";
                                    node->Params[5].n = L"Output Sizes";
                                    Push();
                                    op.nodes.push_back(node);


                                    Push();
									op.nodes.push_back(node);
                                }
                                if (t == L"Cos")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_COS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Cosh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_COSH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Convolution")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_CONVOLUTION);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Mode";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 1;
                                    node->Params[0].list_names = { L"No Cross",L"Cross" };
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"CummulativeSum")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_CUMSUM);
                                    node->Params.resize(3);
                                    node->Params[0].n = L"Axis";
                                    node->Params[1].n = L"Decreasing";
                                    node->Params[1].minv = 0.0f;
                                    node->Params[1].maxv = 1.0f;
                                    node->Params[2].n = L"Exclude Current";
                                    node->Params[2].minv = 0.0f;
                                    node->Params[2].maxv = 1.0f;

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"CummulativeProduct")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_CUMPROD);
                                    node->Params.resize(3);
                                    node->Params[0].n = L"Axis";
                                    node->Params[1].n = L"Decreasing";
                                    node->Params[1].minv = 0.0f;
                                    node->Params[1].maxv = 1.0f;
                                    node->Params[2].n = L"Exclude Current";
                                    node->Params[2].minv = 0.0f;
                                    node->Params[2].maxv = 1.0f;

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Divide")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_DIVIDE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"DepthToSpace")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_DEPTHTOSPACE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;

                                    node->Params.resize(2);
									node->Params[0].n = L"Block Size";
									node->Params[0].v = L"1";
                                    node->Params[1].n = L"Mode";
                                    node->Params[1].minv = 0;
                                    node->Params[1].maxv = 1;
                                    node->Params[1].list_names = { L"DEPTH_COLUMN_ROW",L"COLUMN_ROW_DEPTH"};


                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Dequantize")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(10, TYPE_DEQUANTIZE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Mode";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 1;
                                    node->Params[0].list_names = { L"Scale",L"Zero Point"};
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"DequantizeLinear")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_DEQUANTIZELINEAR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                
                                if (t == L"DifferenceSquare")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_DIFFERENCESQUARE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Erf")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ERF);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Exp")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_EXP);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Equals")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_EQUALS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Floor")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_FLOOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Gather")
                                {
									auto node = std::make_shared<XLNODE_ANY>(2, TYPE_GATHER);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
									node->Params.resize(2);
									node->Params[0].n = L"Axis";
                                    node->Params[1].n = L"Index Dimensions";
                                    Push();
									op.nodes.push_back(node);
                                }
								if (t == L"GatherElements")
								{
									auto node = std::make_shared<XLNODE_ANY>(2, TYPE_GATHERELEMENTS);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Axis";
                                    Push();
									op.nodes.push_back(node);
								}
                                if (t == L"GatherND")
                                {
									auto node = std::make_shared<XLNODE_ANY>(2, TYPE_GATHERND);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
									node->Params.resize(3);
                                    node->Params[0].n = L"Input Dimension Count";
                                    node->Params[1].n = L"Indices Dimension Count";
                                    node->Params[2].n = L"Batch Dimension Count";
									Push();
									op.nodes.push_back(node);
                                }


                                if (t == L"Gemm")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_GEMM);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(4);
                                    node->Params[0].n = L"Transpose 1";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 1;
                                    node->Params[1].n = L"Transpose 2";
                                    node->Params[1].minv = 0;
                                    node->Params[1].maxv = 1;
                                    node->Params[2].n = L"Alpha";
                                    node->Params[3].n = L"Beta";
                                    node->Params[2].v = L"1";
                                    node->Params[3].v = L"1";
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Gru")
                                {
                                    // TODO: It's fused
/*                                    auto node = std::make_shared<XLNODE_ANY>(6, TYPE_GRU, 3);
                                    node->Params.resize(6);

                                    for (int ii = 0; ii < 6; ii++)
                                    {
                                        if (ii != 4)
                                        {
                                            node->Params[ii].minv = -1;
                                            node->Params[ii].maxv = -1;
                                        }
                                    }

                                    node->Params[0].n = L"Strides";
                                    node->Params[1].n = L"Dilations";
                                    node->Params[2].n = L"Start Pad";
                                    node->Params[3].n = L"End Pad";
                                    node->Params[4].n = L"Group Count";
                                    node->Params[4].v = L"1.0";
                                    node->Params[5].n = L"Output Tensor Sizes";
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                    */
                                }

                                if (t == L"GreaterThan")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_GREATERTHAN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"GreaterThanOrEqual")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_GREATERTHANOREQUAL);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Identity")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_IDENTITY);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"IsInfinity")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ISINFINITY);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Mode";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 2;
									node->Params[0].list_names = { L"Either",L"Positive Infinity",L"Negative Infinity" };
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"IsNan")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ISNAN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Join")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(10, TYPE_JOIN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Axis";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 100;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"If")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_IF);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Log")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_LOG);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"LocalResponseNormalization")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_LOCALRESPONSENORMALIZATION);
                                    node->Params.resize(5);
                                    node->Params[0].n = L"Cross Channel ";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 1;
                                    node->Params[1].n = L"Local Size";
                                    node->Params[2].n = L"Alpha";
                                    node->Params[3].n = L"Beta";
                                    node->Params[4].n = L"Bias";
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"LessThan")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_LESSTHAN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"LessThanOrEqual")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_LESSTHANOREQUAL);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Max")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MAX);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"MaxPooling")
                                {
									auto node = std::make_shared<XLNODE_ANY>(1, TYPE_MAXPOOLING,2);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
									node->Params.resize(7);
                                    for (int ii = 0; ii < 7; ii++)
                                    {
										node->Params[ii].minv = -1;
										node->Params[ii].maxv = -1;
                                        if (ii == 5)
                                        {
											node->Params[ii].minv = 0;
											node->Params[ii].maxv = 1;
                                        }
                                    }
									node->Params[0].n = L"Window Size";
									node->Params[1].n = L"Strides";
									node->Params[2].n = L"Start Pad";
									node->Params[3].n = L"End Pad";
									node->Params[4].n = L"Dilations";
									node->Params[5].n = L"Output Indices";
									node->Params[6].n = L"Output Sizes";
									Push();
									op.nodes.push_back(node);
                                }
                                if (t == L"Mean")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MEAN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"MeanVarianceNormalization")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_MEANVARIANCENORMALIZATION);
                                    node->Params.resize(4);
                                    node->Params[0].n = L"Axes";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    node->Params[1].n = L"Normalize Variance";
                                    node->Params[2].n = L"Normalize Mean";
                                    node->Params[3].n = L"Epsilon";
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Min")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MIN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Multiply")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MULTIPLY);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ModulusFloor")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MODULUSFLOOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
								if (t == L"ModulusTruncate")
								{
									auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MODULUSTRUNCATE);
									node->hit.left = pos.X;
									node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
								}
                                if (t == L"Neg")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_NEGATE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"NonZeroCoordinates")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_NONZEROCOORDINATES,2);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Not")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_LNOT);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"OneHot")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_ONEHOT);
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Output Length";
                                    node->Params[0].v = L"1.0";
                                    node->Params[1].n = L"Axis";
                                    node->Params[1].v = L"1.0";
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Or")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_LOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Padding")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_PADDING);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(4);
                                    node->Params[0].n = L"Padding Mode";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 2;
									node->Params[0].list_names = { L"Constant",L"Edge",L"Reflection"};
                                    node->Params[1].n = L"Padding Value";
                                    node->Params[2].n = L"Start Padding";
                                    node->Params[2].minv = -1;
                                    node->Params[2].maxv = -1;
                                    node->Params[3].n = L"End Padding";
                                    node->Params[3].minv = -1;
                                    node->Params[3].maxv = -1;
                                    Push();
                                    op.nodes.push_back(node);

                                }
                                if (t == L"Pow")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_POW);
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Exponent";
                                    node->Params[0].v = L"1.0";
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"QuantizeLinear")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_QUANTIZELINEAR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"QuantizedLinearConvolution")
                                {
                                    // 5 forced input descriptions, 4 optional input descriptions, datatype, strides-dilations-start-endpdding tensors, group count,output tensor
                                    auto node = std::make_shared<XLNODE_ANY>(9, TYPE_QUANTIZEDLINEARCONVOLUTION);
                                    node->Params.resize(6);
                                    for (int ii = 0; ii < 6; ii++)
                                    {
                                        if (ii != 4)
                                        {
                                            node->Params[ii].minv = -1;
                                            node->Params[ii].maxv = -1;
                                        }
                                    }

                                    node->Params[0].n = L"Strides";
                                    node->Params[1].n = L"Dilations";
                                    node->Params[2].n = L"Start Pad";
                                    node->Params[3].n = L"End Pad";
                                    node->Params[4].n = L"Group Count";
                                    node->Params[4].v = L"1.0";
                                    node->Params[5].n = L"Output Tensor Sizes";
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"RandomGenerator")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_RANDOMGENERATOR,2);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Output Tensor";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    node->Params[1].n = L"Output State";
                                    node->Params[1].minv = 0;
                                    node->Params[1].maxv = 1;
                                    node->Params[1].v = L"1";

                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Recip")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_RECIP);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Reduce")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_REDUCE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Function";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 11;
                                    node->Params[0].list_names = { L"ArgMax",L"ArgMin",L"Average",L"L1",L"L2",L"LogSum",L"LogSumExp",L"Max",L"Min",L"Multiply",L"Sum",L"SumSquare"};
                                    node->Params[1].n = L"Axes";
                                    node->Params[1].minv = -1;
                                    node->Params[1].maxv = -1;
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Resample")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_RESAMPLE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(4);
                                    node->Params[0].n = L"Output Tensor";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    node->Params[1].n = L"Interpolation";
                                    node->Params[1].minv = 0;
                                    node->Params[1].maxv = 1;
                                    node->Params[1].list_names = { L"Nearest",L"Linear" };
                                    node->Params[2].n = L"Axis Direction";
                                    node->Params[2].minv = 0;
                                    node->Params[2].maxv = 1;
                                    node->Params[2].list_names = { L"Increasing",L"Decreasing" };
                                    node->Params[3].n = L"Scales";
                                    node->Params[3].minv = -1;
                                    node->Params[3].maxv = -1;

                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ResampleGrad")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_RESAMPLEGRAD);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(3);
                                    node->Params[0].n = L"Output Tensor";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    node->Params[1].n = L"Interpolation";
                                    node->Params[1].minv = 0;
                                    node->Params[1].maxv = 1;
                                    node->Params[1].list_names = { L"Nearest",L"Linear" };
                                    node->Params[2].n = L"Scales";
                                    node->Params[2].minv = -1;
                                    node->Params[2].maxv = -1;

                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Round")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ROUND);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Mode";
                                    node->Params[0].minv = 0;
                                    node->Params[0].maxv = 2;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"RoiAlign")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_ROIALIGN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(12);
                                    std::vector<std::wstring> pn = {
                                        L"Reduction Function",L"Interpolation", L"Spatial Scale X", L"Spatial Scale Y", 
                                        L"Output Pixel Offset", L"Output Bounds Offset", L"Out of bounds input value",
                                        L"minSpO", L"maxSpO", L"AlignRegionsToCorners", L"Height", L"Width"};
                                    for (int ii = 0; ii < 12; ii++)
                                    {
                                        node->Params[ii].n = pn[ii];
                                        if (i == 0)
                                        {
											node->Params[ii].minv = 0;
											node->Params[ii].maxv = 11;
                                            node->Params[ii].list_names = { L"ArgMax",L"ArgMin",L"Average",L"L1",L"L2",L"LogSum",L"LogSumExp",L"Max",L"Min",L"Multiply",L"Sum",L"SumSquare" };
                                        }
                                        if (i == 1)
                                        {
                                            node->Params[ii].minv = 0;
                                            node->Params[ii].maxv = 2;
                                            node->Params[ii].list_names = { L"None",L"Nearest",L"Linear" };
                                        }
                                    }
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"RoiAlignGrad")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(4, TYPE_ROIALIGNGRAD,2);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(14);
                                    std::vector<std::wstring> pn = {
                                        L"Reduction Function",L"Interpolation", L"Spatial Scale X", L"Spatial Scale Y",
                                        L"Output Pixel Offset", L"Output Bounds Offset", L"Out of bounds input value",
                                        L"minSpO", L"maxSpO", L"AlignRegionsToCorners", L"Height", L"Width",
                                    L"Compute Output Gradient",L"Compute Output ROI Gradient"};
                                    for (int ii = 0; ii < 14; ii++)
                                    {
                                        node->Params[ii].n = pn[ii];
                                        if (i == 0)
                                        {
                                            node->Params[ii].minv = 0;
                                            node->Params[ii].maxv = 11;
                                            node->Params[ii].list_names = { L"ArgMax",L"ArgMin",L"Average",L"L1",L"L2",L"LogSum",L"LogSumExp",L"Max",L"Min",L"Multiply",L"Sum",L"SumSquare" };
                                        }
                                        if (i == 1)
                                        {
                                            node->Params[ii].minv = 0;
                                            node->Params[ii].maxv = 2;
                                            node->Params[ii].list_names = { L"None",L"Nearest",L"Linear" };
                                        }
                                    }
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Reinterpret")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_REINTERPRET);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"New Size";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"ReverseSubsequences")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_REVERSESUBSEQUENCES);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Axis";
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"ScatterElements")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(3, TYPE_SCATTERELEMENTS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Axis";
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Slice")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_SLICE);
                                    node->Params.resize(3);
                                    node->Params[0].n = L"Offsets";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    node->Params[1].n = L"Sizes";
                                    node->Params[1].minv = -1;
                                    node->Params[1].maxv = -1;
                                    node->Params[2].n = L"Strides";
                                    node->Params[2].minv = -1;
                                    node->Params[2].maxv = -1;

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"SliceGrad")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_SLICEGRAD);
                                    node->Params.resize(4);
                                    node->Params[0].n = L"Output Gradient Sizes";
                                    node->Params[0].minv = -1;
                                    node->Params[0].maxv = -1;
                                    node->Params[1].n = L"Offsets";
                                    node->Params[1].minv = -1;
                                    node->Params[1].maxv = -1;
                                    node->Params[2].n = L"Sizes";
                                    node->Params[2].minv = -1;
                                    node->Params[2].maxv = -1;
                                    node->Params[3].n = L"Strides";
                                    node->Params[3].minv = -1;
                                    node->Params[3].maxv = -1;

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Subtract")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_SUBTRACT);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"SpaceToDepth")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_SPACETODEPTH);
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Block Size";
                                    node->Params[1].n = L"Order";
                                    node->Params[1].minv = 0;
                                    node->Params[1].maxv = 1;
                                    node->Params[1].list_names = { L"DEPTH_COLUMN_ROW",L"COLUMN_ROW_DEPTH" };
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }



                                if (t == L"Sqrt")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_SQRT);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Sign")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_SIGN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Threshold")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_THRESHOLD);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(1);
                                    node->Params[0].n = L"Minimum";
                                    Push();
                                    op.nodes.push_back(node);
                                }
                                if (t == L"TopK")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_TOPK,2);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(3);
                                    node->Params[0].n = L"Axis";
                                    node->Params[1].n = L"K";
                                    node->Params[2].n = L"Axis Direction";
                                    node->Params[2].minv = 0;
                                    node->Params[2].maxv = 1;
                                    node->Params[2].list_names = { L"Increasing",L"Decreasing" };
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Upsample2D")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_UPSAMLPLE2D);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(3);
                                    node->Params[0].n = L"ScaleX";
                                    node->Params[1].n = L"ScaleY";
                                    node->Params[2].n = L"Interpolation";
                                    node->Params[2].minv = 0;
                                    node->Params[2].maxv = 2;
                                    node->Params[2].list_names = { L"None",L"Nearest",L"Linear"};
                                    Push();
                                    op.nodes.push_back(node);
                                }

                                if (t == L"ValueScale2D")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_VALUESCALE2D);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    node->Params.resize(2);
                                    node->Params[0].n = L"Scale";
                                    node->Params[1].minv = -1;
                                    node->Params[1].maxv = -1;
                                    node->Params[1].n = L"Bias";
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Xor")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_LXOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Output")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_OUTPUT,0);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    Push();
                                    op.nodes.push_back(node);
                                }



                                Refresh();
                            });
                        m.ShowAt(scp, pos);
                    }
                    return;
                }


                if (!Shift)
                   Unselect();

				for (size_t i = 0; i < xl.ops.size(); i++)
				{
					auto& op = xl.ops[i];
                    if (op.Visible == 0)
                        continue;

                    for (size_t ii = 0; ii < op.nodes.size(); ii++)
                    {
                        for (size_t iii = 0; iii < op.nodes[ii]->children.size(); iii++ )
                        {
							auto& ch = op.nodes[ii]->children[iii];
							if (Hit(pos.X, pos.Y, ch.hit))
							{
                                if (ch.O == 1)
                                {
                                    ch.S = 1;
                                    auto numout = op.nodes[ii]->nout();
                                    if (numout > 1 && ch.name.length())
                                        Tip(ch.name.c_str());
                                }
                                else
                                {
                                    // Tool tip
                                    auto numin = op.nodes[ii]->nin();
                                    if (numin == 1 || ch.name.empty())
                                    {
                                    }
                                    else
                                        Tip(ch.name.c_str());
                                }
								return;
							}
                        }
                        if (Hit(pos.X, pos.Y, op.nodes[ii]->bhit))
                        {
                            MovingNodeP = op.nodes[ii];
                            Tip(L"Memory out");

                            MovingNode = 2;
                            op.nodes[ii]->bSelected = 1;
                            break;
                        }
                        if (Hit(pos.X, pos.Y, op.nodes[ii]->bhit2))
                        {
                            Tip(L"Memory in");
                        }

                        if (Hit(pos.X, pos.Y, op.nodes[ii]->hit))
                        {
                            MovingNode = 1;
                            op.nodes[ii]->S = 1;
                            if (!Shift)
                            {
                                if (ActiveOperator2 != i)
                                {
                                    ActiveOperator2 = i;
									RefreshMenu();
                                }
                            }
                            break;
                        }
                    }
				}
                Paint();
            });

        Resize();
        RefreshMenu();
    }

    void MLGraph::LoadAdapters()
    {
		if (all_adapters.empty())
		{
			CComPtr<IDXGIFactory1> dxgiFactory;
			CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgiFactory));
			CComPtr<IDXGIAdapter1> adapterq;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapterq) != DXGI_ERROR_NOT_FOUND; ++i)
			{
                DXGI_ADAPTER_DESC1 desc = {};
                adapterq->GetDesc1(&desc);
				all_adapters.push_back(adapterq);
                adapterq = 0;
			}
        }
    }

    void MLGraph::Resize()
    {
        LoadAdapters();
        auto sp = Content().as<Panel>();
        auto men = sp.FindName(L"menu").as<MenuBar>();
        RECT rc = {};
        GetClientRect((HWND)wnd(), & rc);
        if (rc.right <= 5 || rc.bottom <= 5)
            return;
        auto he = (float)(rc.bottom - rc.top);
        auto wi = (float)(rc.right - rc.left);

        float dpi_scale = (float)GetDpiForWindow((HWND)wnd()) / 96;
		if (dpi_scale < 1)
			dpi_scale = 1;  
		wi = wi / dpi_scale;
		he = he / dpi_scale;

        auto scp = sp.FindName(L"scp").as<SwapChainPanel>();
        auto scv = sp.FindName(L"scv").as<ScrollView>();
        auto menh = men.ActualHeight();
        if (menh < 0)
            return;
        he -= (float)menh;

		auto wnd2 = windows[(HWND)wnd()];
        if (wnd2)
        {
            auto topj = wnd2.as<MainWindow>().Content().as<NavigationView>();
            if (topj)
            {
                wi -= 50;
            }
        }
//        auto sp1 = sp.FindName(L"sp1").as<StackPanel>();
 //       auto sp2 = sp.FindName(L"sp2").as<StackPanel>();
  //      he -= (float)(sp1.ActualHeight() + sp2.ActualHeight());

//        he -= 100;

//        scp.Focus(FocusState::Keyboard);


        scp.Width(wi*ScaleX);
        scp.Height(he * ScaleX);

        scv.Width(wi);
        scv.Height(he);

        if (d2d)
        {
            if (d2d->SizeCreated.cx != (wi * ScaleX) || d2d->SizeCreated.cy != (he * ScaleX))
            {
//                d2d->Resize((int)(wi * ScaleX), (int)(wi * ScaleX));
 //               d2d->Resize2((int)(wi*ScaleX),(int)(wi*ScaleX));
//				d2d->SizeCreated.cx = (int)(wi * ScaleX);
//				d2d->SizeCreated.cy = (int)(he * ScaleX);
                d2d->Off();
                d2d = 0;
            }
        }

        if (d2d)
        {
            if (d2d->m_d2dContext5 == 0)
            {
                d2d = 0;
            }
        }

        if (!d2d)
        {
            d2d = std::make_shared<D2D>();
            d2d->CreateD2X(0,0, (int)(wi * ScaleX), (int)(he * ScaleX), 1, 0, 0, 1);
        }
        else
            d2d->Resize((int)(wi * ScaleX), (int)(he * ScaleX));


        IInspectable i = (IInspectable)scp;
        auto p2 = i.as<ISwapChainPanelNative>();
        p2->SetSwapChain(d2d->m_swapChain1);
        iVisiblePage = 0;

        Paint();
    }

    void MLGraph::Refresh(const wchar_t* s)
    {
        m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ s });
        Paint();
    }
    void MLGraph::Refresh(std::vector<std::wstring> strs)
    {
        if (strs.empty())
            m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"" });
        else
        {
            for (auto& s : strs)
                m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ s });
        }
        Paint();
    }

    std::vector<XML3::XMLElement> clipboard;
    void MLGraph::OnCopy(IInspectable const&, IInspectable const&)
    {
        clipboard.clear();
        auto& xl = prj.xl();
        for (auto& op : xl.ops)
		{
            for (auto& n : op.nodes)
            {
				if (n->S)
				{
					XML3::XMLElement el;
					n->Ser(el);
					clipboard.push_back(el);
				}

            }
		}   
    }
    void MLGraph::OnPaste(IInspectable const&, IInspectable const&)
    {
        bool p1 = 0;
        POINT pos = {};
        GetCursorPos(&pos);
        ScreenToClient((HWND)wnd(), &pos);
        if (pos.x > 50)
            pos.x -= 50;
        if (pos.y > 50)
            pos.y -= 50;
        for (auto& e : clipboard)
        {
			if (!p1)
			{
				Push();
                Dirty(1);
                p1 = 1;
			}

            auto& xl = prj.xl();
            auto& op = xl.ops[ActiveOperator2];
            auto n = op.Unser2(e);
			for (auto& e3 : n->children)
				e3.g.clear();
            n->hit.left = 0; n->hit.top = 0;
			n->hit.left = (float)pos.x;
			n->hit.top = (float)pos.y;
            op.nodes.push_back(n);
        }
		FullRefresh();
    }
    void MLGraph::OnDelete(IInspectable const&, IInspectable const&)
    {
		Key(VK_DELETE,true);
    }

    void MLGraph::OnUndo(IInspectable const&, IInspectable const&)
    {
		if (undo_list.empty())
			return;
        XML3::XMLElement el;
        auto& xl = prj.xl();
        xl.Ser(el);
		redo_list.push(el);
		el = undo_list.top();
		undo_list.pop();
        xl.Unser(el);
		Refresh();
        RefreshMenu();

    }
    void MLGraph::OnRedo(IInspectable const&, IInspectable const&)
    {
		if (redo_list.empty())
			return;
        XML3::XMLElement el;
        auto& xl = prj.xl();
        xl.Ser(el);
        undo_list.push(el);
	    el = redo_list.top();
		redo_list.pop();
		xl.Unser(el);
		Refresh();
		RefreshMenu();
    }


    void MLGraph::UpdateVideoMemory()
    {
        CComPtr<IDXGIAdapter3> d3;

        CComPtr<IDXGIAdapter> ad;
        auto iAdapter = SettingsX->GetRootElement().vv("iAdapter").GetValueInt(0);
        if (iAdapter > 0 && iAdapter <= all_adapters.size())
            d3 = all_adapters[iAdapter - 1];
        if (!d3 && d2d)
            d3 = d2d->dxgiAdapter;
        if (d3)
        {
            d3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vmi);
            d3->GetDesc1(&this->vdesc);
        }
    }

    void MLGraph::Paint()
    {
        wchar_t wt[1000] = {};

        LoadAdapters();

        auto sp = Content().as<Panel>();
        auto scp = sp.FindName(L"scp").as<SwapChainPanel>();
        if (d2d)
        {
            if (d2d->m_d2dContext == 0)
            {
                d2d = 0;
                auto wi = sp.ActualWidth();
                auto he = sp.ActualHeight();
                d2d = std::make_shared<D2D>();
                d2d->CreateD2X(0, 0, (int)wi, (int)he, 1, 0, 0, 1);
            }

        }
        if (!d2d)
            return;
        if (!d2d->m_d2dContext)
            return;

        if (iVisiblePage != 0)
        {
            Resize();
            return;
        }

        ClearRectsAndTips();
        d2d->m_d2dContext->BeginDraw();
        d2d->m_d2dContext->Clear({ 1,1,1,1 });
        float dpi_scale = (float)GetDpiForWindow((HWND)wnd()) / 96;
        if (dpi_scale < 1)
            dpi_scale = 1;


        if (!d2d->WhiteBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 1,1,1,1 }, &d2d->WhiteBrush);
        if (!d2d->BlackBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0,0,0,1 }, &d2d->BlackBrush);
        if (!d2d->RedBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 1,0,0,1 }, &d2d->RedBrush);
        if (!d2d->CyanBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0,0,1,1 }, &d2d->CyanBrush);
        if (!d2d->GreenBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0,1,0,1 }, &d2d->GreenBrush);
        if (!d2d->YellowBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 1,1,0,1 }, &d2d->YellowBrush);
        if (!d2d->SnapBrush2)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0.5f,0.5f,0.5f,1 }, &d2d->SnapBrush2);

        if (!d2d->Text)
        {
            LOGFONT lf;
            GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
            DWRITE_FONT_STYLE fst = DWRITE_FONT_STYLE_NORMAL;
            DWRITE_FONT_STRETCH fsr = DWRITE_FONT_STRETCH_NORMAL;
            FLOAT fs = (FLOAT)fabs(lf.lfHeight);
            fs *= 2.0f / dpi_scale;
            d2d->WriteFa->CreateTextFormat(lf.lfFaceName, 0, lf.lfWeight > 500 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, fst, fsr, fs, L"", &d2d->Text);
            fs /= 1.5f;
            d2d->WriteFa->CreateTextFormat(lf.lfFaceName, 0, lf.lfWeight > 500 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, fst, fsr, fs, L"", &d2d->Text2);
            fs *= 2.0f;
            d2d->WriteFa->CreateTextFormat(lf.lfFaceName, 0, lf.lfWeight > 500 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, fst, fsr, fs, L"", &d2d->Text3);
        }
        D2D1_RECT_F rfull = { 0,0,(float)scp.ActualWidth(),(float)scp.ActualHeight() };
        rfull.bottom = (float)scp.Height();
        auto r = d2d->m_d2dContext5;

#ifdef _DEBUG
		r->DrawRectangle(rfull, d2d->BlackBrush,10);   
#endif


        if (1)
        {
            if (vmi.Budget == 0)
            {
                UpdateVideoMemory();
            }

			swprintf_s(wt, L"%s - Video Memory: %I64u MB, Usage: %I64u MB", vdesc.Description ? vdesc.Description : L"Default Adapter", vmi.Budget / (1024 * 1024),vmi.CurrentUsage / (1024*1024));
			auto msr = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text2, wt,-1);
            if (rfull.right > std::get<0>(msr))
            {
                D2D1_RECT_F r1 = { 0,0,std::max(std::get<0>(msr) + 10,rfull.right),std::get<1>(msr) + 5 };

                d2d->CyanBrush->SetOpacity(0.2f);
                r->FillRoundedRectangle(D2D1::RoundedRect(r1, 5, 5), d2d->CyanBrush);
                d2d->CyanBrush->SetOpacity(1.0f);
                TEXTALIGNPUSH textalign(d2d->Text2, ScaleX <= 1 ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                r1.left += 5;
                r->DrawText(wt, (UINT32)wcslen(wt), d2d->Text2, r1, d2d->BlackBrush);
				AddRectAndTip(r1, L"GPU Information");
            }
        }

        // Global Zoom

        // Draw Graph
        auto& xl = prj.xl();
        for (size_t i = 0; i < xl.ops.size(); i++)
		{
			auto& op = xl.ops[i];
            if (op.Visible == 0)
                continue;

            // Zoom
/*            D2D1_MATRIX_3X2_F m1 = {};
			r->GetTransform(&m1);
			if (op.Zoom != 1.0f)
			{
				D2D1_MATRIX_3X2_F m2 = D2D1::Matrix3x2F::Scale(op.Zoom, op.Zoom);
				r->SetTransform(m2);
			}
*/
            for (size_t j = 0; j < op.nodes.size(); j++)
			{
                auto& node = op.nodes[j];
                MLOP* mlop = 0;
                if (xl.ml && xl.ml->ops.size() > i)
					mlop = &xl.ml->ops[i];
                bool Active = op.Active;
				node->Draw(mlop,ActiveOperator2 == i,Active, r,i,j);   
			}
//            r->SetTransform(&m1);


            // Output to Input lines
            for (size_t j = 0; j < op.nodes.size(); j++)
            {
                auto& node = op.nodes[j];
				for (size_t k = 0; k < node->children.size(); k++)
				{
					auto& ch = node->children[k];
                    if (ch.O)
                    {
                        for (auto& gg : ch.g)
                        {
                            XLNODEBULLET* node2 = 0;
                            for (auto& nn2 : op.nodes)
                            {
                                for (auto& ch2 : nn2->children)
                                {
                                    if (ch2.O == 0)
                                    {
                                        for (auto& gg2 : ch2.g)
                                        {
                                            if (gg2.key == gg.key)
                                            {
                                                node2 = &ch2;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            if (node2)
                            {
                                D2D1_POINT_2F p1 = { ch.hit.MiddleX(),ch.hit.MiddleY() };
                                D2D1_POINT_2F p2 = { node2->hit.MiddleX(),node2->hit.MiddleY() };
                                r->DrawLine(p1, p2, d2d->BlackBrush);

                                D2D1_POINT_2F middle = {};
                                middle.x = (p1.x + p2.x) / 2;
                                middle.y = (p1.y + p2.y) / 2;

                                wchar_t cond[1000] = {};
#ifdef _DEBUG
                                if (gg.conditions.size() == 0)
                                {
                                    swprintf_s(cond, 1000, L"%zi", gg.key);
                                    auto msr1 = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text, cond);
                                    D2D1_RECT_F r1 = { middle.x - std::get<0>(msr1) / 2.0f - 2,middle.y - std::get<1>(msr1) / 2.0f - 2,middle.x + std::get<0>(msr1) / 2.0f + 2,middle.y + std::get<1>(msr1) / 2.0f + 2 };
                                    d2d->CyanBrush->SetOpacity(0.5f);
                                    r->FillRoundedRectangle(D2D1::RoundedRect(r1, 5, 5), d2d->CyanBrush);
                                    d2d->CyanBrush->SetOpacity(1.0f);
                                    TEXTALIGNPUSH textalign(d2d->Text2, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                                    r->DrawText(cond, (UINT32)wcslen(cond), d2d->Text2, r1, d2d->WhiteBrush);
                                }
                                else
                                {
                                    swprintf_s(cond, 1000, L"%zi", gg.key);
                                }
#endif


                                // Conditions
                                if (gg.conditions.size())
                                {
                                    if (wcslen(cond))
										wcscat_s(cond, 100, L"\r\n"); 
                                    for (size_t ii = 0; ii < gg.conditions.size(); ii++)
                                    {
										swprintf_s(cond + wcslen(cond), 100, L"%S", gg.conditions[ii].expression.c_str());
										if (ii != gg.conditions.size() - 1)
											wcscat_s(cond, 100,L"\n");
                                    }

                                    auto msr1 = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text, cond);
                                    D2D1_RECT_F r1 = { middle.x - std::get<0>(msr1)/2.0f - 2,middle.y - std::get<1>(msr1)/2.0f - 2,middle.x + std::get<0>(msr1)/2.0f + 2,middle.y + std::get<1>(msr1)/2.0f + 2};
                                    d2d->CyanBrush->SetOpacity(0.8f);
                                    r->FillRoundedRectangle(D2D1::RoundedRect(r1, 5, 5), d2d->CyanBrush);
									d2d->CyanBrush->SetOpacity(1.0f);
									TEXTALIGNPUSH textalign(d2d->Text2, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
									r->DrawText(cond, (UINT32)wcslen(cond), d2d->Text2, r1, d2d->WhiteBrush);
                                }
                            }
                        }
					}
				}
            }
		}

        // Shared Memory
        for (auto& op : xl.ops)
        {
            if (op.Visible == 0)
                continue;
            for (auto& n1 : op.nodes)
            {
                if (n1->ShareMemory == 0)
                    continue;
                for (auto& op2 : xl.ops)
                {
                    if (op2.Visible == 0)
                        continue;
                    for (auto& n2 : op2.nodes)
                    {
                        if (n2->ShareMemory == 0)
                            continue;
                        if (n1.get() == n2.get())
							continue;
						if (n1->ShareMemory == -n2->ShareMemory && n2->IsInput() && n1->ShareMemory > 0)
						{
							D2D1_POINT_2F p1 = { n1->bhit.MiddleX(),n1->bhit.MiddleY() };
							D2D1_POINT_2F p2 = { n2->bhit2.MiddleX(),n2->bhit2.MiddleY() };
							r->DrawLine(p1, p2, d2d->BlackBrush);
						}
                    }
                }
            }
        }


        // red connection
		if (red_from.x  > 0 && red_to.x > 0)
		{
			r->DrawLine(red_from, red_to, d2d->RedBrush);
		}

        // Running 
        if (xl.running)
        {
            TEXTALIGNPUSH textalign(d2d->Text2, ScaleX <= 1 ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            D2F rf = rfull;
			D2D1_RECT_F r1 = { rfull.left,rf.top + 30,rfull.right,rf.top + 70};
            if (ScaleX <= 1)
            {
                r1.top = rf.MiddleY() - 30;
                r1.bottom = rf.MiddleY() + 30;
            }
			d2d->RedBrush->SetOpacity(0.2f);
			r->FillRectangle(r1, d2d->RedBrush);
			d2d->RedBrush->SetOpacity(1.0f);
			swprintf_s(wt, L" Running");
			r->DrawTextW(wt, (UINT32)wcslen(wt), d2d->Text2, r1, d2d->BlackBrush);   
        }

        [[maybe_unused]] auto hr = d2d->m_d2dContext->EndDraw();
        if (FAILED(hr))
        {
            MessageBeep(0);
        }
        if (d2d->m_swapChain1)
            hr = d2d->m_swapChain1->Present(1, 0);
    }

    void MLGraph::OnAddVariable(IInspectable const&, IInspectable const&)
    {
        WhatInput = 4;
        _i1 = L"";
        _i0 = L"Enter variable name:";
        Refresh({ L"i1",L"i0" });
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.ShowAsync();

    }

    void MLGraph::Tip(const wchar_t* t)
    {
		auto tt = Content().as<Panel>().FindName(L"Tooltip").as<ToolTip>();
        if (t)
        {
            POINT pt;
            GetCursorPos(&pt);
			ScreenToClient((HWND)wnd(), &pt);
            pt.x += 50;
            pt.y += 50;
            winrt::Windows::Foundation::Rect re;
			re.X = (float)pt.x;
			re.Y = (float)pt.y;
			re.Width = 100;
			re.Height = 100;
            tt.PlacementRect(re);
//			tt.Placement(winrt::Microsoft::UI::Xaml::Controls::Primitives::PlacementMode::Mouse);
            tt.Content(winrt::box_value(t));
            tt.Visibility(Visibility::Visible);
        }
        else
        {
			tt.Content(nullptr);
            tt.Visibility(Visibility::Collapsed);
        }
    }


    void MLGraph::OnAddSet(IInspectable const&, IInspectable const&)
    {
        XL xln = ADefXL();
		prj.xls.push_back(xln);
		prj.iActive = prj.xls.size() - 1;
        FullRefresh();
    }

    void MLGraph::OnAddOp(IInspectable const&, IInspectable const&)
    {
        Push();
        auto& xl = prj.xl();
        xl.ops.push_back(XLOP());
        ActiveOperator2 = xl.ops.size() - 1;
        Tip(L"Operator added and activated");
        FullRefresh();
    }


    void MLGraph::Push()
    {
        XML3::XMLElement e;
        auto& xl = prj.xl();
        xl.Ser(e);
		undo_list.push(e);
    }

    void MLGraph::RefreshMenu()
    {
        auto sp = Content().as<Panel>();
        auto m1 = sp.FindName(L"ActiveOperatorSubmenu").as<MenuFlyoutSubItem>();
		m1.Items().Clear();

        // Add operators as radio
        auto& xl = prj.xl();
        for (size_t i = 0; i < xl.ops.size(); i++)
		{
			auto mi = RadioMenuFlyoutItem();
			mi.Text(ystring().Format(L"Operator %zi",i + 1));
			mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
				{
					ActiveOperator2 = i;
                    Refresh();
				});

            if (i <= 9)
            {
                winrt::Microsoft::UI::Xaml::Input::KeyboardAccelerator kba;
                kba.Key((winrt::Windows::System::VirtualKey)(i + 0x31));
                kba.Modifiers(winrt::Windows::System::VirtualKeyModifiers::Control);
                mi.KeyboardAccelerators().Append(kba);
            }

			if (ActiveOperator2 == i)
				mi.IsChecked(true);
			m1.Items().Append(mi);
		}

        // Visible
        auto m3 = sp.FindName(L"VisibleOperatorSubmenu").as<MenuFlyoutSubItem>();
        m3.Items().Clear();
        for (size_t i = 0; i < xl.ops.size(); i++)
        {
            auto mi = winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem();
            mi.Text(ystring().Format(L"Operator %zi", i + 1));
            mi.IsChecked(xl.ops[i].Visible);
            mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                {
                    auto& xl = prj.xl();
                    xl.ops[i].Visible = !xl.ops[i].Visible;
                    Refresh();
                });
            if (i <= 9)
            {
                winrt::Microsoft::UI::Xaml::Input::KeyboardAccelerator kba;
                kba.Key((winrt::Windows::System::VirtualKey)(i + 0x31));
                kba.Modifiers(winrt::Windows::System::VirtualKeyModifiers::Menu);
                mi.KeyboardAccelerators().Append(kba);
            }
            m3.Items().Append(mi);
        }

        // Visible
        if (1)
        {
            auto m5 = sp.FindName(L"EnableOperatorSubmenu").as<MenuFlyoutSubItem>();
            m5.Items().Clear();
            for (size_t i = 0; i < xl.ops.size(); i++)
            {
                auto mi = winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem();
                mi.Text(ystring().Format(L"Operator %zi", i + 1));
                mi.IsChecked(xl.ops[i].Active);
                mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                    {
                        auto& xl = prj.xl();
                        xl.ops[i].Active = !xl.ops[i].Active;
                        Refresh();
                    });
                if (i <= 9)
                {
                    winrt::Microsoft::UI::Xaml::Input::KeyboardAccelerator kba;
                    kba.Key((winrt::Windows::System::VirtualKey)(i + 0x31));
                    kba.Modifiers(winrt::Windows::System::VirtualKeyModifiers::Shift);
                    mi.KeyboardAccelerators().Append(kba);
                }
                m5.Items().Append(mi);
            }
        }

        // And to the remove menu
		auto m2 = sp.FindName(L"DeleteOperatorSubmenu").as<MenuFlyoutSubItem>();
		m2.Items().Clear();
        for (size_t i = 0; i < xl.ops.size(); i++)
        {
            auto mi = MenuFlyoutItem();
			mi.Text(ystring().Format(L"Operator %zi", i + 1));


			mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
				{
                    Push();
                    Dirty(1);
                    auto& xl = prj.xl();
                    xl.ops.erase(xl.ops.begin() + i);
					if (ActiveOperator2 >= xl.ops.size())
						ActiveOperator2 = xl.ops.size() - 1;
                    FullRefresh();
				});
			m2.Items().Append(mi);
        }

        // And the variables
		std::sort(xl.variables.begin(), xl.variables.end(), [](const VARIABLE& a, const VARIABLE& b) { return a.n < b.n; });
        auto m21 = sp.FindName(L"EditVariableSubmenu").as<MenuFlyoutSubItem>();
        m21.Items().Clear();
        for (size_t i = 0; i < xl.variables.size(); i++)
        {
            auto mi = MenuFlyoutItem();
            mi.Text(ystring().Format(L"%s: %s", xl.variables[i].n.c_str(), xl.variables[i].orv.c_str()));
            mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                {
                    WhatInput = 5;
                    auto& xl = prj.xl();
                    _i0 = xl.variables[i].n;
                    _i1 = xl.variables[i].orv;
                    WhatVariable = &xl.variables[i];
                    Refresh({ L"i1",L"i0" });
                    auto sp = Content().as<Panel>();
                    auto ct = sp.FindName(L"Input1").as<ContentDialog>();
                    ct.ShowAsync();
                });
            m21.Items().Append(mi);
        }
        auto m22 = sp.FindName(L"DeleteVariableSubmenu").as<MenuFlyoutSubItem>();
        m22.Items().Clear();
        for (size_t i = 0; i < xl.variables.size(); i++)
        {
            auto mi = MenuFlyoutItem();
            mi.Text(ystring().Format(L"%s", xl.variables[i].n.c_str()));
            mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                {
                      Push();
                      Dirty(1);
                      auto& xl = prj.xl();
                      xl.variables.erase(xl.variables.begin() + i);
                      FullRefresh();
                });
            m22.Items().Append(mi);
        }


        // And the sets
        auto m41 = sp.FindName(L"EditSetSubmenu").as<MenuFlyoutSubItem>();
        m41.Items().Clear();
        for (size_t i = 0; i < prj.xls.size(); i++)
        {
            auto mi = RadioMenuFlyoutItem();
            mi.Text(ystring().Format(L"Set %zi",i + 1));
			mi.IsChecked(prj.iActive == i);
            if (i <= 9)
            {
                winrt::Microsoft::UI::Xaml::Input::KeyboardAccelerator kba;
                kba.Key((winrt::Windows::System::VirtualKey)(i + 0x31));
                kba.Modifiers((winrt::Windows::System::VirtualKeyModifiers)3);
                mi.KeyboardAccelerators().Append(kba);
            }

            mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                {
					prj.iActive = i;
					FullRefresh();
                });
            m41.Items().Append(mi);
        }
        auto m42 = sp.FindName(L"DeleteSetSubmenu").as<MenuFlyoutSubItem>();
        m42.Items().Clear();
        for (size_t i = 0; i < prj.xls.size(); i++)
        {
            auto mi = MenuFlyoutItem();
			mi.IsEnabled(prj.xls.size() > 1);
            mi.Text(ystring().Format(L"Set %zi", i + 1));
            mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                {
                    if (prj.xls.size() > 1)
                    {
                        prj.xls.erase(prj.xls.begin() + i);
                        if (prj.iActive >= prj.xls.size())
                            prj.iActive = prj.xls.size() - 1;

                        FullRefresh();
                    }
                });
            m42.Items().Append(mi);
        }

        // And the adapters
        auto m31 = sp.FindName(L"AdapterMenu").as<MenuBarItem>();
        if (m31.Items().Size() == 0)
        {
            auto iAdapter = SettingsX->GetRootElement().vv("iAdapter").GetValueInt(0);

            for (size_t i = 0; i <= all_adapters.size(); i++)
            {
                if (i == 0)
                {
                    auto mi = RadioMenuFlyoutItem();
                    mi.Text(L"Default Adapter");
                    mi.IsChecked(iAdapter == 0);
                    mi.Click([this](IInspectable const&, RoutedEventArgs const&)
                        {
							SettingsX->GetRootElement().vv("iAdapter").SetValueInt(0);
                            SettingsX->Save();
							Clean();
                            FullRefresh();
                            UpdateVideoMemory();
                        });
                    m31.Items().Append(mi);

                    auto mi2 = MenuFlyoutSeparator();
                    m31.Items().Append(mi2);
                    continue;
                }
                auto mi = RadioMenuFlyoutItem();
                DXGI_ADAPTER_DESC1 desc;
                all_adapters[i - 1]->GetDesc1(&desc);
                mi.Text(ystring().Format(L"Adapter %zi: %s", i, desc.Description));
                mi.IsChecked(iAdapter == i);
                mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
                    {
                        SettingsX->GetRootElement().vv("iAdapter").SetValueInt((int)i);
                        SettingsX->Save();
                        Clean();
                        FullRefresh();
                          UpdateVideoMemory();
                    });
                m31.Items().Append(mi);
            }
        }

    }
    void MLGraph::Input2_Completed(IInspectable, IInspectable)
    {
		RadioButtons rb = Content().as<Panel>().FindName(L"Input4a").as<RadioButtons>();
        if (WhatInput == 11 && WhatNode && WhatVariable)
        {
			try
			{
				auto it = dynamic_cast<XLNODE_ANY*>(WhatNode);
                if (it)
                {
                    VARIABLECHANGE vc;
					vc.n = WhatVariable->n;
                    vc.V = std::stof(_i1.c_str());
					vc.Type = rb.SelectedIndex();   

                    Push();
                    // Remove existing variable
                    for (size_t i = 0; i < it->VariableChanges.size(); i++)
                    {
                        if (it->VariableChanges[i].n == WhatVariable->n)
                        {
                            it->VariableChanges.erase(it->VariableChanges.begin() + i);
                            break;
                        }
                    }
                    it->VariableChanges.push_back(vc);
                    Dirty(1);
                }
			}
			catch (...)
			{
			}
			FullRefresh();
        }
    }

    void MLGraph::Input_Completed(IInspectable, IInspectable)
    {
        auto& xl = prj.xl();

        if (WhatInput == 21 && WhatBullet && WhatConnection)
        {
            CONDITION nc;
            nc.expression = XML3::XMLU(_i1.c_str());
            WhatConnection->conditions.push_back(nc);
            Push();
            Dirty(1);
            FullRefresh();
            return;
        }

        if (WhatInput == 22 && WhatBullet && WhatConnection && WhatCondition)
        {
			WhatCondition->expression = XML3::XMLU(_i1.c_str());    
            Push();
            Dirty(1);
            FullRefresh();
            return;
        }

        if (WhatInput == 6 && WhatNode)
        {
            //Tensor Shape
          if (auto* node = dynamic_cast<XLNODE_ANY*>(WhatNode))
            {

                auto dims = StringTensorFromString(XML3::XMLU(_i1.c_str()));
                node->tensor_dims2 = dims;
            }
			FullRefresh();
            return;
        }
        if (WhatInput == 5 && WhatVariable)
        {
            // Edit Variable
            try
            {
                Push();
                Dirty(1);
                WhatVariable->orv = _i1;
            }
            catch (...)
            {

            }
            FullRefresh();

        }
        else
        if (WhatInput == 4)
        {
            // Add variable
            if (wcslen(_i1.c_str()))
            {
                bool F = 0;
                for (auto& ff : xl.variables)
				{
					if (ff.n == _i1)
					{
						F = 1;
						break;
					}
				}
                if (F == 0)
                {
                    VARIABLE v;
                    v.n = _i1;
                    xl.variables.push_back(v);
                    FullRefresh();
                }
            }

        }
        else
        if (WhatInput == 2 && WhatParam)
        {
            try
            {
                Push();
                Dirty(1);
                bool Number = 0;
                try
                {
                    [[maybe_unused]] auto j = std::stoi(_i1);
                    Number = 1;
                }
                catch (...)
                {

                }

                if (Number == 0 || (WhatParam->minv <= -1 && WhatParam->maxv <= -1))
                    WhatParam->v = _i1;
				else
                    WhatParam->v =  std::to_wstring(std::clamp(std::stof(_i1),WhatParam->minv,WhatParam->maxv));
            }
            catch (...)
            {

            }
			FullRefresh();
            return;
        }
        if (WhatInput == 1 || WhatInput == 3)
        {
            // Input/Cosntant Tensor Dimensions
            auto& op = xl.ops[ActiveOperator2];
            std::vector<unsigned int> dims = TensorFromString(_i1.c_str()); // convert string to dims, split in x
			if (dims.empty())
				return; 
            try
            {
                Push();
                Dirty(1);
                POINT pos = {};
                GetCursorPos(&pos);
                ScreenToClient((HWND)wnd(), &pos);
                if (pos.x > 50)
                    pos.x -= 50;
                if (pos.y > 50)
                    pos.y -= 50;

                if (WhatInput == 3)
                {
                    auto t = std::make_shared<XLNODE_ANY>(0,TYPE_CONSTANT,1);
					t->Params.push_back({ L"Value",L"0"});
                    t->hit = D2D1_RECT_F({ 10,10,100,100 });
                    t->hit.left = (float)pos.x;
                    t->hit.top = (float)pos.y;
                    t->SetTensorDims(dims);
                    op.nodes.push_back(t);
                }
                if (WhatInput == 1)
                {
                    auto t = std::make_shared<XLNODE_ANY>(0,TYPE_INPUT,1);
                    t->hit = D2D1_RECT_F({ 10,10,100,100 });
                    t->hit.left = (float)pos.x;
                    t->hit.top = (float)pos.y;
                    t->SetTensorDims(dims);
                    op.nodes.push_back(t);
                }
            }
            catch (...)
            {

            }
            FullRefresh();
        }
    }

    void MLGraph::OnAddConstant(IInspectable const&, IInspectable const&)
    {
        WhatInput = 3;
        _i1 = L"10x10";
        _i0 = L"Enter constant tensor dimensions:";
        Refresh({ L"i1",L"i0" });
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.ShowAsync();
    }

    void MLGraph::OnAddInput(IInspectable const&, IInspectable const&)
    {
        WhatInput = 1;
        _i1 = L"10x10";
        _i0 = L"Enter input tensor dimensions:";
        Refresh({ L"i1",L"i0" });
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.ShowAsync();

    }
    void MLGraph::OnAddOutput(IInspectable const&, IInspectable const&)
    {
        Push();
        Dirty(1);
        auto& xl = prj.xl();
        auto& op = xl.ops[ActiveOperator2];
        auto node = std::make_shared<XLNODE_ANY>(1,TYPE_OUTPUT,0);
        POINT pos = {};
        GetCursorPos(&pos);
		ScreenToClient((HWND)wnd(), &pos);  
        if (pos.x > 50)
            pos.x -= 50;
        if (pos.y > 50)
            pos.y -= 50;
        node->hit.left = (float)pos.x;
        node->hit.top = (float)pos.y;

        op.nodes.push_back(node);
        FullRefresh();
    }
    void MLGraph::OnStop(IInspectable const&, IInspectable const&)
    {
        Stop();
    }
    void MLGraph::Stop()
    {
        MustStop = 1;
    }


    void MLGraph::OnRun(IInspectable const&, IInspectable const&)
    {
        auto& xl = prj.xl();
        if (xl.running)
            return;

        if (!xl.ml)
            xl.ml = std::make_shared<ML>();
        if (xl.ml->d3D12Device == 0)
        {
            auto hr = Compile();
            if (FAILED(hr))
                return;
        }

        MustStop = 0;
        xl.running = std::make_shared<std::thread>([this](HWND h)
            {
                Run();
//                Sleep(5000);
                PostMessage(h, WM_USER + 10, 0, 0);
            },(HWND)wnd());
        FullRefresh();
    }

    void MLGraph::Finished()
    {
        auto& xl = prj.xl();
        if (xl.running)
        {
            xl.running->join();
            xl.running = 0;
        }
        FullRefresh();
    }

    void MLGraph::Run()
    {
		CoInitializeEx(0, COINIT_MULTITHREADED);
        auto& xl = prj.xl();
        if (!xl.ml)
			xl.ml = std::make_shared<ML>();
        auto& ml = *xl.ml;
        try
        {
            void Locate(const wchar_t* fi);

            // Initialize
            if (ml.d3D12Device == 0)
                Compile();
            if (ml.d3D12Device == 0)
                return;
            if (ml.ops.size() != xl.ops.size())
                return;

            // Load csv files in inputs
            if (ml.descriptorHeap == 0)
                ml.Prepare();

            bool WillBeLast = 0;


            // File mapping
            for (size_t iop = 0; iop < xl.ops.size(); iop++)
            {
                auto& op = xl.ops[iop];
                if (op.Active == 0)
                    continue;

                for (auto& node : op.nodes)
                {
                    if (auto it = std::dynamic_pointer_cast<XLNODE_ANY>(node))
                    {
                        if (it->what == TYPE_INPUT)
                        {
                            if (it->ShareMemory < 0)
                                continue;
                            if (it->csv_input.length())
                            {
                                if (it->csv_input == L"\"Random\"")
                                {
                                    continue;
                                }
                                if (it->csv_input == L"\"Sequence\"")
                                {
                                    continue;
                                }
                                auto inf = it->csv_input;
                                if (GetFileAttributes(inf.c_str()) == 0xFFFFFFFF)
                                {
                                    std::vector<wchar_t> mf(1000);
                                    wcscpy_s(mf.data(), 1000, current_file.c_str());
                                    std::wstring mfs = mf.data();
                                    auto p = mfs.find_last_of(L"\\");
                                    mfs = mfs.substr(0, p);
                                    mfs += L"\\";
                                    mfs += it->csv_input;
                                    inf = mfs;
                                }

                                // Actually csv?
                                auto ch = wcsrchr(inf.c_str(), L'.');
                                if (ch && _wcsicmp(ch, L".csv") == 0)
                                {
                                    std::wstring TempFile3();
                                    auto tf = TempFile3();
                                    void CsvToBinary(const wchar_t* csv, const wchar_t* binout);
                                    CsvToBinary(inf.c_str(), tf.c_str());
                                    it->mapin.Map(tf.c_str());
                                }
                                else
                                {
                                    it->mapin.Map(inf.c_str());
                                }
                            }
                        }
                    }
                    if (node->csv_output.length())
                    {
                        auto of = node->csv_output;
                        auto pathhas = wcsrchr(of.c_str(), L'\\');
                        if (!pathhas)
                        {
                            std::vector<wchar_t> pa(1000);
                            wcscpy_s(pa.data(), 1000, current_file.c_str());
                            std::wstring mfs = pa.data();
                            auto p = mfs.find_last_of(L"\\");
                            mfs = mfs.substr(0, p);
                            mfs += L"\\";
                            mfs += of;
                            of = mfs;
                        }
                        DeleteFile(of.c_str());
//						node->mapout.CreateForRecord(of.c_str()); 
                    }
                }
            }

            for (auto& v : xl.variables)
                v.runv = v.orv;
            PUSHPOPVAR ppv(&xl);

            for (int iRun = 0; ; iRun++)
            {
                if (WillBeLast == 1)
                    break;
                if (MustStop)
                {
                    MustStop = 0;
                    break;
                }
				WillBeLast = 1;
                for (size_t iop = 0; iop < xl.ops.size(); iop++)
                {
                    auto& op = xl.ops[iop];
                    if (op.Active == 0)
                        continue;
                    auto& mlop = ml.ops[iop];
                    for (auto& node : op.nodes)
                    {
                        if (auto it = std::dynamic_pointer_cast<XLNODE_ANY>(node))
                        {
                            if (it->what == TYPE_INPUT)
                            {
                                for (auto& a_tid : it->tidxs)
                                {
                                    auto& wh = mlop.Item(a_tid);
                                    if (!wh.buffer)
                                        continue;

                                    if (it->ShareMemory >= 0 && it->csv_input == L"\"Random\"")
                                    {
                                        long long bs = (long long)wh.buffer->b.sz();
                                        std::vector<float> fv(bs / 4);
                                        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                                        for (size_t i = 0; i < fv.size(); i++)
                                        {
                                            extern std::mt19937 grand_mt;
                                            fv[i] = dist(grand_mt);
                                        }
                                        wh.buffer->Upload(&ml, fv.data(), bs);
                                        continue;
                                    }

                                    if (it->ShareMemory >= 0 && it->csv_input == L"\"Sequence\"")
                                    {
                                        long long bs = (long long)wh.buffer->b.sz();
                                        std::vector<float> fv(bs / 4);
                                        float nf = 0.0f;
                                        for (size_t i = 0; i < fv.size(); i++)
                                        {
                                            fv[i] = nf;
                                            nf += 0.01f;
                                        }
                                        wh.buffer->Upload(&ml, fv.data(), bs);
                                        continue;
                                    }

                                    if (it->ShareMemory < 0 || it->mapin.p() == 0)
                                        continue;

                                    long long bs = (long long)wh.buffer->b.sz();
                                    long long szfu = (long long)(it->mapin.size()) - (long long)(iRun * bs);
                                    if (szfu <= 0)
                                        continue;
                                    if (szfu <= bs)
                                        wh.buffer->Upload(&ml, it->mapin.p() + (iRun * bs), szfu);
                                    else
                                    {
                                        wh.buffer->Upload(&ml, it->mapin.p() + (iRun * bs), bs);
                                        WillBeLast = 0;
                                    }
                                }
                            }
                        }
                    }


                    ml.Run(iop);

                    for (auto& node : op.nodes)
                    {
                        if (WillBeLast == 0)
                            break;
                        if (auto it = std::dynamic_pointer_cast<XLNODE>(node))
                        {
                            if (it->csv_output.length())
                            {
                                auto of = it->csv_output;
                                auto pathhas = wcsrchr(of.c_str(), L'\\');
                                if (!pathhas)
                                {
                                    std::vector<wchar_t> pa(1000);
                                    wcscpy_s(pa.data(), 1000, current_file.c_str());
                                    std::wstring mfs = pa.data();
                                    auto p = mfs.find_last_of(L"\\");
                                    mfs = mfs.substr(0, p);
                                    mfs += L"\\";
                                    mfs += of;
                                    of = mfs;
                                }
                                DeleteFile(of.c_str());
                                for (auto& a_tidx : node->tidxs)
                                {
                                    auto& wh = mlop.Item(a_tidx);
                                    if (!wh.buffer)
                                        continue;
                                    std::vector<char> v;
                                    wh.buffer->Download(&ml, (size_t)-1, v);

                                    auto ch = wcsrchr(of.c_str(), L'.');
                                    if (it->csv_output == L"\"Screen\"")
                                    {
                                        std::vector<char> fv(v.size());
                                        memcpy(fv.data(), v.data(), v.size());
                                        std::wstring j;
                                        auto ts = fv.size();

                                        auto buf = wh.expr.GetOutputDesc().dataType;
                                        wh.operator DML_BINDING_DESC().Desc;


                                        if (buf == DML_TENSOR_DATA_TYPE_FLOAT32)
                                        {
                                            auto fv2 = (float*)v.data();
                                            for (size_t i = 0; i < ts / 4; i++)
                                            {
                                                j += std::to_wstring(fv2[i]);
                                                j += L" ";
                                            }
                                        }
                                        if (buf == DML_TENSOR_DATA_TYPE_FLOAT64)
                                        {
                                            auto fv2 = (double*)v.data();
                                            for (size_t i = 0; i < ts / 8; i++)
                                            {
                                                j += std::to_wstring(fv2[i]);
                                                j += L" ";
                                                if (j.length() > 1000)
                                                    break;
                                            }
                                        }
                                        if (buf == DML_TENSOR_DATA_TYPE_INT32)
                                        {
                                            auto fv2 = (int*)v.data();
                                            for (size_t i = 0; i < ts / 4; i++)
                                            {
                                                j += std::to_wstring(fv2[i]);
                                                j += L" ";
                                                if (j.length() > 1000)
                                                    break;
                                            }
                                        }
                                        if (buf == DML_TENSOR_DATA_TYPE_INT64)
                                        {
                                            auto fv2 = (long long*)v.data();
                                            for (size_t i = 0; i < ts / 8; i++)
                                            {
                                                j += std::to_wstring(fv2[i]);
                                                j += L" ";
                                                if (j.length() > 1000)
                                                    break;
                                            }
                                        }
                                        if (buf == DML_TENSOR_DATA_TYPE_UINT32)
                                        {
                                            auto fv2 = (unsigned int*)v.data();
                                            for (size_t i = 0; i < ts / 4; i++)
                                            {
                                                j += std::to_wstring(fv2[i]);
                                                j += L" ";
                                                if (j.length() > 1000)
                                                    break;
                                            }
                                        }
                                        if (buf == DML_TENSOR_DATA_TYPE_UINT64)
                                        {
                                            auto fv2 = (unsigned long long*)v.data();
                                            for (size_t i = 0; i < ts / 8; i++)
                                            {
                                                j += std::to_wstring(fv2[i]);
                                                j += L" ";
                                            }
                                        }

                                        MessageBox(0, j.c_str(), L"Screen", MB_ICONINFORMATION);
                                        continue;
                                    }
                                    else
                                        if (ch && wcscmp(ch, L".csv") == 0)
                                        {
                                            std::vector<float> fv(v.size() / 4);
                                            memcpy(fv.data(), v.data(), v.size());
                                            std::ofstream f(of);
                                            if (f.is_open())
                                            {
                                                for (size_t i = 0; i < fv.size(); i++)
                                                {
                                                    f << fv[i] << std::endl;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // Binary
                                            PutFile(of.c_str(), v, true);
                                        }
                                    Locate(of.c_str());
                                }
                            }
                        }
                    }
                }
            }
        }
		catch (...)
		{
			MessageBox(0, L"Run failed!", L"Error", MB_ICONERROR);
		}
    }

    void MLGraph::Dirty(bool CL)
    {
        if (CL)
        {
            auto& xl = prj.xl();
            if (xl.ml)
                Clean();
        }
    }

    void MLGraph::Clean()
    {
        auto& xl = prj.xl();
        xl.ml = nullptr;
        for (auto& op : xl.ops)
        {
            for (auto& node : op.nodes)
            {
                if (auto it = std::dynamic_pointer_cast<XLNODE>(node))
                {
                    it->tidxs.clear();
                    auto it2 = std::dynamic_pointer_cast<XLNODE_ANY>(it);
                    if (it2)
                        it2->MultipleOpOutputData = {};
                }
            }
        }

        UpdateVideoMemory();
        FullRefresh();
    }

    void MLGraph::OnGC(IInspectable const&, IInspectable const&)
    {
		auto& xl = prj.xl();
        if (FAILED(Compile()))
            return;
        auto str = xl.GenerateCode();
#ifdef _DEBUGf
		MessageBoxA(0, str.c_str(), "Code", MB_ICONINFORMATION);
#else
        std::wstring TempFile3();
        auto tf = TempFile3();
        DeleteFile(tf.c_str());
        tf += L".txt";

        // put str to tf
		std::ofstream f(tf);
		if (f.is_open())
		{
			f << str;
		}
		ShellExecute(0, L"open", tf.c_str(),  0, 0, SW_SHOWNORMAL);
#endif
    }

    void MLGraph::OnGCV(IInspectable const&, IInspectable const&)
    {
        auto& xl = prj.xl();
        if (FAILED(Compile()))
            return;
        auto str = xl.GenerateCode();
        std::wstring TempFile3();
        auto tf = TempFile3();
        SHCreateDirectory(0, tf.c_str());
        void ExtractVisualStudioSolution(const wchar_t* where, std::string code);
        ExtractVisualStudioSolution(tf.c_str(), str);
        ShellExecute(0, L"open", tf.c_str(), 0, 0, SW_SHOWNORMAL);

    }

    void MLGraph::OnClean(IInspectable const&, IInspectable const&)
    {
        Clean();
    }

    void MLGraph::OnCompile(IInspectable const&, IInspectable const&)
    {
        Compile();
    }


    HRESULT MLGraph::Compile()
    {
        HRESULT hres = E_FAIL;
		Clean();
        auto& xl = prj.xl();
        if (!xl.ml)
            xl.ml = std::make_shared<ML>();
        auto& ml = *xl.ml;

        for (auto& v : xl.variables)
            v.runv = v.orv;
        PUSHPOPVAR ppv(&xl);
        try
        {
#ifdef _DEBUG
            ml.SetDebug(1);
#endif
            ml.SetFeatureLevel(DML_FEATURE_LEVEL_6_4);
            

            CComPtr<IDXGIAdapter> ad;
            auto iAdapter = SettingsX->GetRootElement().vv("iAdapter").GetValueInt(0);
            if (iAdapter > 0 && iAdapter <= all_adapters.size())
                ad = all_adapters[iAdapter - 1];
            ml.On(ad);
            for (size_t i = 0; i < xl.ops.size(); i++)
            {
                int tidx = 0;
                auto& op = xl.ops[i];
                MLOP mop(&ml);

                // test 2 nodes and input output
                if (op.nodes.size() == 2)
                {
					auto first = std::dynamic_pointer_cast<XLNODE_ANY>(op.nodes[0]);
					auto second = std::dynamic_pointer_cast<XLNODE_ANY>(op.nodes[1]);
                    if (first && second && first->what == TYPE_INPUT && second->what == TYPE_OUTPUT)
                    {
                        if (first->children.size() == 1 && second->children.size() == 1)
                        {
                            auto node = std::make_shared<XLNODE_ANY>(1, TYPE_IDENTITY);
                            node->hit.left = 10;
                            node->hit.top = 10;
                            if (node->children.size() == 2)
                            {
                                auto ne1 = nextn();
                                auto ne2 = nextn();
                                first->children[0].g.clear();
                                CONNECTION conn1,conn2;
                                conn1.key = ne1;
                                conn2.key = ne2;
                                first->children[0].g.push_back(conn1);

                                node->children[0].g.clear();
                                node->children[0].g.push_back(conn1);
                                node->children[1].g.clear();
                                node->children[1].g.push_back(conn2);

                                second->children[0].g.clear();
                                second->children[0].g.push_back(conn2);

                                op.nodes.push_back(node);
                            }
                        }
                    }
                }

				std::sort(op.nodes.begin(), op.nodes.end(), [](std::shared_ptr<XLNODE> a, std::shared_ptr<XLNODE> b) 
                    { 
                        auto aa = dynamic_cast<XLNODE_ANY*>(a.get());
                        auto bb = dynamic_cast<XLNODE_ANY*>(b.get());
						if (a->IsInput() && !b->IsInput())
							return 1;   
                        if (!a->IsOutput() && b->IsOutput())
                            return 1;
                        if (a->IsInput() && b->IsInput() && aa->what == TYPE_INPUT && bb->what != TYPE_INPUT)
                            return 1;

                        // Check if a node is behind
/*                        unsigned long long minKeyA = a->min_key();
                        unsigned long long minKeyB = b->min_key();
						if (minKeyA < minKeyB)
							return 1;
                            */

                        return 0;
                    });    


                std::set<size_t> used_nodes;
                for (int iPass = 0 ; iPass < 10 ; iPass++)
                {
					if (op.nodes.size() == used_nodes.size())
						break;
                    for (size_t ii = 0; ii < op.nodes.size(); ii++)
                    {
                        if (std::find(used_nodes.begin(), used_nodes.end(), ii) != used_nodes.end())
                            continue;
                        class jUMP
                        {
                            XL* xl = 0;
                            XLOP* xlop = 0;
                            size_t* pii = 0;
                            PUSHPOPVAR* ppv = 0;
                        public:
                            jUMP(XL* xlx,XLOP* op, size_t* piii,PUSHPOPVAR* ppvv)
                            {
                                xl = xlx;
                                xlop = op;
                                pii = piii;
                                ppv = ppvv;
                            }
                            ~jUMP()
                            {
                                if (!xlop)
                                    return;
                                auto& node = xlop->nodes[*pii];
							    auto node2 = std::dynamic_pointer_cast<XLNODE_ANY>(node);
                                if (!node2)
                                    return;

                                // Apply Variable Changes
                                if (node2->VariableChanges.size())
                                {
                                    ppv->Off();
                                    for (auto& vc : node2->VariableChanges)
                                    {
                                        // Find variable
                                        for (auto& varx : xl->variables)
                                        {
                                            if (varx.n == vc.n)
                                            {
                                                // Change it
                                                if (vc.Type == 0)
                                                    varx.runv = std::to_wstring(vc.V);
                                                if (vc.Type == 1)
                                                    varx.runv = std::to_wstring(std::stof(varx.runv) + vc.V);
                                                break;
                                            }
                                        }
                                    }

                                    ppv->On();
                                }

                                // Jump
                                if (node2->Jump.iNodeJumpTo > 0)
                                {
                                    bool PassConditions = 1;
                                    for (auto& co : node2->Jump.conditions)
                                    {
                                        cparse::TokenMap vars;
                                        for (auto& v : xl->variables)
                                        {
                                            vars[XML3::XMLU(v.n.c_str()).bc()] = _wtof(v.runv.c_str());
                                            auto res = cparse::calculator::calculate(co.expression.c_str(), vars);
                                            if (!res.asBool())
                                            {
                                                PassConditions = 0;
                                                break;
                                            }
                                        }
                                    }
                                    if (PassConditions == 1)
                                    {
									    *pii = (node2->Jump.iNodeJumpTo - 1);
                                    }
                                }
                                
                            }

                        };

                        jUMP jump(&xl,&op,&ii,&ppv);


                        char the_code[1000] = {};
                        char the_code2[1000] = {};
                        char the_code3[1000] = {};
                        auto& node = op.nodes[ii];
                        [[maybe_unused]] auto str = node->name();


                        if (node->IsInput())
                        {
                            std::optional<MLRESOURCE> mlr;
                            char emlr[300] = {};
                            if (node->ShareMemory < 0)
                            {
                                // Find other 
                                int remote_tid = -1;
                                size_t iop = 0;
                                for (size_t ii3 = 0; ii3 < xl.ops.size(); ii3++)
                                {
                                    auto& op3 = xl.ops[ii3];
                                    for (auto& n : op3.nodes)
                                    {
                                        if (n == node)
                                            continue;
                                        if (n->ShareMemory == -node->ShareMemory && n->tidxs.size() > 0)
                                        {
                                            remote_tid = n->tidxs[0];
                                            iop = ii3;
                                            break;
                                        }
                                    }
                                }

                                if (remote_tid >= 0)
                                {
                                    auto& itx = ml.ops[iop].Item(remote_tid);
                                    if (itx.buffer)
                                    {
                                        mlr = itx.buffer->b;

                                        sprintf_s(emlr, R"(ml.ops[%zi].Item(%i).buffer->b)",iop,remote_tid);
                                    }
                                }
                            }

                            if (auto it2 = std::dynamic_pointer_cast<XLNODE_ANY>(node))
                            {
                                if (it2->what == TYPE_CONSTANT)
                                {
                                    DML_SCALAR_UNION scalar2 = {};

                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_FLOAT32)	scalar2.Float32 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_FLOAT16)	scalar2.Float32 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT32)	scalar2.UInt32 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT16)	scalar2.UInt16 = (unsigned short)(unsigned int)it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT8)	scalar2.UInt8 = (unsigned char)(unsigned int)it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT32)	scalar2.Int32 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT16)	scalar2.Int16 = (short)(int)it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT8)	scalar2.Int8 = (char)(int)it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_FLOAT64)	scalar2.Float64 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT64)	scalar2.UInt64 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT64)	scalar2.Int64 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT4)	scalar2.UInt32 = it2->Params[0];
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT4)	scalar2.Int32 = it2->Params[0];


                                    auto expr = dml::FillValueConstant(*mop.GetGraph(), it2->tensor_dims(), (DML_TENSOR_DATA_TYPE)it2->OpType, scalar2);
                                    mop.AddItem(expr, 0, false, BINDING_MODE::NONE);


                                    sprintf_s(the_code2, 1000, R"(DML_SCALAR_UNION scalar2 = {};)");
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_FLOAT32) sprintf_s(the_code3, R"(scalar2.Float32 = %.2f;)", (float)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_FLOAT16) sprintf_s(the_code3, R"(scalar2.Float16 = %.2f;)", (float)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT32) sprintf_s(the_code3, R"(scalar2.UInt32 = %u;)", (unsigned int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT16) sprintf_s(the_code3, R"(scalar2.UInt16 = %u;)", (unsigned short)(unsigned int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT8) sprintf_s(the_code3, R"(scalar2.UInt8 = %u;)", (unsigned char)(unsigned int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT32) sprintf_s(the_code3, R"(scalar2.Int32 = %i;)", (int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT16) sprintf_s(the_code3, R"(scalar2.Int16 = %i;)", (short)(int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT8) sprintf_s(the_code3, R"(scalar2.Int8 = %i;)", (char)(int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_FLOAT64) sprintf_s(the_code3, R"(scalar2.Float64 = %.2f;)", (double)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT64) sprintf_s(the_code3, R"(scalar2.UInt64 = %llu;)", (unsigned long long)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT64) sprintf_s(the_code3, R"(scalar2.Int64 = %lli;)", (long long)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_UINT4) sprintf_s(the_code3, R"(scalar2.UInt32 = %u;)", (unsigned int)it2->Params[0]);
                                    if (it2->OpType == DML_TENSOR_DATA_TYPE_INT4) sprintf_s(the_code3, R"(scalar2.Int32 = %i;)", (int)it2->Params[0]);




                                    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::FillValueConstant(*mop.GetGraph(), {%S},%S,scalar2), 0, false, BINDING_MODE::NONE);)", TensorToString(it2->tensor_dims()).c_str(), optypes2[it2->OpType].c_str());

                                    node->code = "\t";
                                    node->code += the_code2;
                                    node->code += "\r\n\t";
                                    node->code += the_code3;
                                    node->code += "\r\n\t";
                                    node->code += the_code;
                                }
                                else
                                if (it2->what == TYPE_DIAGONALMATRIX)
                                {
								    auto expr = dml::DiagonalMatrix(*mop.GetGraph(), it2->tensor_dims(), (DML_TENSOR_DATA_TYPE)it2->OpType, it2->Params[1],it2->Params[0]);
                                    mop.AddItem(expr, 0, false, BINDING_MODE::NONE);
                                }
                                else
                                if (it2->what == TYPE_INPUT)
                                {
                                    mop.AddInput({ (DML_TENSOR_DATA_TYPE)it2->OpType, it2->tensor_dims()}, 0, mlr ? false : true, BINDING_MODE::BIND_IN, mlr);
                                    sprintf_s(the_code, 1000, R"(mop.AddInput({ %S, {%S} }, 0, %s, BINDING_MODE::BIND_IN,%s);)", optypes2[it2->OpType].c_str(), TensorToString(it2->tensor_dims()).c_str(), mlr ? "false" : "true", mlr ? emlr : "{}");
                                    node->code = the_code;

                                }
                            }

						    used_nodes.insert(ii);
                            node->tidxs.push_back(tidx++);
                            continue;
                        }

                        std::vector<int> whati;
                        for (size_t ci = 0; ci < node->children.size(); ci++)
                        {
						    if (node->children[ci].O == 1)
							    continue;   
                            auto input_g = node->children[ci].g;
                            for (auto& input_gg : input_g)
                            {
                                for (auto& j : op.nodes)
                                {
                                    bool F = 0;
                                    if (j == node)
                                        continue;
                                    int count_outs = 0;
                                    for (auto& ch : j->children)
                                    {
                                        for (auto& chg : ch.g)
                                        {
                                            if (chg.key == input_gg.key && j->tidxs.size() > count_outs)
                                            {
                                                //Conditions
                                                bool PassConditions = 1;
                                                for (auto& c : chg.conditions)
                                                {
                                                    cparse::TokenMap vars;
                                                    for (auto& v : xl.variables)
                                                    {
													    vars[XML3::XMLU(v.n.c_str()).bc()] = _wtof(v.runv.c_str());
                                                        auto res = cparse::calculator::calculate(c.expression.c_str(), vars);
                                                        if (!res.asBool())
                                                        {
														    PassConditions = 0;
														    break;
                                                        }
                                                    }
                                                }
                                                if (PassConditions)
                                                {
                                                    F = 1;
                                                    whati.push_back(j->tidxs[count_outs]);
                                                    break;
                                                }

                                            }
                                        }
                                        if (F)
                                            break;
                                        if (ch.O == 1)
                                            count_outs++;
                                    }
                                    if (F)
                                        break;
                                }
                            }
                        }

                        if (auto it = std::dynamic_pointer_cast<XLNODE_ANY>(node))
                        {
                            if (it->what == TYPE_OUTPUT)
                            {
                                if (whati.size() == 1)
                                {
                                    if (whati[0] != -1)
                                    {

                                        mop.AddOutput(mop.Item(whati[0]));
                                        node->tidxs.push_back(tidx++);

                                        sprintf_s(the_code, 1000, R"(mop.AddOutput(mop.Item(%i));)", whati[0]);
                                        node->code = the_code;

                                        used_nodes.insert(ii);
                                        continue;
                                    }
                                }

                                continue;
                            }
                        }

                        dml::Expression expr;
                        bool Y = false;

                        if (auto it = std::dynamic_pointer_cast<XLNODE_ANY>(node))
                        {
                            if (whati.size() == 0)
                                continue;
                            if (whati.size() < it->ninreq())
                                continue;

                            used_nodes.insert(ii);

                            if (it->what == TYPE_ACT_CELU)
                            {
                                expr = (dml::ActivationCelu(mop.Item(whati[0]), (float)it->Params[0]));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationCelu(mop.Item(%i),%.2f));)", whati[0],(float)it->Params[0]);
                                node->code = the_code;

                            }
                            if (it->what == TYPE_ACT_ELU)
                            {
                                expr = (dml::ActivationElu(mop.Item(whati[0]), (float)it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationElu(mop.Item(%i),%.2f));)", whati[0], (float)it->Params[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_GELU)
                            {
                                expr = (dml::ActivationGelu(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationGelu(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_HARDMAX)
                            {
                                expr = (dml::ActivationHardmax(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationHardmax(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_HARDSIGMOID)
                            {
                                expr = (dml::ActivationHardSigmoid(mop.Item(whati[0]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationHardSigmoid(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_IDENTITY)
                            {
                                expr = (dml::ActivationIdentity(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationIdentity(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
						    if (it->what == TYPE_ACT_LEAKYRELU)
                            {
                                expr = (dml::ActivationLeakyRelu(mop.Item(whati[0]), it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationLeakyRelu(mop.Item(%i),%.2f));)", whati[0], (float)it->Params[0]);
							    node->code = the_code;
						    }
                            if (it->what == TYPE_ACT_LINEAR)
                            {
                                expr = (dml::ActivationLinear(mop.Item(whati[0]), it->Params[0], it->Params[1]));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationLinear(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_LOGSOFTMAX)
                            {
                                expr = dml::ActivationLogSoftmax(mop.Item(whati[0]));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationLogSoftmax(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_PRELU)
                            {
                                expr = (dml::ActivationParameterizedRelu(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationParameterizedRelu(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_PSOFTPLUS)
                            {
                                expr = (dml::ActivationParametricSoftplus(mop.Item(whati[0]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationParametricSoftplus(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_RELU)
                            {
                                expr = (dml::ActivationRelu(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationRelu(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_SELU)
                            {
                                expr = (dml::ActivationScaledElu(mop.Item(whati[0]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationScaledElu(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_STANH)
                            {
                                expr = (dml::ActivationScaledTanh(mop.Item(whati[0]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationScaledTanh(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_SHRINK)
                            {
                                expr = (dml::ActivationShrink(mop.Item(whati[0]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationShrink(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_SIGMOID)
                            {
                                expr = (dml::ActivationSigmoid(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationSigmoid(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_SOFTMAX)
                            {
                                expr = (dml::ActivationSoftmax(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationSoftmax(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_SOFTPLUS)
                            {
                                expr = (dml::ActivationSoftplus(mop.Item(whati[0]), it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationSoftplus(mop.Item(%i),%.2f));)", whati[0], (float)it->Params[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_SOFTSIGN)
                            {
                                expr = (dml::ActivationSoftsign(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationSoftsign(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_TANH)
                            {
                                expr = (dml::ActivationTanh(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationTanh(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACT_TRELU)
                            {
                                expr = (dml::ActivationThresholdedRelu(mop.Item(whati[0]), it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ActivationThresholdedRelu(mop.Item(%i),%.2f));)", whati[0], (float)it->Params[0]);
							    node->code = the_code;
                            }


                            if (it->what == TYPE_ABS && whati.size() > 0)
                            {
                                expr = dml::Abs(mop.Item(whati[0]));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Abs(mop.Item(%i)));)", whati[0]);
                                node->code = the_code;
                            }
                            if (it->what == TYPE_ACOS)
                            {
                                expr = (dml::ACos(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ACos(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ACOSH)
                            {
                                expr = (dml::ACosh(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ACosh(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ADD)
                            {
                                expr = (dml::Add(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Add(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
						    }
                            if (it->what == TYPE_ASIN)
                            {
                                expr = (dml::ASin(mop.Item(whati[0])));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ASin(mop.Item(%i)));)", whati[0]);
                                node->code = the_code;
                            }
                            if (it->what == TYPE_ASINH)
                            {
                                expr = (dml::ASinh(mop.Item(whati[0])));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ASinh(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ATAN)
                            {
                                expr = (dml::ATan(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ATan(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
						    }
                            if (it->what == TYPE_ATANH)
                            {
                                expr = (dml::ATanh(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ATanh(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;  
						    }
                            if (it->what == TYPE_ATANYX)
                            {
                                expr = (dml::ATanYX(mop.Item(whati[0]), mop.Item(whati[1])));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ATanYX(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
                                node->code = the_code;
                            }
                            if (it->what == TYPE_AVERAGEPOOLING)
                            {
                                expr = dml::AveragePooling(mop.Item(whati[0]), TensorFromString<unsigned int>(it->Params[0]), TensorFromString<unsigned int>(it->Params[1]), TensorFromString<unsigned int>(it->Params[2]), TensorFromString<unsigned int>(it->Params[3]), TensorFromString<unsigned int>(it->Params[4]), (bool)it->Params[5], TensorFromString<unsigned int>(it->Params[6]));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::AveragePooling(mop.Item(%i),{%S},{%S},{%S},{%S},{%S},%s,{%S}));)", whati[0], TensorStringToString(it->Params[0].v).c_str(), TensorStringToString(it->Params[1].v).c_str(), TensorStringToString(it->Params[2].v).c_str(), TensorStringToString(it->Params[3].v).c_str(), TensorStringToString(it->Params[4].v).c_str(), it->Params[5] ? "true" : "false", TensorStringToString(it->Params[6].v).c_str());
                                node->code = the_code;
                            }

                            if (it->what == TYPE_BITCOUNT)
                            {
                                expr = (dml::BitCount(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitCount(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BITNOT)
                            {
                                expr = (dml::BitNot(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitNot(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BITAND)
                            {
                                expr = (dml::BitAnd(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitAnd(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BITOR)
                            {
                                expr = (dml::BitOr(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitOr(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BITSL)
                            {
                                expr = (dml::BitShiftLeft(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitShiftLeft(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BITSR)
                            {
                                expr = (dml::BitShiftRight(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitShiftRight(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BITXOR)
                            {
                                expr = (dml::BitXor(mop.Item(whati[0]), mop.Item(whati[1])));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BitXor(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BATCHNORMALIZATION)
                            {
                                expr = dml::BatchNormalization(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), mop.Item(whati[3]), mop.Item(whati[4]), it->Params[0], it->Params[1]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::BatchNormalization(mop.Item(%i),mop.Item(%i),mop.Item(%i),mop.Item(%i),mop.Item(%i),%.2f,%.2f));)", whati[0], whati[1], whati[2], whati[3], whati[4], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_BATCHNORMALIZATIONGRAD)
                            {
                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::BatchNormalizationGrad(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), mop.Item(whati[3]), mop.Item(whati[4]), it->Params[0]);
                                auto& b = std::any_cast<dml::BatchNormalizationGradOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.gradient,b.biasGradient,b.scaleGradient })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }

                                node->code = "// Graph includes BatchNormalizationGrad has not yet implemented in the generator. Please use dml::BatchNormalizationGrad.";
                            }
                            if (it->what == TYPE_BATCHNORMALIZATIONTRAINING)
                            {
                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::BatchNormalizationTraining(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]),dml::NullOpt,it->Params[0]);
                                auto& b = std::any_cast<dml::BatchNormalizationTrainingOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.output,b.mean,b.variance })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes BatchNormalizationTraining has not yet implemented in the generator. Please use dml::BatchNormalizationTraining.";
                            }
                            if (it->what == TYPE_BATCHNORMALIZATIONTRAININGGRAD)
                            {
                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::BatchNormalizationTrainingGrad(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), mop.Item(whati[3]), mop.Item(whati[4]), it->Params[0]);
                                auto& b = std::any_cast<dml::BatchNormalizationGradOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.gradient,b.biasGradient,b.scaleGradient })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes BatchNormalizationTrainingGrad has not yet implemented in the generator. Please use dml::BatchNormalizationTrainingGrad.";
                            }
                        
                            if (it->what == TYPE_CAST)
                            {
                                expr = (dml::Cast(mop.Item(whati[0]), (DML_TENSOR_DATA_TYPE)it->OpType));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Cast(mop.Item(%i),%S));)", whati[0], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_CEIL)
                            {
                                expr = (dml::Ceil(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Ceil(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_CLIP)
                            {
                                expr = (dml::Clip(mop.Item(whati[0]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Clip(mop.Item(%i),%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_CLIPGRAD)
                            {
                                expr = (dml::ClipGrad(mop.Item(whati[0]), mop.Item(whati[1]), it->Params[0], it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ClipGrad(mop.Item(%i),mop.Item(%i),%.2f,%.2f));)", whati[0], whati[1], (float)it->Params[0], (float)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_CONVOLUTION)
                            {
                                dml::Optional<dml::Expression> e3;
                                if (whati.size() > 2)
                                    e3 = mop.Item(whati[2]);
                                expr = (dml::Convolution(mop.Item(whati[0]), mop.Item(whati[1]), e3, (DML_CONVOLUTION_MODE)(int)it->Params[0]));
                                if (whati.size() > 2)
    							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Convolution(mop.Item(%i),mop.Item(%i),mop.Item(%i),(DML_CONVOLUTION_MODE)%i));)", whati[0], whati[1], whati[2], (int)it->Params[0]);
                                else
                                    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Convolution(mop.Item(%i),mop.Item(%i),{},(DML_CONVOLUTION_MODE)%i));)", whati[0], whati[1], (int)it->Params[0]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_CONVOLUTIONINTEGER)
                            {
                                dml::Optional<dml::Expression> e1,e2;
                                if (whati.size() > 1)
                                    e1 = mop.Item(whati[1]);
                                if (whati.size() > 2)
                                    e2 = mop.Item(whati[2]);
                                expr = dml::ConvolutionInteger(mop.Item(whati[0]), e1,mop.Item(whati[1]),e2,
                                    TensorFromString<unsigned int>(it->Params[0]), TensorFromString<unsigned int>(it->Params[1]), TensorFromString<unsigned int>(it->Params[2]), TensorFromString<unsigned int>(it->Params[3]), 
                                    it->Params[4], TensorFromString<unsigned int>(it->Params[5]));
                                node->code = "// Graph includes ConvolutionInteger has not yet implemented in the generator. Please use dml::ConvolutionInteger.";
                            }
                        
                            if (it->what == TYPE_COS)
                            {
                                expr = (dml::Cos(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Cos(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_COSH)
                            {
                                expr = (dml::Cosh(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Cosh(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_CUMPROD)
                            {
                                expr = dml::CumulativeProduct(mop.Item(whati[0]), it->Params[0], (DML_AXIS_DIRECTION)(int)it->Params[1], (bool)it->Params[2]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::CumulativeProduct(mop.Item(%i),%i,(DML_AXIS_DIRECTION)%i,%s));)", whati[0], (uint32_t)it->Params[0], (int)it->Params[1], it->Params[2] ? "true" : "false");
							    node->code = the_code;
                            }
                            if (it->what == TYPE_CUMSUM)
                            {
                                expr = dml::CumulativeSummation(mop.Item(whati[0]), it->Params[0], (DML_AXIS_DIRECTION)(int)it->Params[1], (bool)it->Params[2]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::CumulativeSummation(mop.Item(%i),%i,(DML_AXIS_DIRECTION)%i,%s));)", whati[0], (uint32_t)it->Params[0], (int)it->Params[1], it->Params[2] ? "true" : "false");
							    node->code = the_code;
                            }

                            if (it->what == TYPE_DIVIDE)
                            {
                                expr = (dml::Divide(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Divide(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_DEPTHTOSPACE)
                            {
                                expr = (dml::DepthToSpace(mop.Item(whati[0]), (unsigned int)it->Params[0], (DML_DEPTH_SPACE_ORDER)(int)it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::DepthToSpace(mop.Item(%i),%u,(DML_DEPTH_SPACE_ORDER)%i));)", whati[0], (unsigned int)it->Params[0], (int)it->Params[1]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_DEQUANTIZE)
                            {
                                std::vector<dml::Expression> ll;
							    for (int i5 = 1; i5 < whati.size(); i5++)
								    ll.push_back(mop.Item(whati[i5]));   
                                int jt = it->Params[0];
                                jt++;
                                expr = dml::Dequantize(mop.Item(whati[0]), ll,(DML_QUANTIZATION_TYPE)jt);
                                node->code = "// Graph includes Dequantize has not yet implemented in the generator. Please use dml::Dequantize.";
                            }
                            if (it->what == TYPE_DEQUANTIZELINEAR)
                            {
                                expr = (dml::DequantizeLinear(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::DequantizeLinear(mop.Item(%i),mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1], whati[2]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_DIFFERENCESQUARE)
                            {
                                expr = (dml::DifferenceSquare(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::DifferenceSquare(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_ERF)
                            {
                                expr = (dml::Erf(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Erf(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_EXP)
                            {
                                expr = (dml::Exp(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Exp(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_EQUALS)
                            {
                                expr = (dml::Equals(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Equals(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }


                            if (it->what == TYPE_FLOOR)
                            {
                                expr = (dml::Floor(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Floor(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_GATHER)
                            {
                                expr = dml::Gather(mop.Item(whati[0]), mop.Item(whati[1]), it->Params[0], it->Params[1]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Gather(mop.Item(%i),mop.Item(%i),%i,%i));)", whati[0], whati[1], (int)it->Params[0], (int)it->Params[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_GATHERELEMENTS)
                            {
                                expr = (dml::GatherElements(mop.Item(whati[0]), mop.Item(whati[1]), it->Params[0]));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::GatherElements(mop.Item(%i),mop.Item(%i),%i));)", whati[0], whati[1], (int)it->Params[0]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_GATHERND)
                            {
                                expr = dml::GatherND(mop.Item(whati[0]), mop.Item(whati[1]), it->Params[0], it->Params[1], it->Params[2]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::GatherND(mop.Item(%i),mop.Item(%i),%i,%i,%i));)", whati[0], whati[1], (int)it->Params[0], (int)it->Params[1], (int)it->Params[2]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_GEMM)
                            {
                                dml::Optional<dml::Expression> e3;
							    if (whati.size() > 2)
								    e3 = mop.Item(whati[2]);
                                expr = (dml::Gemm(mop.Item(whati[0]), mop.Item(whati[1]),  e3, (DML_MATRIX_TRANSFORM)(int)it->Params[0], (DML_MATRIX_TRANSFORM)(int)it->Params[1], it->Params[2], it->Params[3]));
                                if (whati.size() > 2)
                                    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Gemm(mop.Item(%i),mop.Item(%i),mop.Item(%i),(DML_MATRIX_TRANSFORM)%i,(DML_MATRIX_TRANSFORM)%i,%.2f,%.2f));)", whati[0], whati[1], whati[2], (int)it->Params[0], (int)it->Params[1], (float)it->Params[2], (float)it->Params[3]);
                                else
                                    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Gemm(mop.Item(%i),mop.Item(%i),{},(DML_MATRIX_TRANSFORM)%i,(DML_MATRIX_TRANSFORM)%i,%.2f,%.2f));)", whati[0], whati[1],  (int)it->Params[0], (int)it->Params[1], (float)it->Params[2], (float)it->Params[3]);
                                node->code = the_code;
                            }
                            if (it->what == TYPE_GREATERTHAN)
                            {
                                expr = (dml::GreaterThan(mop.Item(whati[0]), mop.Item(whati[1]), (DML_TENSOR_DATA_TYPE)it->OpType));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::GreaterThan(mop.Item(%i),mop.Item(%i),%S));)", whati[0], whati[1], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_GREATERTHANOREQUAL)
                            {
                                expr = (dml::GreaterThanOrEqual(mop.Item(whati[0]), mop.Item(whati[1]), (DML_TENSOR_DATA_TYPE)it->OpType));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::GreaterThanOrEqual(mop.Item(%i),mop.Item(%i),%S));)", whati[0], whati[1], optypes2[it->OpType].c_str());  
							    node->code = the_code;
                            }

                            if (it->what == TYPE_GRU)
                            {
                                 // TODO
                            }

                            if (it->what == TYPE_IDENTITY)
                            {
                                expr = (dml::Identity(mop.Item(whati[0])));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Identity(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_IF)
                            {
                                expr = (dml::If(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::If(mop.Item(%i),mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1], whati[2]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ISINFINITY)
                            {
                                expr = dml::IsInfinity(mop.Item(whati[0]), (DML_IS_INFINITY_MODE)(int)(it->Params[0]), (DML_TENSOR_DATA_TYPE)it->OpType);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::IsInfinity(mop.Item(%i),(DML_IS_INFINITY_MODE)%i,%S));)", whati[0], (int)it->Params[0], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ISNAN)
                            {
                                expr = (dml::IsNaN(mop.Item(whati[0]), (DML_TENSOR_DATA_TYPE)it->OpType));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::IsNaN(mop.Item(%i),%S));)", whati[0], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_JOIN)
                            {
							    std::vector<dml::Expression> v;
							    for (size_t fi = 0; fi < whati.size(); fi++)
							    {
								    v.push_back(mop.Item(whati[fi]));
							    }
                                expr = dml::Join(v, (UINT)it->Params[0]);
                                node->code = "// Graph includes Join has not yet implemented in the generator. Please use dml::Join.";
                            }


                            if (it->what == TYPE_LAND)
                            {
                                expr = (dml::LogicalAnd(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LogicalAnd(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LOR)
                            {
                                expr = (dml::LogicalOr(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LogicalOr(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LXOR)
                            {
                                expr = (dml::LogicalXor(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LogicalXor(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LNOT)
                            {
                                expr = (dml::LogicalNot(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LogicalNot(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LOG)
                            {
                                expr = (dml::Log(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Log(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LESSTHAN)
                            {
                                expr = (dml::LessThan(mop.Item(whati[0]), mop.Item(whati[1]), (DML_TENSOR_DATA_TYPE)it->OpType));
                                sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LessThan(mop.Item(%i),mop.Item(%i),%S));)", whati[0], whati[1], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LESSTHANOREQUAL)
                            {
                                expr = (dml::LessThanOrEqual(mop.Item(whati[0]), mop.Item(whati[1]), (DML_TENSOR_DATA_TYPE)it->OpType));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LessThanOrEqual(mop.Item(%i),mop.Item(%i),%S));)", whati[0], whati[1], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_LOCALRESPONSENORMALIZATION)
                            {
                                expr = (dml::LocalResponseNormalization(mop.Item(whati[0]), it->Params[0], it->Params[1], it->Params[2], it->Params[3], it->Params[4]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::LocalResponseNormalization(mop.Item(%i),%.2f,%.2f,%.2f,%.2f,%.2f));)", whati[0], (float)it->Params[0], (float)it->Params[1], (float)it->Params[2], (float)it->Params[3], (float)it->Params[4]);
							    node->code = the_code;
                                
                            }
                            if (it->what == TYPE_MAX)
                            {
                                expr = (dml::Max(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Max(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_MAXPOOLING)
                            {
                                if (!it->MultipleOpOutputData.has_value())
								    it->MultipleOpOutputData = dml::MaxPooling(mop.Item(whati[0]), TensorFromString<unsigned int>(it->Params[0]), TensorFromString<unsigned int>(it->Params[1]), TensorFromString<unsigned int>(it->Params[2]), TensorFromString<unsigned int>(it->Params[3]), TensorFromString<unsigned int>(it->Params[4]), (bool)(it->Params[5]), TensorFromString<unsigned int>(it->Params[6]));
                                auto& b = std::any_cast<dml::MaxPoolingOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.values,b.indices })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes MaxPooling has not yet implemented in the generator. Please use dml::MaxPooling.";
                            }

                            if (it->what == TYPE_MEAN)
                            {
                                expr = (dml::Mean(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Mean(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                        

                            if (it->what == TYPE_MEANVARIANCENORMALIZATION)
                            {
                                dml::Optional<dml::Expression> e2;
                                if (whati.size() > 1)
                                    e2 = mop.Item(whati[1]);
                                dml::Optional<dml::Expression> e3;
                                if (whati.size() > 2)
                                    e3 = mop.Item(whati[2]);

							    expr = (dml::MeanVarianceNormalization(mop.Item(whati[0]), e2, e3, TensorFromString(it->Params[0]), it->Params[1],it->Params[2],it->Params[3]));
                                node->code = "// Graph includes MeanVarianceNormalization has not yet implemented in the generator. Please use dml::MeanVarianceNormalization.";
                            }

                            if (it->what == TYPE_MIN)
                            {
                                expr = (dml::Min(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Min(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_MULTIPLY)
                            {
                                expr = (dml::Multiply(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Multiply(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_MODULUSFLOOR)
                            {
                                expr = (dml::ModulusFloor(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ModulusFloor(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
                            }
                            if (it->what == TYPE_MODULUSTRUNCATE)
                            {
                                expr = (dml::ModulusTruncate(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ModulusTruncate(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_NEGATE)
                            {
                                expr = (dml::Negate(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Negate(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_NONZEROCOORDINATES)
                            {
                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::NonZeroCoordinates(mop.Item(whati[0]));
                                auto& b = std::any_cast<dml::NonZeroCoordinatesOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.count,b.coordinates })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes NonZeroCoordinates has not yet implemented in the generator. Please use dml::NonZeroCoordinates.";
                            }


                            if (it->what == TYPE_ONEHOT)
                            {
                                expr = dml::OneHot(mop.Item(whati[0]), mop.Item(whati[1]), it->Params[0], it->Params[1]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::OneHot(mop.Item(%i),mop.Item(%i),%i,%i));)", whati[0], whati[1], (int)it->Params[0], (int)it->Params[1]); 
							    node->code = the_code;  
                            }
                            if (it->what == TYPE_PADDING)
                            {
							    expr = dml::Padding(mop.Item(whati[0]),
                                    (DML_PADDING_MODE)(int)it->Params[0], it->Params[1], TensorFromString<unsigned int>(it->Params[2]), TensorFromString<unsigned int>(it->Params[3]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Padding(mop.Item(%i),(DML_PADDING_MODE)%i,%.2f,{%S},{%S}));)", whati[0], (int)it->Params[0], (float)it->Params[1], TensorStringToString(it->Params[2].v).c_str(), TensorStringToString(it->Params[3].v).c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_POW)
                            {
                                expr = (dml::Pow(mop.Item(whati[0]), it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Pow(mop.Item(%i),%.2f));)", whati[0], (float)it->Params[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_QUANTIZELINEAR)
                            {
                                expr = (dml::QuantizeLinear(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), (DML_TENSOR_DATA_TYPE)it->OpType));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::QuantizeLinear(mop.Item(%i),mop.Item(%i),mop.Item(%i),%S));)", whati[0], whati[1], whati[2], optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_QUANTIZEDLINEARCONVOLUTION)
                            {
                                expr = dml::QuantizedLinearConvolution(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[5]), mop.Item(whati[2]), mop.Item(whati[3]), mop.Item(whati[6]), mop.Item(whati[7]), mop.Item(whati[4]), mop.Item(whati[8]),
                                    (DML_TENSOR_DATA_TYPE)it->OpType,
                                    TensorFromString<unsigned int>(it->Params[0]), TensorFromString<unsigned int>(it->Params[1]), TensorFromString<unsigned int>(it->Params[2]), TensorFromString<unsigned int>(it->Params[3]),
                                    it->Params[4], TensorFromString<unsigned int>(it->Params[5]));
                                node->code = "// Graph includes QuantizedLinearConvolution has not yet implemented in the generator. Please use dml::QuantizedLinearConvolution.";
                            }


                            if (it->what == TYPE_RANDOMGENERATOR)
                            {
                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::RandomGenerator(mop.Item(whati[0]), TensorFromString<unsigned int>(it->Params[0]), (bool)(it->Params[1]));
                                auto& b = std::any_cast<dml::RandomGeneratorOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.values,b.state })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes RandomGenerator has not yet implemented in the generator. Please use dml::RandomGenerator.";
                            }


                            if (it->what == TYPE_RECIP)
                            {
                                expr = (dml::Recip(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Recip(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_ROUND)
                            {
                                expr = (dml::Round(mop.Item(whati[0]), (DML_ROUNDING_MODE)(int)(it->Params[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Round(mop.Item(%i),(DML_ROUNDING_MODE)%i));)", whati[0], (int)it->Params[0]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_ROIALIGN)
                            {
                                expr = dml::RoiAlign(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), (DML_REDUCE_FUNCTION)(int)(it->Params[0]),
                                    (DML_INTERPOLATION_MODE)(int)(it->Params[1]), it->Params[2], it->Params[3], it->Params[4], it->Params[5], it->Params[6], it->Params[7], it->Params[8],
                                    it->Params[9], it->Params[10], it->Params[11]);
							    node->code = "// Graph includes RoiAlign has not yet implemented in the generator. Please use dml::RoiAlign.";
                            }

                            if (it->what == TYPE_ROIALIGNGRAD)
                            {
                                dml::Expression ino;

                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::RoiAlignGrad(ino,mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), (DML_REDUCE_FUNCTION)(int)(it->Params[0]),
                                        (DML_INTERPOLATION_MODE)(int)(it->Params[1]), it->Params[2], it->Params[3], it->Params[4], it->Params[5], it->Params[6], it->Params[7], it->Params[8],
                                        it->Params[9], it->Params[10], it->Params[11], it->Params[12], it->Params[13]);
                                auto& b = std::any_cast<dml::RoiAlignGradOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.outputGradient,b.outputROIGradient})
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes RoiAlignGrad has not yet implemented in the generator. Please use dml::RoiAlignGrad.";

                            }

                            if (it->what == TYPE_REDUCE)
                            {
                                expr = dml::Reduce(mop.Item(whati[0]), (DML_REDUCE_FUNCTION)(int)(it->Params[0]), TensorFromString<unsigned int>(it->Params[1]), (DML_TENSOR_DATA_TYPE)it->OpType);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Reduce(mop.Item(%i),(DML_REDUCE_FUNCTION)%i,{%S},%S));)", whati[0], (int)it->Params[0], TensorStringToString(it->Params[1].v).c_str(), optypes2[it->OpType].c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_RESAMPLE)
                            {
                                expr = dml::Resample(mop.Item(whati[0]), TensorFromString<unsigned int>(it->Params[0]),(DML_INTERPOLATION_MODE)(int)it->Params[1],
                                    (DML_AXIS_DIRECTION)(int)it->Params[2],TensorFromString<float>(it->Params[3]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Resample(mop.Item(%i),{%S},(DML_INTERPOLATION_MODE)%i,(DML_AXIS_DIRECTION)%i,{%S}));)", whati[0], TensorStringToString(it->Params[0].v).c_str(), (int)it->Params[1], (int)it->Params[2], TensorStringToString(it->Params[3].v).c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_RESAMPLEGRAD)
                            {
                                expr = dml::ResampleGrad(mop.Item(whati[0]), TensorFromString<unsigned int>(it->Params[0]), (DML_INTERPOLATION_MODE)(int)it->Params[1],
                                    TensorFromString<float>(it->Params[2]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ResampleGrad(mop.Item(%i),{%S},(DML_INTERPOLATION_MODE)%i,{%S}));)", whati[0], TensorStringToString(it->Params[0].v).c_str(), (int)it->Params[1], TensorStringToString(it->Params[2].v).c_str());
							    node->code = the_code;
                            }

                            if (it->what == TYPE_REINTERPRET)
                            {
                                expr = (dml::Reinterpret(mop.Item(whati[0]), (DML_TENSOR_DATA_TYPE)it->OpType, TensorFromString(it->Params[0].v.c_str()), {}));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Reinterpret(mop.Item(%i),%S,{%S},{}));)", whati[0], optypes2[it->OpType].c_str(), TensorStringToString(it->Params[0].v).c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_REVERSESUBSEQUENCES)
                            {
                                expr = (dml::ReverseSubsequences(mop.Item(whati[0]), mop.Item(whati[1]), it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ReverseSubsequences(mop.Item(%i),mop.Item(%i),%i));)", whati[0], whati[1], (int)it->Params[0]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_SCATTERELEMENTS)
                            {
							    expr = dml::ScatterElements(mop.Item(whati[0]), mop.Item(whati[1]), mop.Item(whati[2]), it->Params[0]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ScatterElements(mop.Item(%i),mop.Item(%i),mop.Item(%i),%i));)", whati[0], whati[1], whati[2], (int)it->Params[0]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_SLICE)
                            {
                                std::vector<unsigned int> offsets = TensorFromString(it->Params[0].v.c_str());
                                std::vector<unsigned int> sizes = TensorFromString(it->Params[1].v.c_str());
                                std::vector<int> strides = TensorFromString<int>(it->Params[2].v.c_str());
                                expr = (dml::Slice(mop.Item(whati[0]),offsets,sizes,strides));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Slice(mop.Item(%i),{%S},{%S},{%S}));)", whati[0], TensorStringToString(it->Params[0].v).c_str(), TensorStringToString(it->Params[1].v).c_str(), TensorStringToString(it->Params[2].v).c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_SLICEGRAD)
                            {
							    std::vector<unsigned int> yo = TensorFromString(it->Params[0].v.c_str());
                                std::vector<unsigned int> offsets = TensorFromString(it->Params[1].v.c_str());
                                std::vector<unsigned int> sizes = TensorFromString(it->Params[2].v.c_str());
                                std::vector<int> strides = TensorFromString<int>(it->Params[3].v.c_str());
							    expr = (dml::SliceGrad(mop.Item(whati[0]), yo, offsets, sizes, strides));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::SliceGrad(mop.Item(%i),{%S},{%S},{%S},{%S}));)", whati[0], TensorStringToString(it->Params[0].v).c_str(), TensorStringToString(it->Params[1].v).c_str(), TensorStringToString(it->Params[2].v).c_str(), TensorStringToString(it->Params[3].v).c_str());
							    node->code = the_code;

                            }

                            if (it->what == TYPE_SUBTRACT)
                            {
                                expr = (dml::Subtract(mop.Item(whati[0]), mop.Item(whati[1])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Subtract(mop.Item(%i),mop.Item(%i)));)", whati[0], whati[1]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_SQRT)
                            {
                                expr = (dml::Sqrt(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Sqrt(mop.Item(%i)));)", whati[0]);
                                node->code = the_code;
                            }
                            if (it->what == TYPE_SIGN)
                            {
                                expr = (dml::Sign(mop.Item(whati[0])));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Sign(mop.Item(%i)));)", whati[0]);
							    node->code = the_code;
                            }
                            if (it->what == TYPE_THRESHOLD)
                            {
                                expr = (dml::Threshold(mop.Item(whati[0]), it->Params[0]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Threshold(mop.Item(%i),%.2f));)", whati[0], (float)it->Params[0]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_VALUESCALE2D)
                            {
                                expr = dml::ValueScale2D(mop.Item(whati[0]), it->Params[0], TensorFromString<float>(it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::ValueScale2D(mop.Item(%i),%i,{%S}));)", whati[0], (int)it->Params[0], TensorStringToString(it->Params[1].v).c_str());
							    node->code = the_code;
                            }
                            if (it->what == TYPE_UPSAMLPLE2D)
                            {
                                expr = dml::Upsample2D(mop.Item(whati[0]), DML_SIZE_2D{ (unsigned int)(it->Params[0]), (unsigned int)(it->Params[1]) }, (DML_INTERPOLATION_MODE)(int)it->Params[2]);
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::Upsample2D(mop.Item(%i),{ %i,%i },(DML_INTERPOLATION_MODE)%i));)", whati[0], (int)it->Params[0], (int)it->Params[1], (int)it->Params[2]);
							    node->code = the_code;
                            }

                            if (it->what == TYPE_SPACETODEPTH)
                            {
                                expr = (dml::SpaceToDepth(mop.Item(whati[0]), (unsigned int)it->Params[0], (DML_DEPTH_SPACE_ORDER)(int)it->Params[1]));
							    sprintf_s(the_code, 1000, R"(mop.AddItem(dml::SpaceToDepth(mop.Item(%i),%i,(DML_DEPTH_SPACE_ORDER)%i));)", whati[0], (int)it->Params[0], (int)it->Params[1]);
							    node->code = the_code;
                            }
                        
                            if (it->what == TYPE_TOPK)
                            {
                                if (!it->MultipleOpOutputData.has_value())
                                    it->MultipleOpOutputData = dml::TopK(mop.Item(whati[0]),it->Params[0],it->Params[1],(DML_AXIS_DIRECTION)(int)it->Params[2]);
                                auto& b = std::any_cast<dml::TopKOutputs&>(it->MultipleOpOutputData);
                                for (auto& ope : { b.value,b.index })
                                {
                                    mop.AddItem(ope, 0, false, BINDING_MODE::NONE);
                                    node->tidxs.push_back(tidx++);
                                }
                                node->code = "// Graph includes TopK has not yet implemented in the generator. Please use dml::TopK.";
                            }


                            if (expr)
                                Y = 1;
                        }

                        if (Y)
                        {
                            LPARAM tag = 0;
                            bool NB = 0;
                            BINDING_MODE BI = BINDING_MODE::NONE;
                            if (node->BufferVisible)
                                NB = 1;

                            if (node->csv_output.length() || NB == 1)
                            {
                                NB = 1;
                                BI = BINDING_MODE::BIND_OUT;
                            }

                            mop.AddItem(expr, tag, NB, BI);

                            node->tidxs.push_back(tidx++);
                            continue;
                        }


                    }
                }


                if (op.Active == 0)
                    ml.ops.push_back(mop);
                else
                    ml.ops.push_back(mop.Build());
            }
            hres = S_OK;
        }
        //catch(std::exception& e)
        catch (...)
        {
//            auto ce = std::current_exception();
  //          auto tye = typeid(ce).name();
            auto w = "Compilation error!";// e.what();
            Clean();
            MessageBoxA((HWND)wnd(), w, "Compile error", MB_ICONERROR);
        }
        FullRefresh();
        UpdateVideoMemory();
        return hres;
    }



    void MLGraph::Import(XML3::XMLElement& e)
    {
        prj.Unser(e);
    }
    void MLGraph::Export(XML3::XMLElement& e, int idx )
    {
        prj.Ser(e,idx);
    }


    void MLGraph::OnNew(IInspectable const&, IInspectable const&)
    {
//        winrt::VisualDML::MainWindow CreateWi();
 //       CreateWi();
		std::vector<wchar_t> fnx(10000);
		GetModuleFileName(0, fnx.data(), 10000);    
		ShellExecute(0, L"open", fnx.data(), 0, 0, SW_SHOWNORMAL);

    }
    void MLGraph::OnOpen(IInspectable const&, IInspectable const&)
    {
        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)wnd();
        of.lpstrFilter = L"*.dml;\0*.dml\0\0";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        of.nMaxFile = 10000;
        of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (!GetOpenFileName(&of))
            return;
        current_file = fnx.data();
		Import(XML3::XML(fnx.data()).GetRootElement());
        FullRefresh();
    }

    void MLGraph::OnSaveSet(IInspectable const&, IInspectable const&)
    {
        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)wnd();
        of.lpstrFilter = L"*.dml\0\0*.dml";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        of.nMaxFile = 10000;
        of.lpstrDefExt = L"dml";
        of.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&of))
            return;

        DeleteFile(fnx.data());
        XML3::XML x(fnx.data());
        Export(x.GetRootElement(),(int)prj.iActive);
        x.Save();
    }

    void MLGraph::OnSave(IInspectable const&, IInspectable const&)
    {
		if (current_file.empty())
			OnSaveAs({}, {});
		else
		{
            DeleteFile(current_file.c_str());
            XML3::XML x(current_file.c_str());
            Export(x.GetRootElement());
            x.Save();
		}
    }
    void MLGraph::OnExit(IInspectable const&, IInspectable const&)
    {
        PostMessage((HWND)wnd(), WM_CLOSE, 0, 0);
    }
    void MLGraph::OnSaveAs(IInspectable const&, IInspectable const&)
    {
        auto file = current_file;
        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)wnd();
        of.lpstrFilter = L"*.dml\0\0*.dml";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        wcscpy_s(fnx.data(), 10000, file.c_str());
        of.nMaxFile = 10000;
        of.lpstrDefExt = L"dml";
        of.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&of))
            return;

        current_file = fnx.data();
        file = fnx.data();
        OnSave({}, {});
    }

}
