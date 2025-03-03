#define DISABLE_XAML_GENERATED_MAIN
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