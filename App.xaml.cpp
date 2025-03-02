#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"



#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"dwmapi.lib")
#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"comdlg32.lib")


using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;



HICON hIcon1 = 0;
WNDPROC wProc = 0;
std::wstring fil;

std::map<HWND, winrt::Windows::Foundation::IInspectable> windows;


LRESULT CALLBACK cbx(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
{

    if (mm == WM_KEYDOWN)
    {
        MessageBeep(0);
    }
    if (mm == WM_SIZE)
    {
        auto i = windows[hh];
        if (i)
        {
            auto mw = i.as<winrt::DirectMLGraph::MainWindow>();
            if (mw)
                mw.Resize();
        }
        return 0;

    }
    return CallWindowProc(wProc, hh, mm, ww, ll);
}

winrt::DirectMLGraph::MainWindow CreateWi()
{
    winrt::DirectMLGraph::MainWindow j;
    j.Activate();
    static int One = 0;

    auto n = j.as<::IWindowNative>();
    if (n)
    {
        HWND hh;
        n->get_WindowHandle(&hh);
        if (hh)
        {
			j.wnd((int64_t)hh);
			windows[hh] = j;    
            hIcon1 = LoadIcon(GetModuleHandle(0), L"ICON_1");

            wProc = (WNDPROC)GetWindowLongPtr(hh, GWLP_WNDPROC);
            SetWindowLongPtr(hh, GWLP_WNDPROC, (LONG_PTR)cbx);


            SetWindowText(hh, L"DirectML Graph");
            if (One == 0)
                ShowWindow(hh, SW_SHOWMAXIMIZED);
            One = 1;
#define GCL_HICONSM         (-34)
#define GCL_HICON           (-14)
            SetClassLongPtr(hh, GCL_HICONSM, (LONG_PTR)hIcon1);
            SetClassLongPtr(hh, GCL_HICON, (LONG_PTR)hIcon1);

            if (1)
            {
                BOOL value = true;
                ::DwmSetWindowAttribute(hh, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
            }
        }
    }
    return j;
}




// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::DirectMLGraph::implementation
{
    /// <summary>
    /// Initializes the singleton application object.  This is the first line of authored code
    /// executed, and as such is the logical equivalent of main() or WinMain().
    /// </summary>
    App::App()
    {
        // Xaml objects should not call InitializeComponent during construction.
        // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    /// <summary>
    /// Invoked when the application is launched.
    /// </summary>
    /// <param name="e">Details about the launch request and process.</param>
    void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
    {
        window = CreateWi();

    }
}



int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR t, int)
{
    //    MessageBox(0, 0, 0, 0);
    if (t)
    {
        fil = t;
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    {
        void (WINAPI * pfnXamlCheckProcessRequirements)();
        auto module = ::LoadLibrary(L"Microsoft.ui.xaml.dll");
        if (module)
        {
            pfnXamlCheckProcessRequirements = reinterpret_cast<decltype(pfnXamlCheckProcessRequirements)>(GetProcAddress(module, "XamlCheckProcessRequirements"));
            if (pfnXamlCheckProcessRequirements)
            {
                (*pfnXamlCheckProcessRequirements)();
            }

            ::FreeLibrary(module);
        }
    }

/*    PWSTR p = 0;
    SHGetKnownFolderPath(FOLDERID_ProgramData, 0, 0, &p);
    std::wstring de = p;
    CoTaskMemFree(p);

    de += L"\\B7D701B9-F0C7-4771-B8ED-3F53453C1AB8";
    SHCreateDirectory(0, de.c_str());
    datafolder = de.c_str();
    std::wstring sf = de + L"\\settings.xml";
    settings = std::make_shared<XML3::XML>(sf.c_str());
    */

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    ::winrt::Microsoft::UI::Xaml::Application::Start(
        [](auto&&)
        {
            ::winrt::make<::winrt::DirectMLGraph::implementation::App>();
        });

//    settings->Save();
    return 0;
}

