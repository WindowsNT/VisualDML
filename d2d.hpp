//

inline DXGI_FORMAT uu = DXGI_FORMAT_B8G8R8A8_UNORM;
//DXGI_FORMAT uu = DXGI_FORMAT_R16G16B16A16_FLOAT;
inline D2D1_ALPHA_MODE alphamode = D2D1_ALPHA_MODE_PREMULTIPLIED;
inline int MainHDR = 0;
inline float MaxHDRLuminance = 0;

struct D2D;



class TEXTALIGNPUSH
{

    IDWriteTextFormat* Text = 0;
    DWRITE_TEXT_ALIGNMENT c1;
    DWRITE_PARAGRAPH_ALIGNMENT c2;
public:
    TEXTALIGNPUSH(IDWriteTextFormat* t, DWRITE_TEXT_ALIGNMENT nc1 = DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT nc2 = DWRITE_PARAGRAPH_ALIGNMENT_CENTER)
    {
        if (!t)
            return;
        Text = t;
        c1 = Text->GetTextAlignment();
        c2 = Text->GetParagraphAlignment();
        s(nc1, nc2);
    }

    void s(DWRITE_TEXT_ALIGNMENT nc1, DWRITE_PARAGRAPH_ALIGNMENT nc2)
    {
        if (Text)
        {
            Text->SetTextAlignment(nc1);
            Text->SetParagraphAlignment(nc2);
        }
    }
    ~TEXTALIGNPUSH()
    {
        if (!Text)
            return;
        Text->SetTextAlignment(c1);
        Text->SetParagraphAlignment(c2);
    }
};


struct D2D
{

	void FailDraw()
	{
		OutputDebugString(L"FAIL DRAW\r\n");
	}

	void dd(bool jpg);

	operator bool()
	{
		if (rt())
			return true;
		return false;
	}


	bool OnceSwapChain = 0; // For WUI

	SIZEL SizeCreated = { 0 };
	//	CComPtr<IDWriteFactory> WriteFactory;

		// Windows 7
	CComPtr<ID2D1HwndRenderTarget> d;

	CComPtr<ID2D1Factory> fa;
	CComPtr<ID2D1Factory1> fa1;
	CComPtr<ID2D1Factory5> fa5;

	// Windows 8
	CComPtr<ID3D11Device> device;
	//	CComPtr<ID3D11Device1> device1;
	CComPtr<ID3D11DeviceContext> context;
	//	CComPtr<ID3D11DeviceContext1> context1;
	CComPtr<IDXGIDevice1> dxgiDevice;
	CComPtr<ID2D1Device> m_d2dDevice;
	CComPtr<ID2D1Device5> m_d2dDevice5;
	CComPtr<ID2D1DeviceContext> m_d2dContext;
	CComPtr<ID2D1DeviceContext5> m_d2dContext5;
	CComPtr<ID2D1DeviceContext6> m_d2dContext6;
	CComPtr<ID2D1DeviceContext7> m_d2dContext7;
	CComPtr<IDXGIAdapter> dxgiAdapter;
	CComPtr<IDXGIFactory2> dxgiFactory;
	CComPtr<IDXGISwapChain1> m_swapChain1;
	int SwapChainForWUI3 = 0;
	std::vector<RECT> SwapChain1Rects;
	CComPtr<ID3D11Texture2D> backBuffer;
	CComPtr<IDXGISurface> dxgiBackBuffer;
	CComPtr<ID2D1Bitmap1> m_d2dTargetBitmap;

	CComPtr<ID2D1Bitmap1> m_TempBitmapForCopying;
	CComPtr<IWICBitmap> CopyToBitmap(CComPtr<IWICBitmap> UseThis);
	CComPtr<IWICImagingFactory> wicfac = 0;

	CComPtr<IWICBitmap> bmp = 0;
	CComPtr<ID2D1RenderTarget> bitmaprender;
	DXGI_OUTPUT_DESC1 desc1 = {}; // hdr

	// Brushes for mixer
	CComPtr<ID2D1SolidColorBrush> SeparatorBrush;
	CComPtr<ID2D1SolidColorBrush> SnapBrush2;
	CComPtr<ID2D1SolidColorBrush> WhiteBrush;
	CComPtr<ID2D1SolidColorBrush> YellowBrush;
	CComPtr<ID2D1SolidColorBrush> RedBrush;
    CComPtr<ID2D1SolidColorBrush> CyanBrush;
    CComPtr<ID2D1SolidColorBrush> GreenBrush;
	CComPtr<ID2D1SolidColorBrush> BlackBrush;
	CComPtr<ID2D1SolidColorBrush> BGBrush;
    CComPtr<IDWriteTextFormat> Text;
    CComPtr<IDWriteTextFormat> Text2;
	CComPtr<IDWriteTextFormat> Text3;
	CComPtr<ID2D1SolidColorBrush> GetD2SolidBrush(ID2D1RenderTarget* p, D2D1_COLOR_F cc)
	{
		CComPtr<ID2D1SolidColorBrush> b = 0;
		p->CreateSolidColorBrush(cc, &b);
		return b;
	}
	void Off(bool KeepDevice = false);
	HRESULT OffOnSize(int wi, int he, bool Quick = 0, bool KeepDevice = false);


	// Multithread-manager stuff
	UINT mmanager_reset = 0;
	HRESULT CreateMultithreadManager();

	CComPtr<ID2D1RenderTarget> rt()
	{
		CComPtr<ID2D1RenderTarget> rr;
		if (m_d2dContext5)
			rr = m_d2dContext5;
		else
			if (m_d2dContext)
				rr = m_d2dContext;
			else
				rr = d;
		return rr;
	}

	GUID PixelFormatForBitmap(int ctx = 0);
	WICBitmapInterpolationMode ScalingModeForBitmap(int ctx = 0);
	int HDRSupported = 0;
	int HDRMode = 0;
	int WithoutEffectsRegistration = 0;
	DXGI_FORMAT pform = DXGI_FORMAT_B8G8R8A8_UNORM; // Guess that DXGI_FORMAT_R16G16B16A16_FLOAT also supported, continue trying here.

	// Direct3D stuff if needed
	CComPtr<ID3D11RenderTargetView> pRenderTargetView;
	CComPtr<ID3D11DepthStencilView> pDepthStencilView;

	std::vector<CComPtr<IUnknown>> ObjectsForDrawingSelection; // See render2.cpp

	bool CreateRenderingBitmap(RECT rc);
	bool CreateIf(HWND hh, RECT rc, bool w7 = false, int SwapChainForWUI3 = 0);
	bool CreateWin7(HWND hh, RECT rc);
	HRESULT Resize(int wi, int he);
	HRESULT Resize2(int wi, int he);

	bool CreateD2(IDXGIAdapter* wad, HWND hh, D3D_DRIVER_TYPE de = D3D_DRIVER_TYPE_HARDWARE, int wi = 0, int he = 0, bool IncreaseTiles = 0, int forwhat = 0, int hdr_if = 0, int SwapChainForWUI3 = 0);
	bool CreateD2X(IDXGIAdapter* wad, HWND hh, int wi, int he, bool IncreaseTiles, int forwhat, int hdr_if = 0, int SwapChainForWUI3 = 0);
	int OnResize(HWND hh, bool RR, int wi = 0, int he = 0);

	std::shared_ptr<IDWriteFactory> WriteFa;

	void CreateWriteFa();
	std::tuple<float, float, DWRITE_OVERHANG_METRICS> MeasureString(IDWriteFactory* pWriteFactory, IDWriteTextFormat* ffo, const wchar_t* txt, int l = -1);

		HWND m_hwnd = 0;


};




inline std::shared_ptr<D2D> d2d;
inline int iVisiblePage = -1;


