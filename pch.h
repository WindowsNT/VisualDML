#define DISABLE_XAML_GENERATED_MAIN
#define DML_TARGET_VERSION_USE_LATEST
#pragma once
#include <windows.h>
#include <unknwn.h>
#include <wininet.h>
#include <restrictederrorinfo.h>
#include <hstring.h>
#include <queue>
#include <any>
#include <optional>
#include <stack>
#include <mutex>
#include <vector>
#include <set>
#include <functional>
#include <memory>
#include <limits>
#include <vector>
#include <shlobj.h>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <random>
#include <dwmapi.h>

#include <wincodec.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d2d1_2.h>
#include <d2d1_3.h>
#include <dwrite.h>
#include <d3d11.h>
#include <d3d12.h>
#include <dxgi.h>
#include <optional>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <atlbase.h>
#include "d3dx12.h"
#include ".\\packages\\Microsoft.AI.DirectML.1.15.4\\include\\DirectML.h"
#include "DirectMLX.h"

#include "ystring.h"

#include <random>
#undef min
#undef max


// Undefine GetCurrentTime macro to prevent
// conflict with Storyboard::GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Microsoft.UI.Input.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.UI.Xaml.Interop.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <microsoft.ui.xaml.media.dxinterop.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.Windows.AppNotifications.h>
#include <winrt/Microsoft.Windows.AppNotifications.Builder.h>
#include <winrt/Microsoft.UI.Xaml.Navigation.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <wil/cppwinrt_helpers.h>
#include <appmodel.h>


struct D2F : public D2D1_RECT_F
{
	float Width()
	{
		return right - left;
	}
	float Height()
	{
		return bottom - top;
	}
	float MiddleX()
	{
		return left + (right - left) / 2.0f;
	}
	float MiddleY()
	{
		return top + (bottom - top) / 2.0f;
	}

	void Shrink(float x)
	{
		left += x;
		top += x;
		right -= x;
		bottom -= x;
	}

	D2F(const D2F& d)
	{
		*this = d;
	}

	void reset()
	{
		left = 0; right = 0; top = 0; bottom = 0;
	}

	D2F(float l = 0, float t = 0, float r = 0, float b = 0)
	{
		left = l; right = r; top = t; bottom = b;
	}
	D2F(const D2D1_RECT_F& r)
	{
		operator=(r);
	}

	D2F& operator=(const D2D1_RECT_F& r)
	{
		left = r.left;
		top = r.top;
		right = r.right;
		bottom = r.bottom;
		return *this;
	}
	operator D2D1_RECT_F()
	{
		return *this;
	}
};



#include "xml3all.h"
#include "d2d.hpp"
#include "dmllib.hpp"


class FMAP
{
private:

	HANDLE hX = INVALID_HANDLE_VALUE;
	HANDLE hY = INVALID_HANDLE_VALUE;
	unsigned long long sz = 0;
	char* d = 0;
	std::wstring mappedfile;

public:

	std::wstring fil(bool WithFolder) const
	{
		if (WithFolder)
			return mappedfile;
		std::vector<wchar_t> t(1000);
		wcscpy_s(t.data(), 1000, mappedfile.c_str());
		PathStripPath(t.data());
		return t.data();
	}
	void Stop(unsigned long long p)
	{
		LARGE_INTEGER li = { 0 };
		GetFileSizeEx(hX, &li);
		if (p > (unsigned long long)li.QuadPart)
			p = li.QuadPart;
		sz = p;

	}

	HANDLE& hF() { return hX; }
	unsigned long long size() { return sz; }
	char* p() { return d; }

	bool CreateForRecord(const wchar_t* f)
	{
		mappedfile = f;
		hX = CreateFile(f, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
		if (hX == INVALID_HANDLE_VALUE || hX == 0)
		{
			hX = 0;
			return false;
		}
		return true;
	}

	bool Map(const wchar_t* f)
	{
		hX = CreateFile(f, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		if (hX == INVALID_HANDLE_VALUE || hX == 0)
		{
			hX = 0;
			return false;
		}
		LARGE_INTEGER li = { 0 };
		GetFileSizeEx(hX, &li);
		sz = li.QuadPart;
		hY = CreateFileMapping(hX, 0, PAGE_READONLY, 0, 0, 0);
		if (hY == 0)
			return false;
		d = (char*)MapViewOfFile(hY, FILE_MAP_READ, 0, 0, 0);
		mappedfile = f;
		return true;
	}

	void Unmap()
	{
		if (d)
			UnmapViewOfFile(d);
		d = 0;
		if (hY != 0)
			CloseHandle(hY);
		hY = 0;
		if (hX != 0)
			CloseHandle(hX);
		hX = 0;
		hY = 0;
	}


	FMAP(const wchar_t* f = 0)
	{
		if (f)
			Map(f);
	}
	~FMAP()
	{
		Unmap();
	}

};


template <typename T>
bool PutFile(const wchar_t* f, std::vector<T>& d, bool Fw = true)
{
	HANDLE hX = CreateFile(f, GENERIC_WRITE, 0, 0, Fw ? CREATE_ALWAYS : CREATE_NEW, 0, 0);
	if (hX == INVALID_HANDLE_VALUE)
		return false;
	DWORD A = 0;
	WriteFile(hX, d.data(), (DWORD)(d.size() * sizeof(T)), &A, 0);
	CloseHandle(hX);
	if (A != d.size())
		return false;
	return true;
}

inline std::shared_ptr<XML3::XML> Settings;
inline const wchar_t* ttitle = L"DirectML Graph Editor";

template <typename T = unsigned int>
std::vector<T> TensorFromString(const wchar_t* str)
{
	std::vector<T> newSizes;
	std::vector<std::wstring> split(const std::wstring & s, wchar_t delim);
	auto sp = split(str, L'x');
	for (auto& s : sp)
		newSizes.push_back((T)std::stoi(s));
	return newSizes;
}
