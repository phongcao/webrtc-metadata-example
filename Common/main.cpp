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
const INT                       kVideoFrameWidth = 1280;
const INT                       kVideoFrameHeight = 720;
const WCHAR                     kFontName[] = L"Verdana";
const FLOAT                     kFontSize = 100;

ComPtr<ID3D11Device>            d3d_device;
ComPtr<ID3D11DeviceContext>     d3d_context;
ComPtr<IDXGISwapChain1>					swapchain;
ComPtr<ID2D1Factory>            d2d_factory;
ComPtr<IDWriteFactory>          dwrite_factory;
ComPtr<IDWriteTextFormat>       text_format;
ComPtr<ID2D1SolidColorBrush>    d2d_brush;
ComPtr<ID2D1RenderTarget>       d2d_render_target;
ComPtr<ID3D11Texture2D>         staging_texture;

WCHAR                           metadata_id_text[64] = { 0 };

INT                             metadata_id = 0;
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
  // Creates custom video capturer.
  std::shared_ptr<CustomVideoCapturer> capturer = std::shared_ptr<CustomVideoCapturer>(
	  new CustomVideoCapturer());

  // Creates conductor.
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
  swapchain_desc.Width = kVideoFrameWidth;
  swapchain_desc.Height = kVideoFrameHeight;

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

  _com_util::CheckError(hr);

  // Gets a frame buffer in the swap chain.
  ComPtr<ID3D11Texture2D> frame_buffer;
  hr = swapchain->GetBuffer(0, IID_PPV_ARGS(&frame_buffer));
  _com_util::CheckError(hr);

  // Obtains a DXGI surface.
  ComPtr<IDXGISurface> dxgi_surface;
  hr = frame_buffer->QueryInterface(dxgi_surface.GetAddressOf());
  _com_util::CheckError(hr);

  // Creates a Direct2D factory.
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d_factory.GetAddressOf());
  _com_util::CheckError(hr);

  // Creates a Direct2D render target which can draw into the surface in the swap chain.
  FLOAT dpi_x;
  FLOAT dpi_y;
  d2d_factory->GetDesktopDpi(&dpi_x, &dpi_y);

  D2D1_RENDER_TARGET_PROPERTIES props =
    D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
      dpi_x,
      dpi_y
    );

  hr = d2d_factory->CreateDxgiSurfaceRenderTarget(
    dxgi_surface.Get(),
    &props,
    &d2d_render_target
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
  hr = d2d_render_target->CreateSolidColorBrush(
    D2D1::ColorF(D2D1::ColorF::White, 1.0f),
    &d2d_brush
  );

  _com_util::CheckError(hr);

  // Creates a staging texture so that we can read pixel data.
  D3D11_TEXTURE2D_DESC tex_desc = { 0 };
  tex_desc.ArraySize = 1;
  tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  tex_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  tex_desc.Width = kVideoFrameWidth;
  tex_desc.Height = kVideoFrameHeight;
  tex_desc.MipLevels = 1;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.Usage = D3D11_USAGE_STAGING;

  hr = d3d_device->CreateTexture2D(&tex_desc, NULL, &staging_texture);
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

        // -----------------------------------------
        // Updates and renders frame buffer.
        // -----------------------------------------
        ULONGLONG tick = GetTickCount64();
        D2D1_RECT_F des_rect = { 
          kVideoFrameWidth / 2 - kFontSize,
          kVideoFrameHeight / 2 - kFontSize,
          kVideoFrameWidth,
          kVideoFrameHeight
        };

        wsprintf(metadata_id_text, L"%d", ++metadata_id);
        d2d_render_target->BeginDraw();
        d2d_render_target->Clear(D2D1::ColorF(D2D1::ColorF::Black));
        d2d_render_target->DrawText(
          metadata_id_text,
          ARRAYSIZE(metadata_id_text) - 1,
          text_format.Get(),
          des_rect,
          d2d_brush.Get()
        );

        d2d_render_target->EndDraw();

        // -----------------------------------------
        // Sends video frame via WebRTC.
        // -----------------------------------------
        int width = kVideoFrameWidth;
        int height = kVideoFrameHeight;
        d3d_context->CopyResource(staging_texture.Get(), frame_buffer.Get());
        rtc::scoped_refptr<webrtc::I420Buffer> buffer =
          webrtc::I420Buffer::Create(width, height);

        // Converts texture to supported video format.
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(d3d_context.Get()->Map(
          staging_texture.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
          libyuv::ABGRToI420(
            (uint8_t*)mapped.pData,
            width * 4,
            buffer.get()->MutableDataY(),
            buffer.get()->StrideY(),
            buffer.get()->MutableDataU(),
            buffer.get()->StrideU(),
            buffer.get()->MutableDataV(),
            buffer.get()->StrideV(),
            width,
            height);

          d3d_context->Unmap(staging_texture.Get(), 0);
        }

        // Updates time stamp.
        auto time_stamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();

        // Creates video frame buffer.
        auto frame = webrtc::VideoFrame(buffer, kVideoRotation_0, time_stamp);
        frame.set_ntp_time_ms(webrtc::Clock::GetRealTimeClock()->CurrentNtpInMilliseconds());
        frame.set_rotation(VideoRotation::kVideoRotation_0);
        frame.set_metadata_id(metadata_id);

        // Sending video frame.
        capturer->SendFrame(frame);

        // -----------------------------------------
        // Present and FPS limiter.
        // -----------------------------------------
        const int interval = 1000 / kMaxFps;
        ULONGLONG time_elapsed = GetTickCount64() - tick;
        DWORD sleep_amount = 0;
        swapchain->Present(1, 0);
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
