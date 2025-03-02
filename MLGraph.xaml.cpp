#include "pch.h"
#include "MLGraph.xaml.h"
#if __has_include("MLGraph.g.cpp")
#include "MLGraph.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.



std::shared_ptr<XLNODE> MovingNodeP = nullptr;
int MovingNode = 0;
int WhatInput = 0;
PARAM* WhatParam = 0;
D2D1_POINT_2F red_from = { 0,0 }, red_to = { 0,0 };
void XLNODE::Ser(XML3::XMLElement& e)
{
    // Hit
    e.vv("xx").SetValueFloat(hit.left);
    e.vv("yy").SetValueFloat(hit.top);
    e.vv("CSV").SetWideValue(csv_output.c_str());
    e.vv("CSVI").SetWideValue(csv_input.c_str());
	e.vv("bf").SetValueInt(BufferVisible);
	e.vv("sm").SetValueULongLong(ShareMemory);  

    auto& ch = e["Children"];
    for (auto& c : children)
    {
        auto& ce = ch.AddElement("Child");
        ce.vv("idx").SetValueULongLong(c.i);
        ce.vv("io").SetValueULongLong(c.O);


        // All gs
        std::wstring ggs;
		for (auto& gg : c.g)
		{
			ggs += std::to_wstring(gg) + L",";
		}
		if (ggs.length())
			ggs.pop_back();
        ce.vv("u").SetWideValue(ggs.c_str());
    }

}


void XLNODE::Unser(XML3::XMLElement& e)
{
    hit.left = e.vv("xx").GetValueFloat();
    csv_output = e.vv("CSV").GetWideValue();
	csv_input = e.vv("CSVI").GetWideValue();
    hit.top = e.vv("yy").GetValueFloat();
	BufferVisible = e.vv("bf").GetValueInt();
	ShareMemory = e.vv("sm").GetValueULongLong();

    children.clear();
    auto& ch = e["Children"];
    for (size_t i = 0; i < ch.GetChildrenNum(); i++)
    {
        auto& ce = ch.GetChildren()[i];
        XLNODEBULLET bu;
		bu.i = ce->vv("idx").GetValueULongLong();
		bu.O = (bool)ce->vv("io").GetValueULongLong();
        std::vector<std::wstring> split(const std::wstring & s, wchar_t delim);

		auto ggs = ce->vv("u").GetWideValue();
		auto gg = split(ggs, L',');
		for (auto& g : gg)
		{   
			bu.g.push_back(std::stoull(g));
		}
        children.push_back(bu);
    }
}

void XLNODE::Draw(ID2D1DeviceContext5* r, D2D* d2d, size_t iop)
{
	TEXTALIGNPUSH tep(d2d->Text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    auto msr = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text, name().c_str());
    if (std::get<0>(msr) <= 100)
		std::get<0>(msr) = 100;


    wchar_t hdr[100] = {};
    swprintf_s(hdr, 100, L"OP %zi", iop + 1);
    auto msrheader = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text, hdr);

    float hmarg = 10 + std::get<1>(msrheader) + std::max(nin(), nout()) * 10;
    D2D1_RECT_F rtext = { hit.left + 10, hit.top + hmarg, hit.left + 10 + std::get<0>(msr), hit.top + hmarg + std::get<1>(msr) };
    hit.right = rtext.right + 10;
    hit.bottom = rtext.bottom + hmarg;
    r->DrawTextW(name().c_str(), (UINT32)name().length(), d2d->Text, rtext, d2d->BlackBrush);
    D2D1_ROUNDED_RECT rr = { hit, 10,10 };
    if (IsInput())
    {
        d2d->CyanBrush->SetOpacity(0.2f);
        d2d->RedBrush->SetOpacity(0.2f);
        r->FillRoundedRectangle(rr, S ? d2d->RedBrush : d2d->CyanBrush);
        d2d->RedBrush->SetOpacity(1.0f);
        d2d->CyanBrush->SetOpacity(1.0f);
    }
    if (IsOutput())
    {
        d2d->GreenBrush->SetOpacity(0.2f);
        d2d->RedBrush->SetOpacity(0.2f);
        r->FillRoundedRectangle(rr, S ? d2d->RedBrush : d2d->GreenBrush);
        d2d->RedBrush->SetOpacity(1.0f);
        d2d->GreenBrush->SetOpacity(1.0f);
    }
    r->DrawRoundedRectangle(rr, S ? d2d->RedBrush : d2d->BlackBrush);

    if (1)
    {
		D2D1_POINT_2F left = { hit.left + 5, hit.top + std::get<1>(msrheader) + 5};
		D2D1_POINT_2F right = { hit.right - 5, left.y };
		r->DrawLine(left, right, d2d->BlackBrush,0.3f);
		r->DrawTextW(hdr, (UINT32)wcslen(hdr), d2d->Text, D2D1_RECT_F({ hit.left + 5, hit.top + 5, hit.right - 5, hit.top + std::get<1>(msrheader) + 5 }), d2d->BlackBrush);
    }

	children.resize(nin() + nout());    


    float elr = 7.5f;

    for (int i = 0; i < 2; i++)
    {
        size_t Total_Bullets = i == 0 ? nin() : nout();
        float TotalHeight = Total_Bullets * 15.0f;
        float NextY = hit.top + 5;
        float FullHeight = hit.Height();
        NextY = (FullHeight - TotalHeight) / 2.0f + hit.top;
        for (auto& ch : children)
        {
            // Calculate the bullet's position
            if (ch.O == (bool)i)
            {
                ch.hit.top = NextY;
				ch.hit.bottom = NextY + 10;
                ch.hit.left = hit.left;
                if (i == 1)
                    ch.hit.left = hit.right - elr / 2.0f;
                ch.hit.right = ch.hit.left + elr / 2.0f;
				NextY += 15;
            }
        }
    }


    if (nin())
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
            r->FillEllipse(el, Red ? d2d->RedBrush : d2d->BlackBrush);
            ch.hit = D2D1_RECT_F({ el.point.x - el.radiusX, el.point.y - el.radiusY, el.point.x + el.radiusX, el.point.y + el.radiusY });
        }

    }

    if (nout())
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
            r->FillEllipse(el, ch.S ? d2d->RedBrush : d2d->BlackBrush);
            ch.hit =  D2D1_RECT_F({ el.point.x - el.radiusX, el.point.y - el.radiusY, el.point.x + el.radiusX, el.point.y + el.radiusY });
        }
    }

    if (BufferVisible || IsInput() || IsOutput())
    {
        bhit.left = hit.MiddleX() - 10;
		bhit.right = hit.MiddleX() + 10;
        bhit.top = hit.bottom - 5;
		bhit.bottom = hit.bottom + 5;
		D2D1_ROUNDED_RECT rr4 = { bhit, 5,5 };
		r->FillRoundedRectangle(rr4, bSelected ? d2d->RedBrush : d2d->SnapBrush2);
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
        r->FillRoundedRectangle(rr4, Red ? d2d->RedBrush : d2d->SnapBrush2);
    }
}

winrt::Microsoft::UI::Xaml::Controls::MenuFlyout BuildNodeRightMenu(std::shared_ptr<XLNODE> nd,int Type,std::function<void(const winrt::Windows::Foundation::IInspectable, const winrt::Windows::Foundation::IInspectable)> fooo)
{
    winrt::Microsoft::UI::Xaml::Controls::MenuFlyout r1;


    if (Type == 1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"CSV input..."); O.Click(fooo);
        r1.Items().Append(O);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
        r1.Items().Append(s);
    }

    if (1)
    {
        for (size_t i = 0; i < nd->Params.size(); i++)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O;
            O.Text(nd->Params[i].n.c_str());
            O.Tag(winrt::box_value(i + 2000));
            O.Click(fooo);
            r1.Items().Append(O);
        }
    }

    if (Type == 2)
    {
    }

    if (Type  == 1 || Type == 2 || Type == 3)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
        r1.Items().Append(s);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"CSV output..."); O.Click(fooo);
        r1.Items().Append(O);
		winrt::Microsoft::UI::Xaml::Controls::ToggleMenuFlyoutItem O1; O1.Text(L"Buffer Visible"); O1.Click(fooo); O1.IsChecked(nd->BufferVisible);
        r1.Items().Append(O1);
    }

    return r1;
}



winrt::Microsoft::UI::Xaml::Controls::MenuFlyout BuildTensorMenu(std::function<void(const winrt::Windows::Foundation::IInspectable,const winrt::Windows::Foundation::IInspectable)> fooo)
{
    winrt::Microsoft::UI::Xaml::Controls::MenuFlyout r1;


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem O; O.Text(L"Input"); O.Click(fooo);
        r1.Items().Append(O);
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSeparator s;
		r1.Items().Append(s);
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
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Cos"); N.Click(fooo);
            A.Items().Append(N);
        }
        if (1)
        {
            winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem N; N.Text(L"Cosh"); N.Click(fooo);
            A.Items().Append(N);
        }
        r1.Items().Append(A);


    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"D");

        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Divide"); Neg.Click(fooo);
        A.Items().Append(Neg);

        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"M");

        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Multiply"); Neg.Click(fooo);
        A.Items().Append(Neg);

        r1.Items().Append(A);
    }


    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"N");

        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Neg"); Neg.Click(fooo);
        A.Items().Append(Neg);

        r1.Items().Append(A);
    }

    if (1)
    {
        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutSubItem A;
        A.Text(L"S");

        winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItem Neg; Neg.Text(L"Subtract"); Neg.Click(fooo);
        A.Items().Append(Neg);

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

namespace winrt::DirectMLGraph::implementation
{


    void MLGraph::FullRefresh()
    {
        Refresh();
        RefreshMenu();
        Paint();
    }

    void MLGraph::Key(long long k)
    {
        if (k == VK_DELETE)
        {
            for (size_t i = 0; i < xl.ops.size(); i++)
            {
                auto& op = xl.ops[i];
                for (size_t ii = 0; ii < op.nodes.size(); ii++)
                {
                    if (op.nodes[ii]->bSelected)
                    {
                        Push();
                        op.nodes[ii]->ShareMemory = 0;
                        FullRefresh();
                        return;
                    }

                    if (op.nodes[ii]->S)
                    {
                        Push();
                        op.nodes.erase(op.nodes.begin() + ii);
                        FullRefresh();
                        return;
                    }
                    for (size_t iii = 0; iii < op.nodes[ii]->children.size(); iii++)
                    {
                        if (op.nodes[ii]->children[iii].S)
                        {
                            Push();
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

    void MLGraph::OnLoaded(IInspectable, IInspectable)
    {
		if (xl.ops.empty())
            xl.ops.push_back(XLOP());
        ActiveOperator2 = 0;

        auto sp = Content().as<Panel>();
        auto scp = sp.FindName(L"scp").as<SwapChainPanel>();
        static int Once = 0;
        if (Once)
            return;
        Once = 1;

        if (fil.length())
        {
			current_file = fil;
			XML3::XML x(fil.c_str());
			xl.Unser(x.GetRootElement());

        }
        scp.PointerMoved([this](IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& a)
            {
                bool Left = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                if (!Left)
                    return;

                auto scp = sender.as<SwapChainPanel>();
                auto pt = a.GetCurrentPoint(scp);
                auto pos = pt.Position();


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
                                if (MovingNodeP->ShareMemory == 0)
                                    MovingNodeP->ShareMemory = nextn();
								nod->ShareMemory = MovingNodeP->ShareMemory;
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
                                            ch2.g.clear();
                                            auto ne = nextn();
                                            ch2.g.push_back(ne);
                                            ch.g.push_back(ne);
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
                bool Right = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
                bool Left = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);
                if (!Left && !Right)
                    return;
                auto scp = sender.as<SwapChainPanel>();
                auto pt = a.GetCurrentPoint(scp);
                static auto pos = pt.Position();
                pos = pt.Position();

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
                                auto m = BuildNodeRightMenu(nod,ty,[this, i,ii](const winrt::Windows::Foundation::IInspectable from, const winrt::Windows::Foundation::IInspectable)
                                    {
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
                                            if (i3 >= 2000)
                                            {
                                                auto pidx = i3 - 2000;
                                                WhatInput = 2;
                                                _i0 = nod->Params[pidx].n;
												_i1 = std::to_wstring(nod->Params[pidx].v);
                                                WhatParam = &nod->Params[pidx];
                                                Refresh({ L"i1",L"i0" });
                                                auto sp = Content().as<Panel>();
                                                auto ct = sp.FindName(L"Input1").as<ContentDialog>();
                                                ct.ShowAsync();
                                            }
                                        }
                                        if (t == L"CSV input...")
                                        {
											auto it = std::dynamic_pointer_cast<XLNODE_INPUT>(nod);
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
                                            it->csv_input = fnx.data();
                                            FullRefresh();
                                        }
                                        if (t == L"Buffer Visible")
                                        {
											nod->BufferVisible = !nod->BufferVisible;
											nod->ShareMemory = 0;
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
                                            it->csv_output = fnx.data();
                                            FullRefresh();
                                        }

                                    });
                                m.ShowAt(scp, pos);
                                return;
                            }
                        }


                        auto m = BuildTensorMenu([this,i](const winrt::Windows::Foundation::IInspectable from, const winrt::Windows::Foundation::IInspectable)
                            {
                                auto& op = xl.ops[i];

                                MenuFlyoutItem m = from.as<MenuFlyoutItem>();
                                auto t = m.Text();
                                if (t == L"Input")
                                {
                                    OnAddInput({}, {});
                                }
                                if (t == L"Abs")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_ABS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ACos")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_ACOS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ACosh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_ACOSH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ASin")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ASIN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ASinh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ASINH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ATan")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ATAN);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ATanh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_ATANH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"ATanYX")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2,TYPE_ATANYX);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Add")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_ADD);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }


                                if (t == L"BitAnd")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITAND);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitCount")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_BITAND);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitNot")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_BITNOT);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitOr")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitShiftLeft")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITSL);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitShiftRight")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITSR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"BitXor")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_BITXOR);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Ceil")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_CEIL);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Clip")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1,TYPE_CLIP);

                                    node->Params.resize(2);
                                    node->Params[0].n = L"Min";
                                    node->Params[0].v = 0.0f;
                                    node->Params[1].n = L"Max";
                                    node->Params[1].v = 1.0f;

                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Cos")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_COS);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Cosh")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_COSH);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Divide")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_DIVIDE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Multiply")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_MULTIPLY);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }
                                if (t == L"Neg")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(1, TYPE_NEGATE);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }

                                if (t == L"Subtract")
                                {
                                    auto node = std::make_shared<XLNODE_ANY>(2, TYPE_SUBTRACT);
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }


                                if (t == L"Output")
                                {
                                    auto node = std::make_shared<XLNODE_OUTPUT>();
                                    node->hit.left = pos.X;
                                    node->hit.top = pos.Y;
                                    op.nodes.push_back(node);
                                }



                                Refresh();
                            });
                        m.ShowAt(scp, pos);
                    }
                    return;
                }


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
    								ch.S = 1;
								break;
							}
                        }
                        if (Hit(pos.X, pos.Y, op.nodes[ii]->bhit))
                        {
                            MovingNodeP = op.nodes[ii];
                            MovingNode = 2;
                            op.nodes[ii]->bSelected = 1;
                            break;
                        }

                        if (Hit(pos.X, pos.Y, op.nodes[ii]->hit))
                        {
                            MovingNode = 1;
                            op.nodes[ii]->S = 1;
                            break;
                        }
                    }
				}
                Paint();
            });

        Resize();
        RefreshMenu();
    }

    void MLGraph::Resize()
    {
        auto sp = Content().as<Panel>();
        auto men = sp.FindName(L"menu").as<MenuBar>();
        RECT rc = {};
        GetClientRect((HWND)wnd(), & rc);
        auto he = (float)(rc.bottom - rc.top);
        auto wi = (float)(rc.right - rc.left);
        auto scp = sp.FindName(L"scp").as<SwapChainPanel>();
        auto menh = men.ActualHeight();
        if (menh < 0)
            return;
        he -= (float)menh;

//        auto sp1 = sp.FindName(L"sp1").as<StackPanel>();
 //       auto sp2 = sp.FindName(L"sp2").as<StackPanel>();
  //      he -= (float)(sp1.ActualHeight() + sp2.ActualHeight());

        he -= 100;

//        scp.Focus(FocusState::Keyboard);

        scp.Width(wi);
        scp.Height(he);

        if (d2d)
        {
            if (d2d->SizeCreated.cx != wi || d2d->SizeCreated.cy != he)
            {
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
            d2d->CreateD2X(0, (int)wi, (int)he, 1, 0, 0, 1);
        }
        else
            d2d->Resize((int)wi, (int)he);


        IInspectable i = (IInspectable)scp;
        auto p2 = i.as<ISwapChainPanelNative>();
        p2->SetSwapChain(d2d->m_swapChain1);

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

    void MLGraph::OnUndo(IInspectable const&, IInspectable const&)
    {
		if (undo_list.empty())
			return;
        XML3::XMLElement el;
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
		xl.Ser(el);
        undo_list.push(el);
	    el = redo_list.top();
		redo_list.pop();
		xl.Unser(el);
		Refresh();
		RefreshMenu();
    }


    void MLGraph::Paint()
    {
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
                d2d->CreateD2X(0, (int)wi, (int)he, 1, 0, 0, 1);
            }

        }
        if (!d2d)
            return;
        d2d->m_d2dContext->BeginDraw();
        d2d->m_d2dContext->Clear({ 1,1,1,1 });


        if (!d2d->WhiteBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 1,1,1,1 }, &d2d->WhiteBrush);
        if (!d2d->BlackBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0,0,0,1 }, &d2d->BlackBrush);
        if (!d2d->RedBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 1,0,0,1 }, &d2d->RedBrush);
        if (!d2d->CyanBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({0,0,1,1 }, &d2d->CyanBrush);
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
            fs *= 2.0f;
            d2d->WriteFa->CreateTextFormat(lf.lfFaceName, 0, lf.lfWeight > 500 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, fst, fsr, fs, L"", &d2d->Text);
            fs *= 2.0f;
            d2d->WriteFa->CreateTextFormat(lf.lfFaceName, 0, lf.lfWeight > 500 ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, fst, fsr, fs, L"", &d2d->Text3);
        }
        D2D1_RECT_F rfull = { 0,0,(float)scp.ActualWidth(),(float)scp.ActualHeight() };
        rfull.bottom = (float)scp.Height();
        auto r = d2d->m_d2dContext5;



        // Draw Graph
		for (size_t i = 0; i < xl.ops.size(); i++)
		{
			auto& op = xl.ops[i];
            if (op.Visible == 0)
                continue;
			for (size_t j = 0; j < op.nodes.size(); j++)
			{
                auto& node = op.nodes[j];
				node->Draw(r, d2d.get(),i);   
			}


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
                                            if (gg2 == gg)
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
                            }
                        }
					}
				}
            }
		}

        // Shared Memory
        for (auto& op : xl.ops)
        {
            for (auto& n1 : op.nodes)
            {
                if (n1->ShareMemory == 0)
                    continue;
                for (auto& op2 : xl.ops)
                {
                    for (auto& n2 : op2.nodes)
                    {
                        if (n2->ShareMemory == 0)
                            continue;
                        if (n1.get() == n2.get())
							continue;
						if (n1->ShareMemory == n2->ShareMemory && n2->IsInput())
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


        [[maybe_unused]] auto hr = d2d->m_d2dContext->EndDraw();
        if (d2d->m_swapChain1)
            hr = d2d->m_swapChain1->Present(1, 0);
    }

    void MLGraph::OnAddOp(IInspectable const&, IInspectable const&)
    {
        Push();
		xl.ops.push_back(XLOP());
        ActiveOperator2 = xl.ops.size() - 1;
        FullRefresh();
    }


    void MLGraph::Push()
    {
        XML3::XMLElement e;
        xl.Ser(e);
		undo_list.push(e);
    }

    void MLGraph::RefreshMenu()
    {
        auto sp = Content().as<Panel>();
        auto m1 = sp.FindName(L"ActiveOperatorSubmenu").as<MenuFlyoutSubItem>();
		m1.Items().Clear();

        // Add operators as radio
		for (size_t i = 0; i < xl.ops.size(); i++)
		{
			auto mi = RadioMenuFlyoutItem();
			mi.Text(ystring().Format(L"Operator %zi",i + 1));
			mi.Click([this, i](IInspectable const&, RoutedEventArgs const&)
				{
					ActiveOperator2 = i;
					Refresh();
				});
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
					xl.ops[i].Visible = !xl.ops[i].Visible;
                    Refresh();
                });
            m3.Items().Append(mi);
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
                    xl.ops.erase(xl.ops.begin() + i);
					if (ActiveOperator2 >= xl.ops.size())
						ActiveOperator2 = xl.ops.size() - 1;
                    FullRefresh();
				});
			m2.Items().Append(mi);
        }
    }

    void MLGraph::Input_Completed(IInspectable, IInspectable)
    {
        if (WhatInput == 2 && WhatParam)
        {
            try
            {
                WhatParam->v = std::stof(_i1);
            }
            catch (...)
            {

            }
			FullRefresh();
            return;
        }
        if (WhatInput == 1)
        {
            // Input Tensor Dimensions
            auto& op = xl.ops[ActiveOperator2];
            std::vector<unsigned int> dims; // convert string to dims, split in x
            std::vector<std::wstring> split(const std::wstring & s, wchar_t delim);

			auto sp = split(_i1, L'x');
			for (auto& s : sp)
				dims.push_back(std::stoi(s));
            try
            {
                Push();
                auto t = std::make_shared<XLNODE_INPUT>();
				t->hit = D2D1_RECT_F({ 10,10,100,100 });
				t->tensor_dims = dims;
				op.nodes.push_back(t);
            }
            catch (...)
            {

            }
            FullRefresh();
        }
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
		auto& op = xl.ops[ActiveOperator2];
        auto node = std::make_shared<XLNODE_OUTPUT>();
        op.nodes.push_back(node);
        FullRefresh();
    }

    void MLGraph::OnRun(IInspectable const&, IInspectable const&)
    {
        void Locate(const wchar_t* fi);

        // Initialize
        if (ml.d3D12Device == 0)
			OnCompile({}, {}); 
        if (ml.d3D12Device == 0)
            return;
        if (ml.ops.size() != xl.ops.size())
            return;

        // Load csv files in inputs
        ml.Prepare();

        for (size_t iop = 0 ; iop < xl.ops.size() ; iop++)
		{
            auto& op = xl.ops[iop];
            auto& mlop = ml.ops[iop];
			for (auto& node : op.nodes)
			{
				if (auto it = std::dynamic_pointer_cast<XLNODE_INPUT>(node))
				{
                    if (it->ShareMemory)
                        continue;
					if (it->csv_input.length())
					{
						std::ifstream f(it->csv_input);
						if (f.is_open())
						{
                            std::vector<float> v;
                            std::string line;
							while (std::getline(f, line))
							{
                                std::vector<std::string> split(const std::string & s, char delim);
								std::vector<std::string> sp = split(line, ',');
                                for (auto& s : sp)
                                {
                                    if (isalpha(s[0]))
                                        continue;
                                    v.push_back(std::stof(s));
                                }
							}

                            auto& wh = mlop.Item(it->tidx);
                            if (!wh.buffer)
                                continue;
                            wh.buffer->Upload(&ml, v.data(), v.size() * sizeof(float));
						}
					}
				}
			}


            ml.Run(iop);

            for (auto& node : op.nodes)
            {
                if (auto it = std::dynamic_pointer_cast<XLNODE>(node))
                {
                    if (it->csv_output.length())
                    {
                        DeleteFile(it->csv_output.c_str());
                        auto& wh = mlop.Item(node->tidx);
                        if (!wh.buffer)
                            continue;
                        std::vector<char> v;
                        wh.buffer->Download(&ml, (size_t)-1, v);
                        std::vector<float> fv(v.size() / 4);
                        memcpy(fv.data(), v.data(), v.size());
                        std::ofstream f(it->csv_output);
                        if (f.is_open())
                        {
                            for (size_t i = 0; i < fv.size(); i++)
                            {
                                f << fv[i] << std::endl;
                            }
                        }
                        Locate(it->csv_output.c_str());
                    }
                }
            }
		}

        ml = {};
    }


    void MLGraph::OnCompile(IInspectable const&, IInspectable const&)
    {
        ml = {};
        try
        {
            ml.On();
            for (size_t i = 0; i < xl.ops.size(); i++)
            {
                int tidx = 0;
                auto& op = xl.ops[i];
                MLOP mop(&ml);

				std::sort(op.nodes.begin(), op.nodes.end(), [](auto& a, auto& b) 
                    { 
						if (a->IsInput() && !b->IsInput())
							return 1;   
                        if (!a->IsOutput() && b->IsOutput())
                            return 1;
                        return 0;
                    });    

                for (size_t ii = 0; ii < op.nodes.size(); ii++)
                {
                    auto& node = op.nodes[ii];
                    [[maybe_unused]] auto str = node->name();
                    if (auto it = std::dynamic_pointer_cast<XLNODE_INPUT>(node))
                    {
                        std::optional<MLRESOURCE> mlr;
                        if (node->ShareMemory)
                        {
                            // Find other 
                            int remote_tid = -1;
                            size_t iop = 0;
                            for (size_t ii3 = 0 ; ii3 < xl.ops.size() ; ii3++)
                            {
                                auto& op3 = xl.ops[ii3];
                                for (auto& n : op3.nodes)
                                {
                                    if (n == node)
                                        continue;
                                    if (n->ShareMemory == node->ShareMemory)
                                    {
                                        remote_tid = n->tidx;
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
                                }
                            }
                        }


                        mop.AddInput({ DML_TENSOR_DATA_TYPE_FLOAT32, it->tensor_dims },0,mlr ? false : true,BINDING_MODE::BIND_IN,mlr);
                        node->tidx = tidx++;
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
                                for (auto& ch : j->children)
                                {
                                    for (auto& chg : ch.g)
                                    {
                                        if (chg == input_gg)
                                        {
                                            F = 1;
                                            whati.push_back(j->tidx);
                                            break;
                                        }
                                    }
                                    if (F)
                                        break;
                                }
                                if (F)
                                    break;
                            }
                        }
                    }

                    if (auto it = std::dynamic_pointer_cast<XLNODE_OUTPUT>(node))
                    {
						if (whati.size() != 1)
							continue;
                        mop.AddOutput(mop.Item(whati[0]));
                        node->tidx = tidx++;
                        continue;
                    }

                    dml::Expression expr;
                    bool Y = false;

                    if (auto it = std::dynamic_pointer_cast<XLNODE_ANY>(node))
                    {
                        if (whati.size() == 0)
                            continue;
                        if (it->nin() != whati.size())
                            continue;

                        if (it->what == TYPE_ABS)
                            expr = dml::Abs(mop.Item(whati[0]));
                        if (it->what == TYPE_ACOS)
                            expr = (dml::ACos(mop.Item(whati[0])));
                        if (it->what == TYPE_ACOSH)
                            expr = (dml::ACosh(mop.Item(whati[0])));
                        if (it->what == TYPE_ASIN)
                            expr = (dml::ASin(mop.Item(whati[0])));
                        if (it->what == TYPE_ASINH)
                            expr = (dml::ASinh(mop.Item(whati[0])));
                        if (it->what == TYPE_ATAN)
                            expr = (dml::ATan(mop.Item(whati[0])));
                        if (it->what == TYPE_ATANH)
                            expr = (dml::ATanh(mop.Item(whati[0])));


                        if (it->what == TYPE_BITCOUNT)
                            expr = (dml::BitCount(mop.Item(whati[0])));
                        if (it->what == TYPE_BITNOT)
                            expr = (dml::BitNot(mop.Item(whati[0])));


                        if (it->what == TYPE_CEIL)
                            expr = (dml::Ceil(mop.Item(whati[0])));
                        if (it->what == TYPE_CLIP)
                            expr = (dml::Clip(mop.Item(whati[0]),it->Params[0].v, it->Params[1].v));
                        if (it->what == TYPE_COS)
                            expr = (dml::Cos(mop.Item(whati[0])));
                        if (it->what == TYPE_COSH)
                            expr = (dml::Cosh(mop.Item(whati[0])));


                        if (it->what == TYPE_NEGATE)
                            expr = (dml::Negate(mop.Item(whati[0])));
                        
                        if (it->what == TYPE_ADD)
                            expr = (dml::Add(mop.Item(whati[0]), mop.Item(whati[1])));
                        if (it->what == TYPE_ATANYX)
                            expr = (dml::ATanYX(mop.Item(whati[0]), mop.Item(whati[1])));

                        if (it->what == TYPE_BITAND)
                            expr = (dml::BitAnd(mop.Item(whati[0]), mop.Item(whati[1])));
                        if (it->what == TYPE_BITOR)
                            expr = (dml::BitOr(mop.Item(whati[0]), mop.Item(whati[1])));
                        if (it->what == TYPE_BITSL)
                            expr = (dml::BitShiftLeft(mop.Item(whati[0]), mop.Item(whati[1])));
                        if (it->what == TYPE_BITSR)
                            expr = (dml::BitShiftRight(mop.Item(whati[0]), mop.Item(whati[1])));
                        if (it->what == TYPE_BITXOR)
                            expr = (dml::BitXor(mop.Item(whati[0]), mop.Item(whati[1])));

                        if (it->what == TYPE_DIVIDE)
                            expr = (dml::Divide(mop.Item(whati[0]), mop.Item(whati[1])));

                        if (it->what == TYPE_MULTIPLY)
                            expr = (dml::Multiply(mop.Item(whati[0]), mop.Item(whati[1])));

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

                        node->tidx = tidx++;
                        continue;
                    }


                }


                ml.ops.push_back(mop.Build());
            }
        }
        catch (...)
        {
            ml = {};
            MessageBox((HWND)wnd(), L"Compilation error!", L"", MB_ICONERROR);
        }

    }


	void TensorDescToXML(dml::TensorDesc& td, XML3::XMLElement& e)
	{
		e.vv("DataType").SetValueInt((int)td.dataType);
		auto& ee = e.AddElement("Dimensions");
		for (auto& d : td.sizes)
		{
			auto& de = ee.AddElement("Dimension");
			de.vv("Size").SetValueInt(d);
		}
        // flags
		e.vv("Flags").SetValueInt(td.flags);

	}

    void XMLToTensorDesc(dml::TensorDesc& td, XML3::XMLElement& e)
    {
		td.dataType = (DML_TENSOR_DATA_TYPE)e.vv("DataType").GetValueInt();
		auto& ee = e["Dimensions"];
        for (size_t i = 0; i < ee.GetChildrenNum() ; i++)
        {
			auto& de = ee.GetChildren()[i];
			td.sizes.push_back(de->vv("Size").GetValueInt());
        }
    }

    void MLGraph::Import(XML3::XMLElement& e)
    {
        xl.Unser(e);

    }
    void MLGraph::Export(XML3::XMLElement& e)
    {
		xl.Ser(e);
    }


    void MLGraph::OnNew(IInspectable const&, IInspectable const&)
    {
        winrt::DirectMLGraph::MainWindow CreateWi();
        CreateWi();

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
