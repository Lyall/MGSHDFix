#include "stdafx.h"
#include "scene_depth.hpp"

#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"

using Microsoft::WRL::ComPtr;

namespace
{
    SafetyHookInline g_omSetRTHook {};

    ComPtr<ID3D11Texture2D>          g_copyTex;
    ComPtr<ID3D11ShaderResourceView> g_copySRV;
    UINT g_copyW = 0, g_copyH = 0;
    DXGI_FORMAT g_copyFmt = DXGI_FORMAT_UNKNOWN;
    bool g_available = false;

    // 3D-pass tracking, for the end-of-3D transition (draw effects under the UI).
    ComPtr<ID3D11Texture2D>          g_sceneDepth;     // the scene depth currently bound
    ComPtr<ID3D11RenderTargetView>   g_sceneColorRTV;  // colour RT bound alongside it
    bool g_in3D = false;
    bool g_fired = false;
    UINT g_bbArea = 0;

    struct CallbackEntry
    {
        SceneDepth::EndOf3DCallback callback;
        int priority;
    };

    std::vector<CallbackEntry> g_callbacks;

    bool MapDepthFormat(DXGI_FORMAT texFmt, DXGI_FORMAT& typeless, DXGI_FORMAT& srv)
    {
        switch (texFmt)
        {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            typeless = DXGI_FORMAT_R24G8_TYPELESS;       srv = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;    return true;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_TYPELESS:
            typeless = DXGI_FORMAT_R32_TYPELESS;         srv = DXGI_FORMAT_R32_FLOAT;                return true;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            typeless = DXGI_FORMAT_R32G8X24_TYPELESS;    srv = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; return true;
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_TYPELESS:
            typeless = DXGI_FORMAT_R16_TYPELESS;         srv = DXGI_FORMAT_R16_UNORM;                return true;
        default:
            return false;
        }
    }

    UINT BackbufferArea()
    {
        if (g_bbArea) return g_bbArea;
        auto* sc = g_D3D11Hooks.swapChain.Get();
        if (!sc) return 0;
        DXGI_SWAP_CHAIN_DESC d {};
        if (SUCCEEDED(sc->GetDesc(&d))) g_bbArea = d.BufferDesc.Width * d.BufferDesc.Height;
        return g_bbArea;
    }

    // Copy a DSV texture into our shader-readable depth copy (CopyResource within the same typeless family).
    void CopyDepth(ID3D11Texture2D* src)
    {
        if (!src) return;
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!dev || !ctx) return;

        D3D11_TEXTURE2D_DESC sd {};
        src->GetDesc(&sd);
        DXGI_FORMAT typeless, srvFmt;
        if (!MapDepthFormat(sd.Format, typeless, srvFmt)) return;

        if (!g_copyTex || g_copyW != sd.Width || g_copyH != sd.Height || g_copyFmt != typeless)
        {
            g_copySRV.Reset();
            g_copyTex.Reset();

            D3D11_TEXTURE2D_DESC td = sd;
            td.Format = typeless;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.CPUAccessFlags = 0;
            td.MiscFlags = 0;

            if (FAILED(dev->CreateTexture2D(&td, nullptr, g_copyTex.GetAddressOf()))) return;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvd {};
            srvd.Format = srvFmt;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;

            if (FAILED(dev->CreateShaderResourceView(g_copyTex.Get(), &srvd, g_copySRV.GetAddressOf())))
            {
                g_copyTex.Reset();
                return;
            }

            g_copyW = sd.Width;
            g_copyH = sd.Height;
            g_copyFmt = typeless;

            spdlog::info("SceneDepth: depth copy target {}x{} fmt={}", sd.Width, sd.Height, static_cast<int>(typeless));
        }

        ctx->CopyResource(g_copyTex.Get(), src);
        g_available = true;
    }

    void STDMETHODCALLTYPE HookedOMSetRenderTargets(
        ID3D11DeviceContext* ctx,
        UINT numViews,
        ID3D11RenderTargetView* const* rtvs,
        ID3D11DepthStencilView* dsv)
    {
        if (dsv)
        {
            ComPtr<ID3D11Resource> res;
            dsv->GetResource(res.GetAddressOf());
            ComPtr<ID3D11Texture2D> tex;
            if (res && SUCCEEDED(res.As(&tex)))
            {
                D3D11_TEXTURE2D_DESC d {};
                tex->GetDesc(&d);
                DXGI_FORMAT a, b;
                UINT bb = BackbufferArea();
                if (d.SampleDesc.Count == 1 && MapDepthFormat(d.Format, a, b) && bb &&
                    static_cast<uint64_t>(d.Width) * d.Height >= static_cast<uint64_t>(bb * 0.6f))
                {
                    g_sceneDepth = tex;
                    if (numViews > 0 && rtvs && rtvs[0])
                        g_sceneColorRTV = rtvs[0];
                }
            }
        }

        g_omSetRTHook.stdcall<void>(ctx, numViews, rtvs, dsv);
    }

}

void SceneDepth::Initialize()
{
    if (g_omSetRTHook) return;

    ID3D11DeviceContext* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!ctx)
    {
        spdlog::error("SceneDepth: no device context to hook.");
        return;
    }

    void** vtable = *reinterpret_cast<void***>(ctx);
    g_omSetRTHook = safetyhook::create_inline(vtable[33], reinterpret_cast<void*>(HookedOMSetRenderTargets));
    LOG_HOOK(g_omSetRTHook, "SceneDepth: OMSetRenderTargets");
}

void SceneDepth::OnPreMenuRender()
{
    if (g_fired) return;
    g_fired = true;

    CopyDepth(g_sceneDepth.Get());

    if (g_sceneColorRTV)
    {
        ID3D11ShaderResourceView* depthSRV = g_available ? g_copySRV.Get() : nullptr;
        for (const auto& entry : g_callbacks)
            if (entry.callback) entry.callback(g_sceneColorRTV.Get(), depthSRV);
    }
}


void SceneDepth::ResetStatus()
{
    // Reset per-frame transition state (the depth copy + callbacks happen mid-frame at the 3D->UI edge).
    g_fired = false;
    g_in3D = false;
    g_sceneColorRTV.Reset();
    g_sceneDepth.Reset();
}

void SceneDepth::SetEndOf3DCallback(EndOf3DCallback cb, int priority)
{
    if (!cb)
    {
        return;
    }

    const auto it = std::find_if(
        g_callbacks.begin(),
        g_callbacks.end(),
        [cb](const CallbackEntry& entry)
        {
            return entry.callback == cb;
        });

    if (it == g_callbacks.end())
    {
        g_callbacks.push_back({ cb, priority });
    }
    else
    {
        it->priority = priority;
    }

    std::stable_sort(
        g_callbacks.begin(),
        g_callbacks.end(),
        [](const CallbackEntry& lhs, const CallbackEntry& rhs)
        {
            return lhs.priority < rhs.priority;
        });
}

ID3D11ShaderResourceView* SceneDepth::GetSRV()
{
    return g_available ? g_copySRV.Get() : nullptr;
}

bool SceneDepth::IsAvailable()
{
    return g_available;
}
