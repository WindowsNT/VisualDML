#include <pch.h>

void D2D::CreateWriteFa()
{
    if (!WriteFa)
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&WriteFa);
    if (!WriteFa)
        return;
    return;
}


std::tuple<float, float, DWRITE_OVERHANG_METRICS> D2D::MeasureString(IDWriteFactory* pWriteFactory, IDWriteTextFormat* ffo, const wchar_t* txt, int l)
{
    CComPtr<IDWriteTextLayout> lay = 0;
    pWriteFactory->CreateTextLayout(txt, l == -1 ? (UINT32)wcslen(txt) : l, ffo, 10000, 10000, &lay);
    if (!lay)
        return std::tuple<float, float, DWRITE_OVERHANG_METRICS>(0, 0, {});
    DWRITE_TEXT_METRICS m = { 0 };
    DWRITE_OVERHANG_METRICS om = { 0 };
    lay->GetMetrics(&m);
    lay->GetOverhangMetrics(&om);
    float wi = (float)m.widthIncludingTrailingWhitespace;
    if (m.widthIncludingTrailingWhitespace > (float)wi)
        wi++;
    float he = (float)m.height;
    if (m.height > (float)he)
        he++;
    return std::tuple<float, float, DWRITE_OVERHANG_METRICS>(wi, he, om);
}



bool D2D::CreateD2X(IDXGIAdapter* wad, HWND hh, int wi, int he, bool IncreaseTiles, int forwhat, int hdr_if, int SwapChainForWUI3x)
{
    CreateD2(wad, hh, D3D_DRIVER_TYPE_HARDWARE, wi, he, IncreaseTiles, forwhat, hdr_if, SwapChainForWUI3x);
    if (!m_d2dContext)
        CreateD2(wad, hh, D3D_DRIVER_TYPE_WARP, wi, he, IncreaseTiles, forwhat, hdr_if, SwapChainForWUI3x);
    if (!m_d2dContext)
        return false;
    return true;
}




void D2D::Off(bool)
{
    SizeCreated.cx = 0;
    SizeCreated.cy = 0;
    fa1 = 0;
    fa5 = 0;
    OnceSwapChain = 0;
    m_TempBitmapForCopying = 0;
    //		fa = 0;
    //		col = 0;
    d = 0;
    device = 0;
    context = 0;
    //	context1 = 0;
    desc1 = {};
    dxgiDevice = 0;
    m_d2dDevice = 0;
    m_d2dDevice5 = 0;
    m_d2dContext = 0;
    m_d2dContext5 = 0;
    m_d2dContext6 = 0;
    m_d2dContext7 = 0;
    dxgiAdapter = 0;
    dxgiFactory = 0;
    m_swapChain1 = 0;
    backBuffer = 0; dxgiBackBuffer = 0;
}

bool D2D::CreateD2(IDXGIAdapter* wad, HWND hh, D3D_DRIVER_TYPE de, int wi, int he, bool IncreaseTiles, [[maybe_unused]] int forwhat, int  hdr_if, int SwapChainForWUI3x)
{
    this->SwapChainForWUI3 = SwapChainForWUI3x;
    if (wad)
        de = D3D_DRIVER_TYPE_UNKNOWN;

    try
    {
        CreateWriteFa();
        if (!WriteFa)
            return false;


        D2D& dd = *this;
        if (dd.m_d2dContext)
            return true;
        SizeCreated.cx = wi;
        SizeCreated.cy = he;
        D3D_FEATURE_LEVEL featureLevels[] =
        {
//                        D3D_FEATURE_LEVEL_12_2,
 //                       D3D_FEATURE_LEVEL_12_1,
  //                      D3D_FEATURE_LEVEL_12_0,
                        D3D_FEATURE_LEVEL_11_1,
                        D3D_FEATURE_LEVEL_11_0,
                        D3D_FEATURE_LEVEL_10_1,
                        D3D_FEATURE_LEVEL_10_0,
                        D3D_FEATURE_LEVEL_9_3,
                        D3D_FEATURE_LEVEL_9_2,
                        D3D_FEATURE_LEVEL_9_1
        };
        D3D_FEATURE_LEVEL m_featureLevel;

        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        //        flags |= D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

#ifdef DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        HRESULT hr = S_OK;
        if (!dd.context || !dd.device)
        {
            dd.context = 0;
            dd.device = 0;
            hr = D3D11CreateDevice(
                wad,
                de,
                0,
                flags,              // optionally set debug and Direct2D compatibility flags
                featureLevels,              // list of feature levels this app can support
                ARRAYSIZE(featureLevels),   // number of possible feature levels
                D3D11_SDK_VERSION,
                &dd.device,                // returns the Direct3D device created
                &m_featureLevel,            // returns feature level of device created
                &dd.context                    // returns the device immediate context
            );
        }


#ifdef _DEBUG
        if (0 && device)
        {
            for (int i = 0; i < 132; i++)
            {
                UINT psup = 0;
                auto hrs = device->CheckFormatSupport((DXGI_FORMAT)i, &psup);
                if (SUCCEEDED(hrs))
                {
                }
            }
        }
#endif

        if (!dd.device || FAILED(hr) || !dd.context)
            return 0;

        //	dd.context1 = dd.context;
        dd.dxgiDevice = dd.device;
        if (!dd.dxgiDevice)
            return 0;

        //		dd.device1 = dd.device;

        //    fa5

        D2D1_CREATION_PROPERTIES dp;
        dp.threadingMode = D2D1_THREADING_MODE::D2D1_THREADING_MODE_MULTI_THREADED;
        dp.debugLevel = D2D1_DEBUG_LEVEL::D2D1_DEBUG_LEVEL_NONE;
#ifdef _DEBUG
        //        dp.debugLevel = D2D1_DEBUG_LEVEL::D2D1_DEBUG_LEVEL_INFORMATION;
#endif
        dp.options = D2D1_DEVICE_CONTEXT_OPTIONS::D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS;
        dd.m_d2dDevice = 0;
        D2D1CreateDevice(dd.dxgiDevice, dp, &dd.m_d2dDevice);
        if (!dd.m_d2dDevice)
            return 0;

        // Check also device 5
        m_d2dDevice5 = m_d2dDevice;



        dd.m_d2dDevice->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            &dd.m_d2dContext);
        if (!dd.m_d2dContext)
            return 0;

        if (WithoutEffectsRegistration != 67)
        {
            //            HRESULT RegisterCustomDirect2DEffects(ID2D1DeviceContext * c);
             //           RegisterCustomDirect2DEffects(dd.m_d2dContext);
        }


        m_d2dContext5 = m_d2dContext;
        m_d2dContext6 = m_d2dContext;
        m_d2dContext7 = m_d2dContext;

        fa = 0;
        fa1 = 0;
        fa5 = 0;
        if (m_d2dContext)
            m_d2dContext->GetFactory(&fa);
        fa1 = fa;
        fa5 = fa;


        if (!dd.dxgiDevice)
            return 0;
        dd.dxgiDevice->GetAdapter(&dd.dxgiAdapter);
        if (!dd.dxgiAdapter)
            return 0;

        // Test HDR
        try
        {
            for (int iidx = 0; ; iidx++)
            {
                CComPtr<IDXGIOutput> out1;
                dxgiAdapter->EnumOutputs(iidx, &out1);
                if (!out1)
                    break;
                if (out1)
                {
                    CComPtr<IDXGIOutput6> out6;
                    out6 = out1;
                    if (out6)
                    {
                        out6->GetDesc1(&desc1);
                        desc1.Rotation = DXGI_MODE_ROTATION_IDENTITY;
                        if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                        {
                            HDRSupported = 1;
                            HDRMode = 0;
                            if (hdr_if == 1)
                            {
                                /*                            UINT sup = 0;
                                                            device->CheckFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, &sup);
                                                            device->CheckFormatSupport(DXGI_FORMAT_R32G32B32A32_FLOAT,&sup);
                                                            pform = DXGI_FORMAT_R16G16B16A16_FLOAT;
                                                            pform = DXGI_FORMAT_R32G32B32A32_FLOAT;*/
                                pform = DXGI_FORMAT_R16G16B16A16_FLOAT;
                                MaxHDRLuminance = desc1.MaxLuminance;
                                HDRMode = 1;
                            }
                        }
                    }
                }
            }
        }
        catch (...)
        {
        }


        // Get the factory object that created the DXGI device.
        dd.dxgiAdapter->GetParent(IID_PPV_ARGS(&dd.dxgiFactory));
        if (!dd.dxgiFactory)
            return 0;

        if (IncreaseTiles)
        {
            // Make sure RenderingControls tile is resized
            D2D1_RENDERING_CONTROLS r4c = {};
            m_d2dContext->GetRenderingControls(&r4c);
            if (r4c.tileSize.width < (UINT32)wi || r4c.tileSize.height < (UINT32)he)
            {
                r4c.tileSize.width = wi;
                r4c.tileSize.height = he;
                m_d2dContext->SetRenderingControls(r4c);
                m_d2dContext->GetRenderingControls(&r4c);
            }
        }

#ifdef _DEBUG
        /*
         if (1)
         {
             D3D11_BLEND_DESC blendStateDesc{};
             blendStateDesc.AlphaToCoverageEnable = FALSE;
             blendStateDesc.IndependentBlendEnable = FALSE;
             blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
             blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
             blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
             blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
             blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
             blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_DEST_ALPHA;
             blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
             blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

             CComPtr<ID3D11BlendState> blendState;
             device->CreateBlendState(&blendStateDesc, &blendState);
             if (blendState)
                 context->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
         }*/
#endif

        if (m_d2dContext)
        {
            m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
        }

        if (!hh && !SwapChainForWUI3)
        {
            D2D1_SIZE_U su;
            su.width = wi;
            su.height = he;
            D2D1_BITMAP_PROPERTIES1 p1 = D2D1_BITMAP_PROPERTIES1();
            p1.colorContext = 0;
            p1.pixelFormat.format = pform;
            p1.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
            p1.dpiX = 96;
            p1.dpiY = 96;
            p1.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
            //			p1.bitmapOptions |= D2D1_BITMAP_OPTIONS_CPU_READ;
            hr = dd.m_d2dContext->CreateBitmap(su, 0, 0, &p1, &m_d2dTargetBitmap);
            if (!m_d2dTargetBitmap)
            {
                p1.pixelFormat.format = pform;
                p1.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
                hr = dd.m_d2dContext->CreateBitmap(su, 0, 0, &p1, &m_d2dTargetBitmap);

            }
            dd.m_d2dContext->SetTarget(dd.m_d2dTargetBitmap);

            return true;
        }

        m_hwnd = hh;
        // Allocate a descriptor.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
        swapChainDesc.Width = 0;                           // use automatic sizing
        swapChainDesc.Height = 0;
        swapChainDesc.Format = pform; // this is the most common swapchain format
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        //FixSwapChainOnWUI3(this, SwapChainForWUI3, swapChainDesc);
        //if (Windows7Only)
        //    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.Width = wi;
        swapChainDesc.Height = he;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
        swapChainDesc.Flags = 0;

        if (SwapChainForWUI3)
            hr = dd.dxgiFactory->CreateSwapChainForComposition(
                dd.device,
                &swapChainDesc,
                nullptr,    // allow on all displays
                &dd.m_swapChain1);
        else
            hr = dd.dxgiFactory->CreateSwapChainForHwnd(
                dd.device,
                hh, &swapChainDesc,
                nullptr,    // allow on all displays
                nullptr,    // allow on all displays
                &dd.m_swapChain1);

        if (!dd.m_swapChain1)
            return 0;


        dd.dxgiDevice->SetMaximumFrameLatency(1);


        if (dd.m_swapChain1)
        {
            CComPtr<IDXGIOutput> outp;
            dd.m_swapChain1->GetContainingOutput(&outp);
            if (outp)
            {
                CComPtr<IDXGIOutput6> outp6;
                outp6 = outp;
                if (outp6)
                    outp6->GetDesc1(&desc1);
            }
        }

        if (wi && he)
        {
            SizeCreated.cx = wi;
            SizeCreated.cy = he;
        }

        hr = dd.m_swapChain1->GetBuffer(0, IID_PPV_ARGS(&dd.backBuffer));
        if (!dd.backBuffer)
            return 0;

        // Now we set up the Direct2D render target bitmap linked to the swapchain. 
        D2D1_BITMAP_PROPERTIES1 bitmapProperties;
        bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmapProperties.pixelFormat.format = pform;
        bitmapProperties.dpiX = 96;
        bitmapProperties.dpiY = 96;
        bitmapProperties.colorContext = 0;


        // Direct2D needs the dxgi version of the backbuffer surface pointer.
        dd.m_swapChain1->GetBuffer(0, IID_PPV_ARGS(&dd.dxgiBackBuffer));
        if (!dd.dxgiBackBuffer)
            return 0;

        // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
        hr = dd.m_d2dContext->CreateBitmapFromDxgiSurface(
            dd.dxgiBackBuffer,
            &bitmapProperties,
            &dd.m_d2dTargetBitmap);
        if (!dd.m_d2dTargetBitmap)
            return 0;

        // Now we can set the Direct2D render target.
        dd.m_d2dContext->SetTarget(dd.m_d2dTargetBitmap);
        return true;

        //        return OnResize(hh, false);
    }
    catch (...)
    {
        return 0;
    }
}


HRESULT D2D::Resize2(int wi, int he)
{
    if (m_swapChain1)
    {
        context->OMSetRenderTargets(0, 0, 0);

        // Release all outstanding references to the swap chain's buffers.
        this->pRenderTargetView = 0;

        HRESULT hr;
        // Preserve the existing buffer count and format.
        // Automatically choose the width and height to match the client rect for HWNDs.
        context->ClearState();
        hr = m_swapChain1->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

        // Perform error handling here!

        // Get buffer and create a render-target-view.
        CComPtr<ID3D11Texture2D> pBuffer;
        hr = m_swapChain1->GetBuffer(0, __uuidof(ID3D11Texture2D),
            (void**)&pBuffer);
        // Perform error handling here!

        hr = device->CreateRenderTargetView(pBuffer, NULL,
            &pRenderTargetView);
        // Perform error handling here!
        pBuffer = 0;

        ID3D11RenderTargetView* v1[] = { pRenderTargetView.p };
        context->OMSetRenderTargets(1, v1, NULL);

        // Set up the viewport.
        D3D11_VIEWPORT vp;
        vp.Width = (float)wi;
        vp.Height = (float)he;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        context->RSSetViewports(1, &vp);
        return S_OK;
    }
    return E_UNEXPECTED;
}


bool D2D::CreateRenderingBitmap(RECT rc)
{
    if (!fa)
        return false;
    if (!wicfac)
    {
        CoCreateInstance(CLSID_WICImagingFactory, 0, CLSCTX_INPROC_SERVER,
            __uuidof(IWICImagingFactory), (void**)&wicfac);
    }
    if (!wicfac)
        return false;

    wicfac->CreateBitmap(rc.right, rc.bottom,
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnLoad, &bmp);
    if (!bmp)
        return false;

    fa->CreateWicBitmapRenderTarget(
        bmp,
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED),
            0.f, 0.f,
            D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE),
        &bitmaprender);

    if (!bitmaprender)
        return false;

    return true;
}



bool D2D::CreateWin7(HWND hh, RECT rc)
{
    D2D1_HWND_RENDER_TARGET_PROPERTIES hp;
    hp.hwnd = hh;
    hp.pixelSize.width = rc.right;
    hp.pixelSize.height = rc.bottom;
    hp.presentOptions = D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS;
    d = 0;

    CComPtr<ID2D1Factory> faa;
#ifdef _DEBUG
    D2D1_FACTORY_OPTIONS options;
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_MULTI_THREADED, options, &faa);
#else
    D2D1CreateFactory(D2D1_FACTORY_TYPE::D2D1_FACTORY_TYPE_MULTI_THREADED, &faa);
#endif
    if (!faa)
        return 0;
    auto props = D2D1::RenderTargetProperties();
    props.dpiX = 96;
    props.dpiY = 96;
    faa->CreateHwndRenderTarget(props, D2D1::HwndRenderTargetProperties(hh, D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top)), &d);

    fa = 0;
    fa1 = 0;
    fa5 = 0;
    d->GetFactory(&fa);
    fa1 = fa;
    fa5 = fa;


    return d;
}



HRESULT D2D::Resize(int wi, int he)
{

    if (!m_d2dContext)
        return E_FAIL;
    if (m_d2dTargetBitmap)
    {
        auto sz = m_d2dTargetBitmap->GetSize();
        if (sz.width == wi && sz.height == he)
            return S_FALSE;
    }
    D2D1_SIZE_U su;
    su.width = wi;
    su.height = he;
    D2D1_BITMAP_PROPERTIES1 p1 = D2D1_BITMAP_PROPERTIES1();
    p1.colorContext = 0;
    p1.pixelFormat.format = pform;
    p1.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    p1.dpiX = 96;
    p1.dpiY = 96;
    p1.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET;
    m_d2dTargetBitmap = 0;
    m_TempBitmapForCopying = 0;
    auto hr = m_d2dContext->CreateBitmap(su, 0, 0, &p1, &m_d2dTargetBitmap);
    m_d2dContext->SetTarget(m_d2dTargetBitmap);
    return hr;
}


