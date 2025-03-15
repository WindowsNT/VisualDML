#pragma once

#include "MainWindow.g.h"

namespace winrt::VisualDML::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void Refresh();
        void Resize();
        void Finished();
        void OnLoad(IInspectable, IInspectable);
        void ItemInvoked(IInspectable, winrt::Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs);


        long long _wnd = 0;
		long long wnd()
		{
			return _wnd;
		}
        void wnd(long long value)
        {
            _wnd = value;
        }

    };
}

namespace winrt::VisualDML::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
