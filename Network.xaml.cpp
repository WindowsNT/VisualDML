#include "pch.h"
#include "Network.xaml.h"
#if __has_include("Network.g.cpp")
#include "Network.g.cpp"
#endif



using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
extern std::vector<CComPtr<IDXGIAdapter1>> all_adapters;

extern bool TrainCancel;
extern std::wstring fil;
extern float ScaleX;

extern std::map<HWND, winrt::Windows::Foundation::IInspectable> windows;
std::wstring TempFile3();

#include "nn.h"
void NNToPythonOnnx(const wchar_t*);
void NNToPythonPTH(const wchar_t*);

extern int CurrentBatch;
extern int NumEpochsX;

void LoadNetworkFile();
// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::VisualDML::implementation
{
    void Network::Refresh(const wchar_t* s)
    {
        m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ s });
        Paint();
    }
    void Network::Refresh(std::vector<std::wstring> strs)
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


    long  Network::IndexOfAct()
    {
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                return (long)nn.Layers[i].ActType;
        }
        return 0;
    }
    void Network::IndexOfAct(long l)
    {
        if (l < 0)
            return;
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                nn.Layers[i].ActType = (ACT_TYPE)l;
        }
    }

    winrt::Windows::Foundation::Collections::IObservableVector<winrt::VisualDML::Item> Network::ActList()
    {
        auto children = single_threaded_observable_vector<VisualDML::Item>();
        auto item = winrt::make<VisualDML::implementation::Item>();
        item.Name1(L"Sigmoid");
        children.Append(item);
        item = winrt::make<VisualDML::implementation::Item>();
        item.Name1(L"ReLu");
        children.Append(item);
        item = winrt::make<VisualDML::implementation::Item>();
        item.Name1(L"Identity");
        children.Append(item);
        item = winrt::make<VisualDML::implementation::Item>();
        item.Name1(L"Leaky ReLu");
        children.Append(item);
        return children;
    }


    winrt::Windows::Foundation::Collections::IObservableVector<winrt::VisualDML::Item> Network::LayerList()
    {
        auto children = single_threaded_observable_vector<VisualDML::Item>();
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            auto item = winrt::make<VisualDML::implementation::Item>();
            if (i == 0)
                item.Name1(std::wstring(s(14)) + std::wstring(L" ") + std::wstring(s(17)));
            else
                if (i == (nn.Layers.size() - 1))
                    item.Name1(std::wstring(s(15)) + std::wstring(L" ") + std::wstring(s(17)));
                else
                    item.Name1(std::wstring(s(16)) + std::wstring(L" ") + std::to_wstring(i));
            children.Append(item);
        }
        return children;
    }

    double Network::BatchNumber()
    {
        return (double)CurrentBatch;
    }
    void Network::BatchNumber(double v)
    {
        CurrentBatch = (int)v;
    }
    double Network::NumEpochs()
    {
        return (double)NumEpochsX;
    }
    void Network::NumEpochs(double v)
    {
        NumEpochsX = (int)v;
    }

    bool Network::InputsVisible()
    {
        for (size_t i = 0; i < nn.Layers.size() && i < 1; i++)
        {
            if (nn.Layers[i].Sel)
                return 1;
        }
        return 0;
    }
    bool Network::CountVisible()
    {
        if (nn.Layers.empty())
            return 0;
        for (size_t i = 1; i < (nn.Layers.size() - 1); i++)
        {
            if (nn.Layers[i].Sel)
                return 1;
        }
        return 0;
    }
    bool Network::OutputsVisible()
    {
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (i == (nn.Layers.size() - 1) && nn.Layers[i].Sel)
                return 1;
        }
        return 0;
    }

    double Network::NumNeurons()
    {
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
            {
                if (i == 0)
                    return (double)nn.Layers[i + 1].weights.rows();
                if (i == (nn.Layers.size() - 1))
                    return (double)nn.Layers[i].biases.cols();
                return (double)nn.Layers[i].weights.cols();
            }
        }
        return 0;
    }
    void Network::NumNeurons(double n)
    {
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
            {
                if (i == 0) // input
                {
                    if (nn.Layers[i + 1].weights.rows() != (int)n)
                    {
                        nn.Layers[i + 1].weights.init((int)n, nn.Layers[i + 1].weights.cols());
                        nn.RecalculateWB();
                        Refresh({ L"WeightsText",L"BiasesText" });
                    }
                }
                else
                    if (i == (nn.Layers.size() - 1)) // output
                    {
                        if (nn.Layers[i].biases.cols() != n)
                        {
                            nn.Layers[i].weights.init(nn.Layers[i].weights.rows(), (int)n);
                            nn.Layers[i].biases.init(1, (int)n);
                            nn.RecalculateWB();
                            Refresh({ L"WeightsText",L"BiasesText" });
                        }
                    }
                    else
                    {
                        if (nn.Layers[i].weights.rows() != (int)n)
                        {
                            nn.Layers[i].weights.init(nn.Layers[i].weights.rows(), (int)n);
                            nn.Layers[i].biases.init(1, (int)n);
                            nn.RecalculateWB();
                            Refresh({ L"WeightsText",L"BiasesText" });
                        }
                    }
                break;
            }
        }
    }


    void Network::OnLoaded(IInspectable, IInspectable)
    {
        Resize();
        nn.Init();

        // And the adapters
        auto sp = Content().as<Panel>();
        auto m31 = sp.FindName(L"AdapterMenu").as<MenuBarItem>();
        if (m31.Items().Size() == 0)
        {
            LoadAdapters();
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
                    });
                m31.Items().Append(mi);
            }
        }

    }

    void Network::LoadAdapters()
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


    void Network::Resize()
    {
        auto sp = Content().as<Panel>();
        auto men = sp.FindName(L"menu").as<MenuBar>();
        RECT rc = {};
        GetClientRect((HWND)wnd(), &rc);
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
        auto menh = men.ActualHeight();
        if (menh < 0)
            return;
        he -= (float)menh;

        auto sp1 = sp.FindName(L"sp1").as<StackPanel>();
        auto sp2 = sp.FindName(L"sp2").as<StackPanel>();
        he -= (float)(sp1.ActualHeight() + sp2.ActualHeight());

        he -= 20;

        scp.Focus(FocusState::Keyboard);

        scp.Width(wi);
        scp.Height(he);


        LoadAdapters();

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
            d2d->CreateD2X(0, 0, (int)wi, (int)he, 1, 0, 0, 1);
        }
        else
            d2d->Resize((int)wi, (int)he);


        IInspectable i = (IInspectable)scp;
        auto p2 = i.as<ISwapChainPanelNative>();
        p2->SetSwapChain(d2d->m_swapChain1);
        iVisiblePage = 1;

        Paint();
    }

    D2D1_ELLIPSE FromR(D2D1_RECT_F r1)
    {
        D2D1_ELLIPSE el2 = {};
        el2.point = { (r1.left + r1.right) / 2, (r1.top + r1.bottom) / 2 };
        el2.radiusX = (r1.right - r1.left) / 2;
        el2.radiusY = (r1.bottom - r1.top) / 2;
        return el2;
    }

    void Network::PaintALayer(D2D1_RECT_F full, void* ly, size_t lidx)
    {
        auto r = d2d->m_d2dContext;
        Layer* l = (Layer*)ly;
        l->DrawnNeurons = {};
        auto numneurons = l->weights.cols();
        if (numneurons == 0)
            numneurons = l->output.cols();
        if (numneurons == 0 && lidx < (nn.Layers.size() - 1))
            numneurons = nn.Layers[lidx + 1].weights.rows();
        if (numneurons == 0)
            return;

        auto nh = (full.right - full.left) - 20;

        int MaxNeuronsHold = (int)((full.bottom - full.top) / nh);
        if (MaxNeuronsHold % 2)
            MaxNeuronsHold--;
        else
            MaxNeuronsHold -= 2;

        float nb = full.bottom;
        bool fi = 1;
        for (int i = 0; i < MaxNeuronsHold; i++)
        {
            wchar_t ne[100] = {};
            D2D1_RECT_F r1 = full;
            r1.left += 10;
            r1.right -= 10;
            r1.top += i * nh;
            r1.bottom = r1.top + nh;

            if (i >= (MaxNeuronsHold / 2))
            {
                if (fi)
                {
                    D2D1_RECT_F rr1 = full;
                    rr1.left += 10;
                    rr1.right -= 10;
                    rr1.top = (full.bottom - full.top) / 2.0f + full.top;
                    rr1.top -= 5;
                    rr1.bottom = rr1.top + 10;
                    rr1.left = (full.right - full.left) / 2.0f + full.left;
                    rr1.left -= 5;
                    rr1.right = rr1.left + 10;
                    D2D1_ELLIPSE el2 = FromR(rr1);
                    r->FillEllipse(el2, d2d->BlackBrush);

                    rr1.top -= 15;
                    rr1.bottom -= 15;
                    el2 = FromR(rr1);
                    r->FillEllipse(el2, d2d->BlackBrush);
                    rr1.top += 30;
                    rr1.bottom += 30;
                    el2 = FromR(rr1);
                    r->FillEllipse(el2, d2d->BlackBrush);

                    fi = 0;
                }

                r1.bottom = nb;
                r1.top = r1.bottom - nh;
                nb -= nh;
            }

            r1.top += 10;
            r1.bottom -= 10;

            D2D1_ELLIPSE el2 = {};
            el2.point = { (r1.left + r1.right) / 2, (r1.top + r1.bottom) / 2 };
            el2.radiusX = (r1.right - r1.left) / 2;
            el2.radiusY = (r1.bottom - r1.top) / 2;
            r->DrawEllipse(el2, d2d->BlackBrush);


            int nP = i + 1;
            if (i >= (MaxNeuronsHold / 2))
                nP = numneurons - (i - MaxNeuronsHold / 2);

            l->DrawnNeurons[nP - 1] = r1;

            TEXTALIGNPUSH tap(d2d->Text);
            swprintf_s(ne, 100, L"%i", nP);

            r->DrawTextW(ne, (UINT32)wcslen(ne), d2d->Text, r1, d2d->BlackBrush);


        }

    }

    void Network::UpdateVideoMemory()
    {
        CComPtr<IDXGIAdapter3> d3;
        d3 = d2d->dxgiAdapter;
        if (d3)
        {
            d3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &vmi);
            d3->GetDesc1(&this->vdesc);
        }
    }


    void Network::Paint()
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
                d2d->CreateD2X(0, 0, (int)wi, (int)he, 1, 0, 0, 1);
            }

        }
        if (!d2d)
            return;

        if (iVisiblePage != 1)
        {
            Resize();
            return;
        }

        d2d->m_d2dContext->BeginDraw();
        d2d->m_d2dContext->Clear({ 1,1,1,1 });


        float dpi_scale = (float)GetDpiForWindow((HWND)wnd()) / 96;
		if (dpi_scale < 1)
			dpi_scale = 1;  

        if (!d2d->WhiteBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 1,1,1,1 }, &d2d->WhiteBrush);
        if (!d2d->BlackBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0,0,0,1 }, &d2d->BlackBrush);
        if (!d2d->CyanBrush)
            d2d->m_d2dContext->CreateSolidColorBrush({ 0,0,1,1 }, &d2d->CyanBrush);

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
        //        if (rfull.bottom == 0)
        rfull.bottom = (float)scp.Height();


        d2d->m_d2dContext5->DrawRectangle(rfull, d2d->BlackBrush, 1);
        auto r = d2d->m_d2dContext5;


#ifdef _DEBUG
        r->DrawRectangle(rfull, d2d->BlackBrush, 10);
#endif



        if (1)
        {
            wchar_t wt[1000] = {};
            if (vmi.Budget == 0)
            {
                UpdateVideoMemory();
            }

            swprintf_s(wt, L"%s - Video Memory: %I64u MB, Usage: %I64u MB", vdesc.Description ? vdesc.Description : L"Default Adapter", vmi.Budget / (1024 * 1024), vmi.CurrentUsage / (1024 * 1024));
            auto msr = d2d->MeasureString(d2d->WriteFa.get(), d2d->Text2, wt, -1);
            if (rfull.right > std::get<0>(msr))
            {
                D2D1_RECT_F r1 = { 0,0,rfull.right,std::get<1>(msr) + 5 };

                d2d->CyanBrush->SetOpacity(0.2f);
                r->FillRoundedRectangle(D2D1::RoundedRect(r1, 5, 5), d2d->CyanBrush);
                d2d->CyanBrush->SetOpacity(1.0f);
                TEXTALIGNPUSH textalign(d2d->Text2, ScaleX <= 1 ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                r1.left += 5;
                r->DrawText(wt, (UINT32)wcslen(wt), d2d->Text2, r1, d2d->BlackBrush);
            }
        }


        float NextX = 10;

        struct LA
        {
            size_t i;
            int n;
        };
        std::vector<LA> NeuronsPerLayer;
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            auto& l = nn.Layers[i];
            auto numneurons = l.weights.cols();
            if (numneurons == 0)
                numneurons = l.output.cols();
            if (numneurons == 0 && i < (nn.Layers.size() - 1))
                numneurons = nn.Layers[i + 1].weights.rows();
            LA la;
            la.i = i;
            la.n = numneurons;
            NeuronsPerLayer.push_back(la);
        }

        std::sort(NeuronsPerLayer.begin(), NeuronsPerLayer.end(), [](LA& left, LA& right)
            {
                if (left.n > right.n) return true;
                return false;
            });

        float LayerSize = 0;
        if (nn.Layers.size())
            LayerSize = (rfull.right - rfull.left) / nn.Layers.size();
        if (LayerSize > 200)
            LayerSize = 200;

        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            auto& ly = nn.Layers[i];
            D2D1_RECT_F r1 = rfull;
            r1.left += NextX;
            r1.right = r1.left + std::min(100.0f,LayerSize);
            r1.top += 20;

            for (size_t j = 0; j < NeuronsPerLayer.size(); j++)
            {
                if (NeuronsPerLayer[j].i == i)
                    break;
                r1.bottom -= 20;
                r1.top += 20;
            }

            NextX += LayerSize;
            PaintALayer(r1, &ly, i);
        }

        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            auto& prev = nn.Layers[i - 1];
            auto& curr = nn.Layers[i];

            // Lines
            for (auto& p : prev.DrawnNeurons)
            {
                for (auto& c : curr.DrawnNeurons)
                {
                    D2D1_POINT_2F p1 = { p.second.right, (p.second.top + p.second.bottom) / 2 };
                    D2D1_POINT_2F p2 = { c.second.left, (c.second.top + c.second.bottom) / 2 };

                    d2d->m_d2dContext->DrawLine(p1, p2, d2d->BlackBrush, 0.5f);
                }
            }
        }


        [[maybe_unused]] auto hr = d2d->m_d2dContext->EndDraw();
        if (d2d->m_swapChain1)
            hr = d2d->m_swapChain1->Present(1, 0);
    }

    winrt::hstring Network::WeightsText()
    {
        std::vector<wchar_t> s(10000);
        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
            {
                swprintf_s(s.data(), 10000, L"%ix%i", nn.Layers[i].weights.rows(), nn.Layers[i].weights.cols());
                break;
            }
        }
        return s.data();
    }

    winrt::hstring Network::BiasesText()
    {
        std::vector<wchar_t> s(10000);
        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
            {
                swprintf_s(s.data(), 10000, L"%ix%i", nn.Layers[i].biases.rows(), nn.Layers[i].biases.cols());
                break;
            }
        }
        return s.data();
    }

    void Network::AddHiddenAfter(IInspectable, IInspectable)
    {
        /*
            mNIST: input # 784
                     hidden : 128 neurons * 784 weights each
                     output : 10 neurons * 128 weights each
                     */
        for (size_t i = 0; i < nn.Layers.size() - 1; i++)
        {
            if (nn.Layers[i].Sel)
            {
                auto lr = nn.Layers[i].lr;
                if (lr <= 0.000f)
                    lr = 0.01f;
                int num_neurons = nn.Layers[i + 1].weights.cols();
                int num_weights_per_neuron = nn.Layers[i].output.cols();
                if (i == 0)
                    num_weights_per_neuron = 128;
                Layer l(lr, ACT_TYPE::RELU, num_neurons, num_weights_per_neuron);

                nn.Layers.insert(nn.Layers.begin() + i + 1, l);

                nn.RecalculateWB();
                Refresh();
                break;
            }
        }
    }


    double Network::LearningRate()
    {
        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                return nn.Layers[i].lr;

        }
        return 0;
        //        return std::numeric_limits<double>::quiet_NaN();;
    }

    void Network::LearningRate(double v)
    {
        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                nn.Layers[i].lr = v;
        }
        return;
    }


    long Network::IndexOfLayer()
    {
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                return (long)i;
        }
        if (nn.Layers.empty())
            return 0;
        nn.Layers[0].Sel = 1;
        return 0;
    }
    void Network::IndexOfLayer(long idx)
    {
        for (size_t i = 0; i < nn.Layers.size(); i++)
        {
            if (i == idx)
                nn.Layers[i].Sel = 1;
            else
                nn.Layers[i].Sel = 0;
        }
        Refresh({ L"LearningRate",L"LearningRateVisible",L"ActFuncVisible",L"IndexOfAct",L"InputsVisible",L"CountVisible",L"OutputsVisible",L"NumNeurons",L"WeightsText",L"BiasesText" });
    }


    bool Network::LearningRateVisible()
    {
        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                return 1;
        }
        return 0;
    }

    bool Network::ActFuncVisible()
    {
        for (size_t i = 1; i < nn.Layers.size(); i++)
        {
            if (nn.Layers[i].Sel)
                return 1;
        }
        return 0;
    }

    void Network::OnOpen(IInspectable, IInspectable)
    {
        //        f:\wuitools\nn\mnist.nn

        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)0;
        of.lpstrFilter = L"*.nn\0*.nn\0\0";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        of.nMaxFile = 10000;
        of.lpstrDefExt = L"nn";
        of.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (!GetOpenFileName(&of))
            return;
        fil = fnx.data();
        LoadNetworkFile();
        Refresh();
    }

    void Network::OnExportPTH(IInspectable, IInspectable)
    {
        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)0;
        of.lpstrFilter = L"*.pth\0*.pth\0\0";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        of.nMaxFile = 10000;
        of.lpstrDefExt = L"pth";
        of.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&of))
            return;

        NNToPythonPTH(fnx.data());

    }


    void Network::OnDataMNIST(IInspectable, IInspectable)
    {
        /*        OPENFILENAME of = {0};
                of.lStructSize = sizeof(of);
                of.hwndOwner = (HWND)0;
                of.lpstrFilter = L"*.nn\0*.nn\0\0";
                std::vector<wchar_t> fnx(10000);
                of.lpstrFile = fnx.data();
                of.nMaxFile = 10000;
                of.lpstrDefExt = L"nn";
                of.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                if (!GetSaveFileName(&of))
                    return;
                    */

        auto tf = TempFile3();
        tf += L".nn";
        void mnist_test(const wchar_t*);
        mnist_test(tf.c_str());
        fil = tf.c_str();
        LoadNetworkFile();
        Refresh();
    }


    void Network::OnExit(IInspectable, IInspectable)
    {

    }

    void Network::OnExportONNX(IInspectable, IInspectable)
    {
        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)0;
        of.lpstrFilter = L"*.onnx\0*.onnx\0\0";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        of.nMaxFile = 10000;
        of.lpstrDefExt = L"onnx";
        of.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&of))
            return;

        NNToPythonOnnx(fnx.data());
    }

    void Network::OnNew(IInspectable, IInspectable)
    {
        std::vector<wchar_t> a(10000);
        GetModuleFileName(0, a.data(), 10000);
        ShellExecute(0, L"open", a.data(), 0, 0, SW_SHOW);
        return;
    }
    void Network::OnSave(IInspectable a, IInspectable b)
    {
        if (fil.empty())
            OnSaveAs(a, b);
        else
        {
            void SaveProjectToFile(const wchar_t* f);
            SaveProjectToFile(fil.c_str());
        }
    }
    void Network::OnSaveAs(IInspectable a, IInspectable b)
    {
        OPENFILENAME of = { 0 };
        of.lStructSize = sizeof(of);
        of.hwndOwner = (HWND)0;
        of.lpstrFilter = L"*.nn\0*.nn\0\0";
        std::vector<wchar_t> fnx(10000);
        of.lpstrFile = fnx.data();
        of.nMaxFile = 10000;
        of.lpstrDefExt = L"nn";
        of.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!GetSaveFileName(&of))
            return;

        fil = fnx.data();
        OnSave(a, b);
    }

    void Network::Train_Cancel(IInspectable, IInspectable)
    {
        TrainCancel = 1;
    }

    void Network::OnTestAGPU(IInspectable, IInspectable)
    {
        void TestAcc(int);
        TestAcc(1);
    }
    void Network::OnTestACPU(IInspectable, IInspectable)
    {
        void TestAcc(int);
        TestAcc(0);
    }

    void Network::OnTrainGPU(IInspectable, IInspectable)
    {
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.Title(winrt::box_value(L"Training on GPU"));
        ct.ShowAsync();

        UpdateVideoMemory();
        void StartTrain(int m, void*);
        StartTrain(1, this);
        UpdateVideoMemory();

    }
    void Network::OnTrainCPU(IInspectable, IInspectable)
    {
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.Title(winrt::box_value(L"Training on CPU"));
        ct.ShowAsync();

        UpdateVideoMemory();
        void StartTrain(int m, void*);
        StartTrain(0, this);
        UpdateVideoMemory();
    }

    void Network::OnRTrainGPU(IInspectable, IInspectable)
    {
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.Title(winrt::box_value(L"RNN Training on GPU"));
        ct.ShowAsync();

        UpdateVideoMemory();
        void StartTrain(int m, void*);
        StartTrain(3, this);
        UpdateVideoMemory();

    }
    void Network::OnRTrainCPU(IInspectable, IInspectable)
    {
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.Title(winrt::box_value(L"RNN Training on CPU"));
        ct.ShowAsync();

        UpdateVideoMemory();
        void StartTrain(int m, void*);
        StartTrain(2, this);
        UpdateVideoMemory();
    }


    void Network::TrainEnds()
    {
        auto sp = Content().as<Panel>();
        auto ct = sp.FindName(L"Input1").as<ContentDialog>();
        ct.Hide();
        UpdateVideoMemory();
    }

}

// Called in thread
DWORD ptc = 0;
HWND mw();
wchar_t msg[1000] = {};
HRESULT ForCallback(int iEpoch, int iDataset, int step, float totalloss, float acc, void* param)
{
    if (TrainCancel)
        return E_ABORT;

    if (step == 0 || step == 4 || step == 5)
    {
        return S_OK;
    }
    auto ctc = GetTickCount();
    if (ctc < (ptc + 1000))
        return S_OK;

    ptc = ctc;
    if (totalloss == 0)
        swprintf_s(msg, 1000, L"Epoch %i - Set %i.\r\nCurrent accuracy: %.1f", iEpoch + 1, iDataset, acc);
    else
        swprintf_s(msg, 1000, L"Epoch %i - Set %i.\r\nCurrent loss: %.1f\r\nCurrent accuracy: %.1f", iEpoch + 1, iDataset, totalloss, acc);

    PostMessage(mw(), WM_USER + 1, (WPARAM)param, (LPARAM)msg);

    return S_OK;
}

// From main thread
void CallbackUpdate(WPARAM param, LPARAM msg1)
{
    winrt::VisualDML::implementation::Network* n1 = (winrt::VisualDML::implementation::Network*)param;
    n1->Content().as<Panel>().FindName(L"Input2").as<TextBlock>().Text((const wchar_t*)msg1);
}


void TrainingEnds()
{
    if (windows.empty())
        return;
    auto i = windows[0];
    if (i)
    {
        auto mw = i.as<winrt::VisualDML::MainWindow>();
        if (mw)
        {
            auto topnv = mw.Content().as<NavigationView>();
            if (topnv)
            {
                Frame fr = topnv.FindName(L"contentFrame").as<Frame>();
                fr.Content().as<winrt::VisualDML::Network>().TrainEnds();
            }
        }
    }


    //    winrt::VisualDML::implementation::Network* n1 = (winrt::VisualDML::implementation::Network*)param;
    //	n1->Content().as<Panel>().FindName(L"Input1").as<ContentDialog>().Hide();

}
