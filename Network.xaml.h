#pragma once

#include "Network.g.h"
#include "Item.h"

namespace winrt::VisualDML::implementation
{
    struct Network : NetworkT<Network>
    {
        Network()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }


        long long _wnd = 0;
        long long wnd()
        {
            HWND mw();
            if (!_wnd)
                _wnd = (long long)mw();
            return _wnd;
        }
        void wnd(long long value)
        {
            _wnd = value;
        }


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
        static winrt::hstring txt(long jx)
        {
            const wchar_t* s(size_t);
            return s(jx);
        }


        void TrainEnds();
        bool InputsVisible();
        bool CountVisible();
        bool OutputsVisible();
        double NumNeurons();
        void NumNeurons(double);

        bool LearningRateVisible();
        bool ActFuncVisible();
        double LearningRate();
        void LearningRate(double);
        double BatchNumber();
        void BatchNumber(double);
        double NumEpochs();
        void NumEpochs(double);
        long  IndexOfLayer();
        void IndexOfLayer(long);
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::VisualDML::Item> LayerList();
        long  IndexOfAct();
        void IndexOfAct(long);
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::VisualDML::Item> ActList();

        winrt::hstring WeightsText();
        winrt::hstring BiasesText();

        void Paint();
        void Resize();
        void LoadAdapters();

        DXGI_QUERY_VIDEO_MEMORY_INFO vmi = {};
        DXGI_ADAPTER_DESC1 vdesc = {};
        void UpdateVideoMemory();

        void PaintALayer(D2D1_RECT_F full, void* ly, size_t lidx);

        // Menu
        void OnLoaded(IInspectable, IInspectable);
        void AddHiddenAfter(IInspectable, IInspectable);
        void OnOpen(IInspectable, IInspectable);
        void OnDataMNIST(IInspectable, IInspectable);
        void OnExit(IInspectable, IInspectable);
        void OnNew(IInspectable, IInspectable);
        void OnSave(IInspectable, IInspectable);
        void OnSaveAs(IInspectable, IInspectable);
        void OnTrainCPU(IInspectable, IInspectable);
        void OnTrainGPU(IInspectable, IInspectable);
        void OnTestAGPU(IInspectable, IInspectable);
        void OnTestACPU(IInspectable, IInspectable);
        void OnRTrainCPU(IInspectable, IInspectable);
        void OnRTrainGPU(IInspectable, IInspectable);
        void Train_Cancel(IInspectable, IInspectable);
        void OnExportONNX(IInspectable, IInspectable);
        void OnExportPTH(IInspectable, IInspectable);


    };
}

namespace winrt::VisualDML::factory_implementation
{
    struct Network : NetworkT<Network, implementation::Network>
    {
    };
}
