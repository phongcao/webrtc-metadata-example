/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "stdafx.h"

#include "webrtc.h"
#ifdef SENDER_APP
#include "custom_video_capturer.h"
#endif // SENDER_APP

// Required app libs
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "usp10.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#ifdef SENDER_APP
using namespace Microsoft::WRL;

const INT                       kMaxFps = 60;
const WCHAR                     kFontName[] = L"Verdana";
const FLOAT                     kFontSize = 50;

ComPtr<ID3D11Device>            d3d_device;
ComPtr<ID3D11DeviceContext>     d3d_context;
ComPtr<IDXGISwapChain1>					swapchain;
ComPtr<ID2D1Factory>            d2d_factory;
ComPtr<IDWriteFactory>          dwrite_factory;
ComPtr<IDWriteTextFormat>       text_format;
ComPtr<ID2D1SolidColorBrush>    d2d_brush;
ComPtr<ID3D11Texture2D>         offscreen_texture;
ComPtr<ID2D1RenderTarget>       offscreen_render_target;

WCHAR                           frame_id_text[64] = { 0 };

int                             frame_id = 0;
#endif // SENDER_APP

int PASCAL wWinMain(HINSTANCE instance, HINSTANCE prev_instance,
                    wchar_t* cmd_line, int cmd_show) {
  rtc::EnsureWinsockInit();
  rtc::Win32Thread w32_thread;
  rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

  rtc::WindowsCommandLineArguments win_args;
  int argc = win_args.argc();
  char **argv = win_args.argv();

  rtc::FlagList::SetFlagsFromCommandLine(&argc, argv, true);
  if (FLAG_help) {
    rtc::FlagList::Print(NULL, false);
    return 0;
  }

  // Abort if the user specifies a port that is outside the allowed
  // range [1, 65535].
  if ((FLAG_port < 1) || (FLAG_port > 65535)) {
    printf("Error: %i is not a valid port.\n", FLAG_port);
    return -1;
  }

  MainWnd wnd(FLAG_server, FLAG_port, FLAG_autoconnect, FLAG_autocall);
  if (!wnd.Create()) {
    RTC_NOTREACHED();
    return -1;
  }

  rtc::InitializeSSL();
  PeerConnectionClient client;

#ifdef SENDER_APP
  // Calculates window width and height.
  RECT rect;
  GetClientRect(wnd.handle(), &rect);
  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  // Creates and initializes the custom video capturer.
  // Note: Conductor is responsible for cleaning up the capturer object.
  std::shared_ptr<CustomVideoCapturer> capturer = std::shared_ptr<CustomVideoCapturer>(
	  new CustomVideoCapturer());

  rtc::scoped_refptr<Conductor> conductor(
        new rtc::RefCountedObject<Conductor>(&client, &wnd, capturer.get()));

  // Initializes Direct3D device and context.
  UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
  creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

  HRESULT hr = S_OK;
  hr = D3D11CreateDevice(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    creation_flags,
    nullptr,
    0,
    D3D11_SDK_VERSION,
    &d3d_device,
    nullptr,
    &d3d_context);

  _com_util::CheckError(hr);

  // Initializes swap chain.
  DXGI_SWAP_CHAIN_DESC1 swapchain_desc = { 0 };
  swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.BufferCount = 2; // Front and back buffer to swap
  swapchain_desc.SampleDesc.Count = 1; // Disable anti-aliasing
  swapchain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  swapchain_desc.Width = width;
  swapchain_desc.Height = height;

  ComPtr<IDXGIDevice1> dxgi_device;
  hr = d3d_device.As(&dxgi_device);
  _com_util::CheckError(hr);

  ComPtr<IDXGIAdapter> dxgi_adapter;
  hr = dxgi_device->GetAdapter(&dxgi_adapter);
  _com_util::CheckError(hr);

  ComPtr<IDXGIFactory2> dxgi_factory;
  hr = dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), &dxgi_factory);
  _com_util::CheckError(hr);

  hr = dxgi_factory->CreateSwapChainForHwnd(
    d3d_device.Get(),
    wnd.handle(),
    &swapchain_desc,
    nullptr,
    nullptr,
    &swapchain
  );

  // Allocates an offscreen D3D surface for D2D to render our 2D content into.
  D3D11_TEXTURE2D_DESC tex_desc;
  tex_desc.ArraySize = 1;
  tex_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  tex_desc.Height = height;
  tex_desc.Width = width;
  tex_desc.MipLevels = 1;
  tex_desc.MiscFlags = 0;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;

  hr = d3d_device->CreateTexture2D(&tex_desc, NULL, &offscreen_texture);
  _com_util::CheckError(hr);

  // Obtains a DXGI surface.
  ComPtr<IDXGISurface> dxgi_surface;
  hr = offscreen_texture->QueryInterface(dxgi_surface.GetAddressOf());
  _com_util::CheckError(hr);

  // Creates a Direct2D factory.
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d_factory.GetAddressOf());
  _com_util::CheckError(hr);

  // Creates a D2D render target which can draw into our offscreen D3D
  // surface. Given that we use a constant size for the texture, we
  // fix the DPI at 96.
  D2D1_RENDER_TARGET_PROPERTIES props =
    D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
      96,
      96
    );

  hr = d2d_factory->CreateDxgiSurfaceRenderTarget(
    dxgi_surface.Get(),
    &props,
    &offscreen_render_target
  );

  _com_util::CheckError(hr);

  // Creates a DirectWrite factory.
  hr = DWriteCreateFactory(
    DWRITE_FACTORY_TYPE_SHARED,
    __uuidof(dwrite_factory),
    reinterpret_cast<IUnknown **>(dwrite_factory.GetAddressOf())
  );

  _com_util::CheckError(hr);

  // Creates a DirectWrite text format object.
  hr = dwrite_factory->CreateTextFormat(
    kFontName,
    NULL,
    DWRITE_FONT_WEIGHT_NORMAL,
    DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH_NORMAL,
    kFontSize,
    L"", // locale
    &text_format
  );

  _com_util::CheckError(hr);

  // Creates a solid color brush.
  hr = offscreen_render_target->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::White, 1.0f),
    &d2d_brush
  );

  _com_util::CheckError(hr);
#else // SENDER_APP
  rtc::scoped_refptr<Conductor> conductor(
    new rtc::RefCountedObject<Conductor>(&client, &wnd));
#endif // SENDER_APP

  // Main loop.
  bool got_msg;
  MSG msg;
  msg.message = WM_NULL;
  PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);
  while (WM_QUIT != msg.message) {
    got_msg = (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0);
    if (got_msg) {
      if (!wnd.PreTranslateMessage(&msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
#ifdef SENDER_APP
    else {
      if ((conductor->connection_active() || client.is_connected()) && 
        wnd.current_ui() == MainWindow::STREAMING) {

        // Updates tick.
        ULONGLONG tick = GetTickCount64();

        // Updates offscreen texture.
        RECT rc;
        GetClientRect(wnd.handle(), &rc);

        FLOAT dpi_x, dpi_y;
        FLOAT scale_x, scale_y;
        d2d_factory->GetDesktopDpi(&dpi_x, &dpi_y);
        scale_x = 96.0f / dpi_x;
        scale_y = 96.0f / dpi_y;

        D2D1_RECT_F des_rect = {
          rc.left * scale_x,
          rc.top * scale_y,
          rc.right * scale_x,
          rc.bottom * scale_y
        };

        wsprintf(frame_id_text, L"%d", ++frame_id);
        offscreen_render_target->BeginDraw();
        offscreen_render_target->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        offscreen_render_target->DrawText(
          frame_id_text,
          ARRAYSIZE(frame_id_text) - 1,
          text_format.Get(),
          des_rect,
          d2d_brush.Get()
        );

        offscreen_render_target->EndDraw();

        // Renders offscreen texture.
        ComPtr<ID3D11Texture2D> frame_buffer;
        hr = swapchain->GetBuffer(0, IID_PPV_ARGS(&frame_buffer));
        _com_util::CheckError(hr);
        d3d_context->CopyResource(frame_buffer.Get(), offscreen_texture.Get());
        swapchain->Present(1, 0);

        // FPS limiter.
        const int interval = 1000 / kMaxFps;
        ULONGLONG time_elapsed = GetTickCount64() - tick;
        DWORD sleep_amount = 0;
        if (time_elapsed < interval) {
          sleep_amount = interval - time_elapsed;
        }

        Sleep(sleep_amount);
      }
    }
#endif // SENDER_APP
  }

  rtc::CleanupSSL();
  return 0;
}
