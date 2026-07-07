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

    // Overlay-end detection: the frame's first big depth is the scene depth; a different big depth bound
    // later is the late overlay pass (demo effects, lens flare). A null depth after that ends the overlay.
    std::vector<SceneDepth::EndOf3DCallback> g_overlayEndCallbacks;
    ComPtr<ID3D11Texture2D> g_firstDepthTex;
    bool g_overlayActive = false;
    bool g_overlayEndFired = false;
    bool g_inOverlayCb = false;

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
    bool CopyDepth(ID3D11Texture2D* src)
    {
        g_available = false;
        if (!src)
        {
            return false;
        }
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!dev || !ctx)
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC sd {};
        src->GetDesc(&sd);
        DXGI_FORMAT typeless, srvFmt;
        if (!MapDepthFormat(sd.Format, typeless, srvFmt))
        {
            return false;
        }

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

            if (FAILED(dev->CreateTexture2D(&td, nullptr, g_copyTex.GetAddressOf())))
            {
                return false;
            }

            D3D11_SHADER_RESOURCE_VIEW_DESC srvd {};
            srvd.Format = srvFmt;
            srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvd.Texture2D.MipLevels = 1;

            if (FAILED(dev->CreateShaderResourceView(g_copyTex.Get(), &srvd, g_copySRV.GetAddressOf())))
            {
                g_copyTex.Reset();
                return false;
            }

            g_copyW = sd.Width;
            g_copyH = sd.Height;
            g_copyFmt = typeless;

            spdlog::info("SceneDepth: depth copy target {}x{} fmt={}", sd.Width, sd.Height, static_cast<int>(typeless));
        }

        ctx->CopyResource(g_copyTex.Get(), src);
        g_available = true;
        return true;
    }

    std::atomic<uint32_t> g_shadowSetCounter = 0;

    // ---- Late-pass depth propagation (MGS2) -----------------------------------------------------
    // With MSAA the game renders the scene into an MSAA depth buffer, then binds a separate never-written
    // non-MSAA depth for the late overlay pass, so overlay prims z-test against an empty buffer (and our
    // depth copy captures that same empty buffer). Fill it from the MSAA scene depth when it is first
    // bound each frame.
    ComPtr<ID3D11Texture2D> g_msDepthTex;
    bool g_msSeenThisFrame = false;
    bool g_propagatedThisFrame = false;
    bool g_inPropagate = false;
    bool g_fillFailed = false;
    ComPtr<ID3D11VertexShader>       g_fillVS;
    ComPtr<ID3D11PixelShader>        g_fillPS;
    ComPtr<ID3D11DepthStencilState>  g_fillDSS;
    ComPtr<ID3D11RasterizerState>    g_fillRS;
    ComPtr<ID3D11ShaderResourceView> g_msSRV;
    ID3D11Texture2D*                 g_msSRVFor = nullptr;

    const char* kFillShader = R"(
    Texture2DMS<float> msDepth : register(t0);
    void VS(uint id : SV_VertexID, out float4 pos : SV_Position) {
        float2 uv = float2((id << 1) & 2, id & 2);
        pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    }
    float PS(float4 pos : SV_Position) : SV_Depth {
        return msDepth.Load(int2(pos.xy), 0);
    }
    )";

    bool EnsureFill(ID3D11Device* dev)
    {
        if (g_fillVS && g_fillPS && g_fillDSS && g_fillRS) return true;
        if (g_fillFailed) return false;
        if (!g_D3D11Hooks.D3DCompileFunc) { g_fillFailed = true; return false; }
        ComPtr<ID3DBlob> vsb, psb, err;
        bool ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kFillShader, strlen(kFillShader), nullptr, nullptr, nullptr,
                      "VS", "vs_5_0", 0, 0, vsb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (ok) ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kFillShader, strlen(kFillShader), nullptr, nullptr, nullptr,
                      "PS", "ps_5_0", 0, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (!ok)
        {
            spdlog::error("SceneDepth: depth-propagate shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
            g_fillFailed = true;
            return false;
        }
        if (FAILED(dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, g_fillVS.GetAddressOf())) ||
            FAILED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, g_fillPS.GetAddressOf())))
        {
            g_fillFailed = true;
            return false;
        }
        D3D11_DEPTH_STENCIL_DESC ds {};
        ds.DepthEnable = TRUE; ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; ds.DepthFunc = D3D11_COMPARISON_ALWAYS;
        dev->CreateDepthStencilState(&ds, g_fillDSS.GetAddressOf());
        D3D11_RASTERIZER_DESC rs {};
        rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE; rs.DepthClipEnable = FALSE;
        dev->CreateRasterizerState(&rs, g_fillRS.GetAddressOf());
        if (!g_fillDSS || !g_fillRS) { g_fillFailed = true; return false; }
        return true;
    }

    void PropagateDepth(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, UINT w, UINT h)
    {
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev || !EnsureFill(dev)) return;

        // SRV over the game's MSAA depth (created with SHADER_RESOURCE bind).
        if (g_msSRV && g_msSRVFor != g_msDepthTex.Get()) g_msSRV.Reset();
        if (!g_msSRV)
        {
            D3D11_TEXTURE2D_DESC md {}; g_msDepthTex->GetDesc(&md);
            DXGI_FORMAT typeless, srvFmt;
            if (!MapDepthFormat(md.Format, typeless, srvFmt)) return;
            D3D11_SHADER_RESOURCE_VIEW_DESC sv {};
            sv.Format = srvFmt; sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            if (FAILED(dev->CreateShaderResourceView(g_msDepthTex.Get(), &sv, g_msSRV.GetAddressOf()))) return;
            g_msSRVFor = g_msDepthTex.Get();
        }

        g_inPropagate = true;

        // Save what we clobber. The caller is about to rebind render targets, so those need no restore.
        ComPtr<ID3D11VertexShader> oVS; ComPtr<ID3D11PixelShader> oPS;
        ctx->VSGetShader(oVS.GetAddressOf(), nullptr, nullptr);
        ctx->PSGetShader(oPS.GetAddressOf(), nullptr, nullptr);
        ComPtr<ID3D11InputLayout> oIL; ctx->IAGetInputLayout(oIL.GetAddressOf());
        D3D11_PRIMITIVE_TOPOLOGY oTopo; ctx->IAGetPrimitiveTopology(&oTopo);
        ComPtr<ID3D11DepthStencilState> oDSS; UINT oSR = 0; ctx->OMGetDepthStencilState(oDSS.GetAddressOf(), &oSR);
        ComPtr<ID3D11RasterizerState> oRS; ctx->RSGetState(oRS.GetAddressOf());
        ComPtr<ID3D11ShaderResourceView> oSRV; ctx->PSGetShaderResources(0, 1, oSRV.GetAddressOf());
        UINT oNVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_VIEWPORT oVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ctx->RSGetViewports(&oNVP, oVP);

        ID3D11ShaderResourceView* srv = g_msSRV.Get();
        D3D11_VIEWPORT vp = { 0.f, 0.f, (float)w, (float)h, 0.f, 1.f };
        ctx->OMSetRenderTargets(0, nullptr, dsv);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_fillVS.Get(), nullptr, 0);
        ctx->PSSetShader(g_fillPS.Get(), nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &srv);
        ctx->OMSetDepthStencilState(g_fillDSS.Get(), 0);
        ctx->RSSetState(g_fillRS.Get());
        ctx->RSSetViewports(1, &vp);
        ctx->Draw(3, 0);

        ctx->VSSetShader(oVS.Get(), nullptr, 0);
        ctx->PSSetShader(oPS.Get(), nullptr, 0);
        ctx->IASetInputLayout(oIL.Get());
        ctx->IASetPrimitiveTopology(oTopo);
        ctx->OMSetDepthStencilState(oDSS.Get(), oSR);
        ctx->RSSetState(oRS.Get());
        ID3D11ShaderResourceView* rSRV = oSRV.Get();
        ctx->PSSetShaderResources(0, 1, &rSRV);
        if (oNVP) ctx->RSSetViewports(oNVP, oVP);

        g_inPropagate = false;
        g_propagatedThisFrame = true;
        static bool s_logged = false;
        if (!s_logged) { s_logged = true; spdlog::info("SceneDepth: overlay depth propagation active ({}x{})", w, h); }
    }

    void STDMETHODCALLTYPE HookedOMSetRenderTargets(
        ID3D11DeviceContext* ctx,
        UINT numViews,
        ID3D11RenderTargetView* const* rtvs,
        ID3D11DepthStencilView* dsv)
    {
        // Count shadow passes (square depth-attached targets) - a frame-skip catch-up
        // renders the scene twice in one present, doubling this per frame.
        if (numViews > 0 && rtvs && rtvs[0] && dsv)
        {
            ComPtr<ID3D11Resource> res;
            rtvs[0]->GetResource(res.GetAddressOf());
            ComPtr<ID3D11Texture2D> tex;
            if (res && SUCCEEDED(res.As(&tex)))
            {
                D3D11_TEXTURE2D_DESC d {};
                tex->GetDesc(&d);
                if (d.Width == d.Height && d.Width >= 256)
                {
                    g_shadowSetCounter.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }

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
                const bool bigDepth = MapDepthFormat(d.Format, a, b) && bb &&
                    static_cast<uint64_t>(d.Width) * d.Height >= static_cast<uint64_t>(bb * 0.6f);
                if (bigDepth && !g_inPropagate && !g_inOverlayCb)
                {
                    if (!g_firstDepthTex) g_firstDepthTex = tex;
                    else if (tex.Get() != g_firstDepthTex.Get()) g_overlayActive = true;
                }
                if (bigDepth && d.SampleDesc.Count > 1 && !g_inPropagate)
                {
                    // The MSAA 3D-pass depth: remember it for the late-pass propagation.
                    g_msDepthTex = tex;
                    g_msSeenThisFrame = true;
                    static bool s_loggedMs = false;
                    if (!s_loggedMs) { s_loggedMs = true; spdlog::info("SceneDepth: MSAA scene depth {}x{} count={}", d.Width, d.Height, d.SampleDesc.Count); }
                }
                else if (bigDepth && d.SampleDesc.Count == 1)
                {
                    // A separate non-MSAA depth bound after the MSAA scene = the empty overlay depth;
                    // fill it with the scene depth so the late pass z-tests like the PS2 did.
                    if ((eGameType & MGS2) && g_msSeenThisFrame && !g_propagatedThisFrame && !g_inPropagate &&
                        tex.Get() != g_msDepthTex.Get())
                    {
                        PropagateDepth(ctx, dsv, d.Width, d.Height);
                    }
                    if (!g_inOverlayCb)
                    {
                        g_sceneDepth = tex;
                        if (numViews > 0 && rtvs && rtvs[0])
                            g_sceneColorRTV = rtvs[0];
                    }
                }
            }
        }
        else if (g_overlayActive && !g_overlayEndFired && !g_inOverlayCb && !g_inPropagate)
        {
            g_overlayEndFired = true;
            g_inOverlayCb = true;
            if (g_sceneColorRTV)
            {
                ID3D11ShaderResourceView* depthSRV = g_available ? g_copySRV.Get() : nullptr;
                for (auto cb : g_overlayEndCallbacks)
                    if (cb) cb(g_sceneColorRTV.Get(), depthSRV);
            }
            g_inOverlayCb = false;
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
    g_msSeenThisFrame = false;
    g_propagatedThisFrame = false;
    g_firstDepthTex.Reset();
    g_overlayActive = false;
    g_overlayEndFired = false;
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

void SceneDepth::SetOverlayEndCallback(EndOf3DCallback cb)
{
    if (!cb) return;
    if (std::find(g_overlayEndCallbacks.begin(), g_overlayEndCallbacks.end(), cb) == g_overlayEndCallbacks.end())
        g_overlayEndCallbacks.push_back(cb);
}

uint32_t SceneDepth::ReadAndResetShadowSetCount()
{
    return g_shadowSetCounter.exchange(0, std::memory_order_relaxed);
}

ID3D11ShaderResourceView* SceneDepth::GetSRV()
{
    return g_available ? g_copySRV.Get() : nullptr;
}

bool SceneDepth::IsAvailable()
{
    return g_available;
}

ID3D11ShaderResourceView* SceneDepth::CaptureSceneDepth()
{
    return CopyDepth(g_sceneDepth.Get()) ? g_copySRV.Get() : nullptr;
}

ID3D11ShaderResourceView* SceneDepth::CaptureDepth(ID3D11DepthStencilView* depthStencil)
{
    if (!depthStencil)
    {
        g_available = false;
        return nullptr;
    }

    ComPtr<ID3D11Resource> resource;
    depthStencil->GetResource(resource.GetAddressOf());
    ComPtr<ID3D11Texture2D> texture;
    if (!resource || FAILED(resource.As(&texture)) || !texture)
    {
        g_available = false;
        return nullptr;
    }

    return CopyDepth(texture.Get()) ? g_copySRV.Get() : nullptr;
}
