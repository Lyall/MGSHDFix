#include "stdafx.h"
#include "photo_camera.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

#pragma warning(push)
#pragma warning(disable:4828)
#include "isteamscreenshots.h"
#pragma warning(pop)

// The camera freezes the screen for its capture, so the next present still shows exactly the
// photo frame - grab the backbuffer there and hand it to Steam alongside the game's own
// save-file capture.
namespace
{
    std::atomic<bool> gPending = false;
    SafetyHookMid gSnapHook {};

    void Capture(IDXGISwapChain* swap)
    {
        ComPtr<ID3D11Texture2D> backbuffer;
        swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()));
        if (!backbuffer || !g_D3D11Hooks.d3dDevice || !g_D3D11Hooks.d3dDeviceContext)
        {
            return;
        }
        D3D11_TEXTURE2D_DESC desc {};
        backbuffer->GetDesc(&desc);
        const bool bgra = desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM || desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        if (!bgra && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        {
            spdlog::warn("Photo Camera - unsupported backbuffer format {}.", static_cast<int>(desc.Format));
            return;
        }

        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc = { 1, 0 };
        ComPtr<ID3D11Texture2D> staging;
        g_D3D11Hooks.d3dDevice->CreateTexture2D(&desc, nullptr, staging.GetAddressOf());
        if (!staging)
        {
            return;
        }
        ID3D11DeviceContext* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
        ctx->CopyResource(staging.Get(), backbuffer.Get());

        // The stall is fine: the game itself is holding the frame for its own capture.
        D3D11_MAPPED_SUBRESOURCE mapped {};
        if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        {
            return;
        }
        std::vector<uint8_t> rgb(static_cast<size_t>(desc.Width) * desc.Height * 3);
        for (UINT y = 0; y < desc.Height; ++y)
        {
            const uint8_t* src = static_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch;
            uint8_t* dst = rgb.data() + static_cast<size_t>(y) * desc.Width * 3;
            for (UINT x = 0; x < desc.Width; ++x, src += 4, dst += 3)
            {
                dst[0] = src[bgra ? 2 : 0];
                dst[1] = src[1];
                dst[2] = src[bgra ? 0 : 2];
            }
        }
        ctx->Unmap(staging.Get(), 0);

        if (ISteamScreenshots* shots = SteamScreenshots())
        {
            shots->WriteScreenshot(rgb.data(), static_cast<uint32>(rgb.size()), desc.Width, desc.Height);
        }
    }
}

void PhotoCamera::OnPresent(IDXGISwapChain* swap)
{
    if (gPending.exchange(false, std::memory_order_relaxed))
    {
        Capture(swap);
    }
}

void PhotoCamera::Initialize()
{
    if (!bEnabled || !(eGameType & (MGS2 | MGS3)))
    {
        return;
    }
    if (!g_SteamAPI.bInitialized)
    {
        spdlog::info("MGS 2: Photo Camera: SteamAPI not initialized. Skipping setup.");
        return;
    }

    // Both games freeze the screen for the capture (DG_UnDrawFrameCount parked at its max),
    // so that store is the shutter moment: once per photo, every camera mode.
    uint8_t* address = eGameType & MGS2
        ? Memory::PatternScan(baseModule,
            "8B 0D ?? ?? ?? ?? 44 89 35 ?? ?? ?? ?? 89 4F",
            "MGS 2: Photo Camera - Capture Start | skoba\\equip\\capture.c -> NewCaptureStart()")
        : Memory::PatternScan(baseModule,
            "8B 05 ?? ?? ?? ?? 8B C8 48 8B 5C 24",
            "MGS 3: Photo Camera - Capture Start | mc_photo.c photo actor");
    if (address == nullptr)
    {
        return;
    }
    gSnapHook = safetyhook::create_mid(address, [](SafetyHookContext&)
    {
        gPending.store(true, std::memory_order_relaxed);
    });
    LOG_HOOK(gSnapHook, "Photo Camera - Capture Start")
}
