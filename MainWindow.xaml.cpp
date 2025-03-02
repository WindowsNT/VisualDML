#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif


#include "MLGraph.xaml.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::DirectMLGraph::implementation
{

	void MainWindow::ItemInvoked(IInspectable, NavigationViewItemInvokedEventArgs ar)
	{
		auto topnv = Content().as<NavigationView>();
		if (!topnv)
			return;
		Frame fr = topnv.FindName(L"contentFrame").as<Frame>();
		if (!fr)
			return;
		if (ar.IsSettingsInvoked())
		{
//			fr.Navigate(winrt::xaml_typename<winrt::NN::Settings>());
			return;
		}
		auto it = ar.InvokedItemContainer().as<NavigationViewItem>();
		if (!it)
			return;
		auto n = it.Name();
//		if (n == L"ViewGraph")
//			fr.Navigate(winrt::xaml_typename<winrt::NN::Network>());
		/*		if (n == L"ViewAudio")
			fr.Navigate(winrt::xaml_typename<winrt::tsed::Audio>());
		if (n == L"ViewLinks")
			fr.Navigate(winrt::xaml_typename<winrt::tsed::Links>());
			*/
	}

	void MainWindow::OnLoad(IInspectable, IInspectable)
	{
		auto topnv = Content().as<NavigationView>();
		if (topnv)
		{
			Frame fr = topnv.FindName(L"contentFrame").as<Frame>();
			if (fr)
			{
				fr.Navigate(winrt::xaml_typename<winrt::DirectMLGraph::MLGraph>());
				fr.Content().as<winrt::DirectMLGraph::MLGraph>().wnd((wnd()));
			}

			topnv.KeyDown([this](IInspectable const& , Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& a)
				{
						auto topnv = Content().as<NavigationView>();
						if (topnv)
						{
							Frame fr = topnv.FindName(L"contentFrame").as<Frame>();
							if (fr)
							{
								auto co = fr.Content().try_as<winrt::DirectMLGraph::MLGraph>();
								if (co)
								{
									co.Key((long long)a.Key());
								}
							}
						}
				});


		}
	}


	void MainWindow::Resize()
	{
		auto nv = Content().as<NavigationView>();
		if (nv)
		{
			auto fr = nv.Content().as<Frame>();
			if (fr)
			{
				auto n = fr.Content().as<Page>();
				if (n)
				{
					auto s = n.as<winrt::DirectMLGraph::MLGraph>();
					if (s)
					{
						s.Resize();
					}
				}
			}
		}
	}


	void MainWindow::Refresh()
	{
		auto topnv = Content().as<NavigationView>();
		if (topnv)
		{
			Frame fr = topnv.FindName(L"contentFrame").as<Frame>();
			if (fr)
			{
				auto co = fr.Content();
				//				auto sc = co.try_as<winrt::tsed::Score>();
				//				if (sc)
				//					sc.Paint();
			}
		}
	}

}
