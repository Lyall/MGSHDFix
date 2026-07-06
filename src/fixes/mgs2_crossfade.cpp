#include "stdafx.h"
#include "mgs2_crossfade.hpp"

#include "d3d11_api.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "scene_depth.hpp"

// Demo camera crossfade: replay the PS2 frame-store overlay on the D3D11 path.
namespace
{
    const char* kShader = R"(
    Texture2D    g_tex  : register(t0);
    SamplerState g_samp : register(s0);
    cbuffer CB : register(b0) { float gAlpha; float gBrightness; float2 _pad; }

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0) {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    }
    float4 PS(float4 p : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
        float3 c = g_tex.Sample(g_samp, uv).rgb;
        return float4(c * gBrightness, gAlpha);
    }
    )";

    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11Buffer>             cb;
    ComPtr<ID3D11BlendState>         blend;
    ComPtr<ID3D11RasterizerState>    rs;
    ComPtr<ID3D11DepthStencilState>  noDepth;
    ComPtr<ID3D11SamplerState>       sampler;

    ComPtr<ID3D11Texture2D>          prevTex;   // rolling copy of the last presented frame
    ComPtr<ID3D11ShaderResourceView> prevSRV;
    ComPtr<ID3D11Texture2D>          snapTex;   // frozen old frame for the active fade
    ComPtr<ID3D11ShaderResourceView> snapSRV;
    UINT texW = 0, texH = 0;

    bool      ready = false;
    bool      active = false;
    float     fadeTotal = 0.f, fadeCount = 0.f;   // seconds
    ULONGLONG lastTick = 0;
    bool      havePrev = false;
    bool      capturedCleanFrame = false;

    SafetyHookMid getResourcesHook{};
    SafetyHookInline customHook{};

    enum class FadeMode
    {
        Normal,
        Custom
    };

    struct CustomFadeState
    {
        float captureInterval = 0.f;
        float captureCount = 0.f;
        float alphaTime = 0.f;
        float alphaCount = 0.f;
        float brightTime = 0.f;
        float brightCount = 0.f;
    };

    struct PendingCustomFade
    {
        bool active = false;
        int time = 0;
        int captureInterval = 0;
        int brightTime = 0;
        int alphaTime = 0;
        int flag = 0;
    };

    FadeMode fadeMode = FadeMode::Normal;
    CustomFadeState customFade{};
    thread_local PendingCustomFade pendingCustomFade{};

    float TicksToSeconds(int ticks)
    {
        return static_cast<float>(std::max(ticks, 0)) / 300.0f;
    }

    float Clamp01(float v)
    {
        return std::clamp(v, 0.0f, 1.0f);
    }

    bool CaptureSnap(ID3D11Resource* source)
    {
        auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!ctx || !snapTex || !source) return false;
        ctx->CopyResource(snapTex.Get(), source);
        return true;
    }

    void StartNormalFade(int time)
    {
        fadeTotal = fadeCount = TicksToSeconds(time);
        if (fadeTotal <= 0.f || !CaptureSnap(prevTex.Get()))
        {
            active = false;
            return;
        }

        fadeMode = FadeMode::Normal;
        customFade = {};
        active = true;
        lastTick = 0;
    }

    void StartCustomFade(const PendingCustomFade& fade)
    {
        fadeTotal = fadeCount = TicksToSeconds(fade.time);
        if (fadeTotal <= 0.f || !CaptureSnap(prevTex.Get()))
        {
            active = false;
            return;
        }

        fadeMode = FadeMode::Custom;
        customFade.captureInterval = TicksToSeconds(fade.captureInterval);
        customFade.captureCount = 0.f;
        customFade.alphaTime = TicksToSeconds(fade.alphaTime);
        customFade.alphaCount = customFade.alphaTime;
        customFade.brightTime = TicksToSeconds(fade.brightTime);
        customFade.brightCount = customFade.brightTime;
        active = true;
        lastTick = 0;
    }

    void UpdateFade(float dt, ID3D11Resource* currentFrame)
    {
        if (!active || fadeCount <= 0.f) return;

        fadeCount -= dt;
        if (fadeCount <= 0.f)
        {
            fadeCount = 0.f;
            active = false;
            return;
        }

        if (fadeMode != FadeMode::Custom || dt <= 0.f) return;

        customFade.captureCount += dt;
        customFade.alphaCount -= dt;
        customFade.brightCount -= dt;

        if (customFade.captureInterval <= 0.f || customFade.captureCount > customFade.captureInterval)
        {
            if (customFade.captureInterval > 0.f)
            {
                customFade.captureCount -= customFade.captureInterval;
                if (customFade.captureCount > customFade.captureInterval)
                {
                    customFade.captureCount = 0.f;
                }
            }
            else
            {
                customFade.captureCount = 0.f;
            }

            customFade.alphaCount = customFade.alphaTime;
            customFade.brightCount = customFade.brightTime;
            CaptureSnap(currentFrame);
        }

        customFade.alphaCount = std::max(customFade.alphaCount, 0.f);
        customFade.brightCount = std::max(customFade.brightCount, 0.f);
    }

    float CurrentAlpha()
    {
        if (!active) return 0.f;
        if (fadeMode == FadeMode::Custom)
        {
            return Clamp01(customFade.alphaTime > 0.f ? customFade.alphaCount / customFade.alphaTime : 1.f);
        }
        return Clamp01(fadeTotal > 0.f ? fadeCount / fadeTotal : 0.f);
    }

    float CurrentBrightness()
    {
        if (!active || fadeMode != FadeMode::Custom) return 1.f;
        return Clamp01(customFade.brightTime > 0.f ? customFade.brightCount / customFade.brightTime : 1.f);
    }

    void* __fastcall CrossfadeCustom_Detour(int time, int captureInterval, int brightTime, int alphaTime, int flag)
    {
        PendingCustomFade previous = pendingCustomFade;
        if (brightTime != 0 || alphaTime != 0 || flag != 0)
        {
            pendingCustomFade = { true, time, captureInterval, brightTime, alphaTime, flag };
        }
        else
        {
            pendingCustomFade = {};
        }

        void* result = customHook.fastcall<void*>(time, captureInterval, brightTime, alphaTime, flag);
        pendingCustomFade = previous;
        return result;
    }

    bool EnsureTextures(ID3D11Device* dev, const D3D11_TEXTURE2D_DESC& bb)
    {
        if (prevTex && texW == bb.Width && texH == bb.Height) return true;
        prevSRV.Reset(); prevTex.Reset(); snapSRV.Reset(); snapTex.Reset(); havePrev = false;

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = bb.Width; td.Height = bb.Height; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = bb.Format; td.SampleDesc = { 1, 0 };
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, prevTex.GetAddressOf()))) return false;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, snapTex.GetAddressOf()))) return false;
        dev->CreateShaderResourceView(prevTex.Get(), nullptr, prevSRV.GetAddressOf());
        dev->CreateShaderResourceView(snapTex.Get(), nullptr, snapSRV.GetAddressOf());
        texW = bb.Width; texH = bb.Height;
        return true;
    }
}

void MGS2_Crossfade::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled) return;
    if (!g_D3D11Hooks.D3DCompileFunc) { spdlog::error("MGS2_Crossfade: no D3DCompile"); return; }
    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!dev) { spdlog::error("MGS2_Crossfade: no device"); return; }

    auto compile = [&](const char* e, const char* t, ComPtr<ID3DBlob>& b) {
        ComPtr<ID3DBlob> err;
        HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                                                 e, t, 0, 0, b.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr)) { spdlog::error("MGS2_Crossfade: compile {} failed: {}", e, err ? (const char*)err->GetBufferPointer() : "?"); return false; }
        return true;
    };
    ComPtr<ID3DBlob> vsB, psB;
    if (!compile("VS", "vs_5_0", vsB) || !compile("PS", "ps_5_0", psB)) return;
    dev->CreateVertexShader(vsB->GetBufferPointer(), vsB->GetBufferSize(), nullptr, vs.GetAddressOf());
    dev->CreatePixelShader(psB->GetBufferPointer(), psB->GetBufferSize(), nullptr, ps.GetAddressOf());

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = 16; cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&cbd, nullptr, cb.GetAddressOf());

    D3D11_BLEND_DESC bd = {};
    auto& rt = bd.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;  rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA; rt.BlendOp = D3D11_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D11_BLEND_ONE;   rt.DestBlendAlpha = D3D11_BLEND_ZERO;     rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&bd, blend.GetAddressOf());

    D3D11_RASTERIZER_DESC rd = {}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE;
    dev->CreateRasterizerState(&rd, rs.GetAddressOf());

    D3D11_DEPTH_STENCIL_DESC dsd = {}; dsd.DepthEnable = FALSE; dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dev->CreateDepthStencilState(&dsd, noDepth.GetAddressOf());

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER; sd.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&sd, sampler.GetAddressOf());

    // GetResources time is in 1/300s ticks.
    if (uint8_t* addr = Memory::PatternScan(baseModule, "40 53 55 56 57 41 56 48 83 EC ?? 41 8B C0 89 51 ?? 83 E0 01 89 51 ??", "MGS2: Crossfade GetResources"))
    {
        getResourcesHook = safetyhook::create_mid(addr, [](SafetyHookContext& ctx)
        {
            if (!ready || !havePrev) return;

            const uint32_t t = static_cast<uint32_t>(ctx.rdx & 0xFFFFFFFF);
            if (pendingCustomFade.active)
            {
                StartCustomFade(pendingCustomFade);
            }
            else
            {
                StartNormalFade(static_cast<int>(t));
            }
        });
        LOG_HOOK(getResourcesHook, "MGS2: Crossfade GetResources");
    }

    if (uint8_t* addr = Memory::PatternScan(baseModule, "48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 8B 6C 24 ?? 41 8B F9 41 8B F0 44 8B F2 44 8B F9 45 85 C0", "MGS2: Crossfade Custom"))
    {
        customHook = safetyhook::create_inline(addr, reinterpret_cast<void*>(CrossfadeCustom_Detour));
        LOG_HOOK(customHook, "MGS2: Crossfade Custom");
    }

    SceneDepth::SetEndOf3DCallback(&OnPreMenuRender, SceneDepth::PRIORITY_CROSSFADE);

    ready = true;
    spdlog::info("MGS2_Crossfade initialized.");
}

void MGS2_Crossfade::OnPreMenuRender(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth)
{
    if (!ready || !bEnabled || capturedCleanFrame) return;
    if (!g_GameVars.InCutscene()) return;

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!ctx) return;

    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return;

    ComPtr<ID3D11Resource> res;
    rtv->GetResource(res.GetAddressOf());
    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(res.As(&tex))) { rtv->Release(); return; }

    D3D11_TEXTURE2D_DESC td;
    tex->GetDesc(&td);
    const bool sizeMatch = (td.Width == texW && td.Height == texH);

    // timing
    ULONGLONG now = GetTickCount64();
    float dt = (lastTick != 0) ? (now - lastTick) / 1000.0f : 0.f;
    lastTick = now;

    UpdateFade(dt, sizeMatch ? tex.Get() : nullptr);

    // composite the active fade: old (frozen) frame over the live new angle
    if (active && snapSRV && sizeMatch)
    {
        const float alpha = CurrentAlpha();
        const float brightness = CurrentBrightness();

        if (alpha > 0.f)
        {
            D3D11_MAPPED_SUBRESOURCE m;
            if (SUCCEEDED(ctx->Map(cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m)))
            {
                // Fold brightness into alpha - dimming the overlay colour like the PS2
                // darkens the whole mix here, so keep it a straight live/snap blend.
                float d[4] = { alpha * brightness, 1.0f, 0, 0 }; memcpy(m.pData, d, 16); ctx->Unmap(cb.Get(), 0);
            }

            // save the pipeline state we touch, draw the fade, then restore it
            ID3D11RenderTargetView* oRTV[8] = {}; ID3D11DepthStencilView* oDSV = nullptr;
            ID3D11BlendState* oBlend = nullptr; UINT oMask = 0; float oFac[4] = {};
            ID3D11DepthStencilState* oDSS = nullptr; UINT oRef = 0;
            ID3D11RasterizerState* oRS = nullptr; D3D11_VIEWPORT oVP[16] = {}; UINT oNVP = 16;
            ID3D11VertexShader* oVS = nullptr; ID3D11PixelShader* oPS = nullptr;
            ID3D11InputLayout* oIL = nullptr; D3D11_PRIMITIVE_TOPOLOGY oTopo;
            ID3D11ShaderResourceView* oSRV = nullptr; ID3D11SamplerState* oSamp = nullptr; ID3D11Buffer* oCB = nullptr;
            ctx->OMGetRenderTargets(8, oRTV, &oDSV);
            ctx->OMGetBlendState(&oBlend, oFac, &oMask);
            ctx->OMGetDepthStencilState(&oDSS, &oRef);
            ctx->RSGetState(&oRS); ctx->RSGetViewports(&oNVP, oVP);
            ctx->VSGetShader(&oVS, nullptr, nullptr); ctx->PSGetShader(&oPS, nullptr, nullptr);
            ctx->IAGetInputLayout(&oIL); ctx->IAGetPrimitiveTopology(&oTopo);
            ctx->PSGetShaderResources(0, 1, &oSRV); ctx->PSGetSamplers(0, 1, &oSamp); ctx->PSGetConstantBuffers(0, 1, &oCB);

            D3D11_VIEWPORT vp = { 0, 0, (float)td.Width, (float)td.Height, 0.f, 1.f };
            ctx->IASetInputLayout(nullptr);
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ctx->VSSetShader(vs.Get(), nullptr, 0);
            ctx->PSSetShader(ps.Get(), nullptr, 0);
            ctx->PSSetShaderResources(0, 1, snapSRV.GetAddressOf());
            ctx->PSSetSamplers(0, 1, sampler.GetAddressOf());
            ctx->PSSetConstantBuffers(0, 1, cb.GetAddressOf());
            ctx->OMSetBlendState(blend.Get(), nullptr, 0xFFFFFFFF);
            ctx->OMSetDepthStencilState(noDepth.Get(), 0);
            ctx->RSSetState(rs.Get()); ctx->RSSetViewports(1, &vp);
            ctx->OMSetRenderTargets(1, &rtv, nullptr);
            ctx->Draw(3, 0);

            ctx->OMSetRenderTargets(8, oRTV, oDSV);
            ctx->OMSetBlendState(oBlend, oFac, oMask);
            ctx->OMSetDepthStencilState(oDSS, oRef);
            ctx->RSSetState(oRS); ctx->RSSetViewports(oNVP, oVP);
            ctx->VSSetShader(oVS, nullptr, 0); ctx->PSSetShader(oPS, nullptr, 0);
            ctx->IASetInputLayout(oIL); ctx->IASetPrimitiveTopology(oTopo);
            ctx->PSSetShaderResources(0, 1, &oSRV); ctx->PSSetSamplers(0, 1, &oSamp); ctx->PSSetConstantBuffers(0, 1, &oCB);
            for (auto* r : oRTV) if (r) r->Release();
            if (oDSV) oDSV->Release(); if (oBlend) oBlend->Release(); if (oDSS) oDSS->Release();
            if (oRS) oRS->Release(); if (oVS) oVS->Release(); if (oPS) oPS->Release(); if (oIL) oIL->Release();
            if (oSRV) oSRV->Release(); if (oSamp) oSamp->Release(); if (oCB) oCB->Release();
        }
    }

    // capture clean 3D frame for next crossfade
    if (prevTex && sizeMatch)
    {
        ctx->CopyResource(prevTex.Get(), tex.Get());
        havePrev = true;
        capturedCleanFrame = true;
    }

    rtv->Release();
}

void MGS2_Crossfade::OnPresent(IDXGISwapChain* swap)
{
    if (!ready || !bEnabled || !swap) return;
    if (!g_GameVars.InCutscene()) return;

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    auto* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!ctx || !dev) return;

    ComPtr<ID3D11Texture2D> bb;
    if (FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)bb.GetAddressOf())) || !bb) return;
    D3D11_TEXTURE2D_DESC bd; bb->GetDesc(&bd);

    if (!EnsureTextures(dev, bd)) return;

    // fallback: fires if DmaPack hook RT wasn't the backbuffer
    if (!capturedCleanFrame)
    {
        ctx->CopyResource(prevTex.Get(), bb.Get());
        havePrev = true;
    }

    capturedCleanFrame = false;
}

bool MGS2_Crossfade::IsFading()
{
    return bEnabled && active;
}
