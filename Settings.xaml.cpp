#include "pch.h"
#include "Settings.xaml.h"
#if __has_include("Settings.g.cpp")
#include "Settings.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;


std::wstring PythonFolder();
bool BrowseFolder(HWND hh, const TCHAR* tit, const TCHAR* root, const TCHAR* sel, TCHAR* rv);

namespace winrt::VisualDML::implementation
{
	void Settings::Refresh()
	{
		m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"" });
	}
	winrt::hstring Settings::PythonLocation()
	{
		return PythonFolder().c_str();
	}
	void Settings::PythonLocation(winrt::hstring const& value)
	{
		SettingsX->GetRootElement().vv("pyf").SetWideValue(value.c_str());
		SettingsX->Save();
		Refresh();
	}

	void Settings::OnPythonLocation(IInspectable const&, IInspectable const&)
	{
		std::vector<wchar_t> rv(10000);
		auto e = PythonFolder();
		SHCreateDirectory(0, e.c_str());
		auto b = BrowseFolder(0, 0, 0, e.c_str(), rv.data());
		if (!b)
			return;
		PythonLocation(rv.data());
	}
}
