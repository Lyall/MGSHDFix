#include "stdafx.h"
#include "mgs2_demo_blur.hpp"
#include <cmath>
#include <algorithm>

#include "common.hpp"
#include "d3d11_api.hpp"
#include "scene_depth.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

namespace
{
    // Blur actor Act(); work: +0x60 packet (bit 0x100 = hidden), +0x78 intense (float, 0..128).
    constexpr const char* kActSig =
        "48 8B C4 48 89 58 ?? 57 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 48 8D 50";

    SafetyHookInline g_actHook {};

    // Several blur actors can run at once; the game draws each one's blend in sequence. Equivalent
    // single factor: 1 - prod(1 - intense_i/128), accumulated across the frame's Act calls.
    std::atomic<float>     g_factor { 0.0f };
    std::atomic<ULONGLONG> g_lastActMs { 0 };

    void ReadInstance(uintptr_t work)
    {
        static int   s_clock = -0x7fffffff;
        static float s_keep = 1.0f;
        if (const int clock = g_GameVars.DG_Clock(); clock != s_clock)
        {
            s_clock = clock;
            s_keep = 1.0f;
        }

        const float intense = *reinterpret_cast<const float*>(work + 0x78);
        const uintptr_t packet = *reinterpret_cast<const uintptr_t*>(work + 0x60);
        const bool hidden = !packet || (*reinterpret_cast<const uint32_t*>(packet) & 0x100) != 0;
        if (!hidden && intense > 0.5f)
            s_keep *= 1.0f - std::min(intense, 128.0f) / 128.0f;
        g_factor.store(1.0f - s_keep, std::memory_order_relaxed);
        g_lastActMs.store(GetTickCount64(), std::memory_order_relaxed);
    }

    void __fastcall Act_Detour(uintptr_t work)
    {
        g_actHook.fastcall<void>(work);
        if (!work) return;
        __try { ReadInstance(work); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    const char* kShader = R"(
    Texture2D    prevTex : register(t0);
    SamplerState sampLin : register(s0);
    cbuffer CB : register(b0) { float gFactor; float3 _pad; }
    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0) {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    }
    float4 PS(float4 p : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
        // out = (prev - cur) * intense/128 + cur, with prev sampled half a 512x448 texel offset - the
        // same bilinear spread the original picked up per iteration, independent of render resolution.
        const float2 kOffset = float2(0.5 / 512.0, 0.5 / 448.0);
        return float4(prevTex.Sample(sampLin, uv + kOffset).rgb, gFactor);
    }
    )";

    ComPtr<ID3D11VertexShader>      g_vs;
    ComPtr<ID3D11PixelShader>       g_ps;
    ComPtr<ID3D11Buffer>            g_cb;
    ComPtr<ID3D11BlendState>        g_blend;
    ComPtr<ID3D11RasterizerState>   g_rs;
    ComPtr<ID3D11DepthStencilState> g_dss;
    ComPtr<ID3D11SamplerState>      g_samp;
    ComPtr<ID3D11Texture2D>         g_prevTex;
    ComPtr<ID3D11ShaderResourceView> g_prevSRV;
    UINT g_prevW = 0, g_prevH = 0;
    bool g_havePrev = false;
    bool g_d3dInit = false, g_d3dFailed = false;

    ID3D11Device* g_boundDevice = nullptr;

    bool EnsureD3D(ID3D11Device* dev)
    {
        // The game recreates its device on some scene transitions - rebuild on change.
        if (dev != g_boundDevice)
        {
            g_d3dInit = false; g_d3dFailed = false; g_havePrev = false;
            g_prevSRV.Reset(); g_prevTex.Reset(); g_prevW = g_prevH = 0;
            g_cb.Reset(); g_blend.Reset(); g_rs.Reset(); g_dss.Reset(); g_samp.Reset();
            g_vs.Reset(); g_ps.Reset();
            g_boundDevice = dev;
        }
        if (g_d3dInit) return true;
        if (g_d3dFailed) return false;
        if (!g_D3D11Hooks.D3DCompileFunc) { g_d3dFailed = true; return false; }

        ComPtr<ID3DBlob> vsb, psb, err;
        bool ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                        "VS", "vs_5_0", 0, 0, vsb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (ok) ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                        "PS", "ps_5_0", 0, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (!ok)
        {
            spdlog::error("MGS 2: Demo Blur: shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
            g_d3dFailed = true;
            return false;
        }
        if (FAILED(dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, g_vs.GetAddressOf())) ||
            FAILED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, g_ps.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_BUFFER_DESC bd {};
        bd.ByteWidth = 16; bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&bd, nullptr, g_cb.GetAddressOf());

        D3D11_BLEND_DESC bl {};
        bl.RenderTarget[0].BlendEnable = TRUE;
        bl.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bl.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bl.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bl.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        bl.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        bl.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&bl, g_blend.GetAddressOf());

        D3D11_RASTERIZER_DESC rs {};
        rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE;
        dev->CreateRasterizerState(&rs, g_rs.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC ds {};
        ds.DepthEnable = FALSE;
        dev->CreateDepthStencilState(&ds, g_dss.GetAddressOf());

        D3D11_SAMPLER_DESC sp {};
        sp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sp.AddressU = sp.AddressV = sp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        dev->CreateSamplerState(&sp, g_samp.GetAddressOf());

        g_d3dInit = g_cb && g_blend && g_rs && g_dss && g_samp;
        return g_d3dInit;
    }
}

void MGS2DemoBlur::DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView*)
{
    if (!(eGameType & MGS2) || !bEnabled || !sceneColor) return;

    const ULONGLONG last = g_lastActMs.load(std::memory_order_relaxed);
    const bool alive = last && (GetTickCount64() - last) < 250;
    const float factor = g_factor.load(std::memory_order_relaxed);
    if (!alive)
    {
        g_havePrev = false;   // effect gone - reseed on next spawn
        return;
    }

    auto* dev = g_D3D11Hooks.d3dDevice.Get();
    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!dev || !ctx || !EnsureD3D(dev)) return;

    bool inCutscene = g_GameVars.InCutscene();
    if (bCutscenesOnly && !inCutscene)
    {
        return;
    }

    ComPtr<ID3D11Resource> colorRes;
    sceneColor->GetResource(colorRes.GetAddressOf());
    ComPtr<ID3D11Texture2D> backbuf;
    if (!colorRes || FAILED(colorRes.As(&backbuf)) || !backbuf) return;
    D3D11_TEXTURE2D_DESC bb; backbuf->GetDesc(&bb);
    if (bb.SampleDesc.Count != 1) return;

    if (!g_prevTex || g_prevW != bb.Width || g_prevH != bb.Height)
    {
        g_prevSRV.Reset(); g_prevTex.Reset(); g_havePrev = false;
        D3D11_TEXTURE2D_DESC td = bb;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE; td.Usage = D3D11_USAGE_DEFAULT;
        td.CPUAccessFlags = 0; td.MiscFlags = 0; td.SampleDesc = { 1, 0 }; td.MipLevels = 1;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, g_prevTex.GetAddressOf()))) return;
        D3D11_SHADER_RESOURCE_VIEW_DESC sv = {}; sv.Format = td.Format;
        sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; sv.Texture2D.MipLevels = 1;
        dev->CreateShaderResourceView(g_prevTex.Get(), &sv, g_prevSRV.GetAddressOf());
        g_prevW = bb.Width; g_prevH = bb.Height;
    }

    // Advance every frame; the per-step factor makes two 60Hz steps equal one authored 30fps step.
    static int  s_tickClock = -0x7fffffff;
    static bool s_skip = false;
    if (const int clock = g_GameVars.DG_Clock(); clock != s_tickClock)
    {
        s_tickClock = clock;
        s_skip = inCutscene ? !s_skip : false;
    }
    const bool advance = !s_skip;

    if (!g_havePrev)
    {
        ctx->CopyResource(g_prevTex.Get(), backbuf.Get());
        g_havePrev = true;
    }

    if (factor > 0.003f)
    {
        ID3D11RenderTargetView* oRTV[8] = {}; ID3D11DepthStencilView* oDSV = nullptr;
        ctx->OMGetRenderTargets(8, oRTV, &oDSV);
        ComPtr<ID3D11BlendState> oBlend; FLOAT oBF[4]; UINT oBM;
        ctx->OMGetBlendState(oBlend.GetAddressOf(), oBF, &oBM);
        ComPtr<ID3D11DepthStencilState> oDSS; UINT oSR;
        ctx->OMGetDepthStencilState(oDSS.GetAddressOf(), &oSR);
        ComPtr<ID3D11RasterizerState> oRS; ctx->RSGetState(oRS.GetAddressOf());
        ComPtr<ID3D11VertexShader> oVS; ComPtr<ID3D11PixelShader> oPS;
        ctx->VSGetShader(oVS.GetAddressOf(), nullptr, nullptr);
        ctx->PSGetShader(oPS.GetAddressOf(), nullptr, nullptr);
        ComPtr<ID3D11InputLayout> oIL; ctx->IAGetInputLayout(oIL.GetAddressOf());
        D3D11_PRIMITIVE_TOPOLOGY oTopo; ctx->IAGetPrimitiveTopology(&oTopo);
        ComPtr<ID3D11ShaderResourceView> oSRV; ctx->PSGetShaderResources(0, 1, oSRV.GetAddressOf());
        ComPtr<ID3D11SamplerState> oSamp; ctx->PSGetSamplers(0, 1, oSamp.GetAddressOf());
        ComPtr<ID3D11Buffer> oCB; ctx->PSGetConstantBuffers(0, 1, oCB.GetAddressOf());
        UINT oNVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_VIEWPORT oVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        ctx->RSGetViewports(&oNVP, oVP);

        {
            D3D11_MAPPED_SUBRESOURCE m;
            if (SUCCEEDED(ctx->Map(g_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m)))
            {
                const float perStep = 1.0f - sqrtf(std::max(0.0f, 1.0f - factor));
                const float cb[4] = { perStep, 0, 0, 0 };
                memcpy(m.pData, cb, sizeof(cb));
                ctx->Unmap(g_cb.Get(), 0);
            }
        }

        ID3D11ShaderResourceView* srv = g_prevSRV.Get();
        ID3D11SamplerState* smp = g_samp.Get();
        D3D11_VIEWPORT vp = { 0, 0, (float)bb.Width, (float)bb.Height, 0.f, 1.f };
        ctx->OMSetRenderTargets(1, &sceneColor, nullptr);
        ctx->OMSetBlendState(g_blend.Get(), nullptr, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_dss.Get(), 0);
        ctx->RSSetState(g_rs.Get());
        ctx->RSSetViewports(1, &vp);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_vs.Get(), nullptr, 0);
        ctx->PSSetShader(g_ps.Get(), nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &srv);
        ctx->PSSetSamplers(0, 1, &smp);
        ctx->PSSetConstantBuffers(0, 1, g_cb.GetAddressOf());
        ctx->Draw(3, 0);

        ctx->OMSetRenderTargets(8, oRTV, oDSV);
        ctx->OMSetBlendState(oBlend.Get(), oBF, oBM);
        ctx->OMSetDepthStencilState(oDSS.Get(), oSR);
        ctx->RSSetState(oRS.Get());
        if (oNVP) ctx->RSSetViewports(oNVP, oVP);
        ctx->VSSetShader(oVS.Get(), nullptr, 0);
        ctx->PSSetShader(oPS.Get(), nullptr, 0);
        ctx->IASetInputLayout(oIL.Get());
        ctx->IASetPrimitiveTopology(oTopo);
        ID3D11ShaderResourceView* rSRV = oSRV.Get(); ctx->PSSetShaderResources(0, 1, &rSRV);
        ID3D11SamplerState* rSmp = oSamp.Get();      ctx->PSSetSamplers(0, 1, &rSmp);
        ctx->PSSetConstantBuffers(0, 1, oCB.GetAddressOf());
        for (auto* r : oRTV) if (r) r->Release();
        if (oDSV) oDSV->Release();
    }

    if (advance) ctx->CopyResource(g_prevTex.Get(), backbuf.Get());
}

void MGS2DemoBlur::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled) return;

    if (uint8_t* act = Memory::PatternScan(baseModule, kActSig, "MGS 2: Demo Blur -> Act()"))
    {
        g_actHook = safetyhook::create_inline(act, reinterpret_cast<void*>(Act_Detour));
        LOG_HOOK(g_actHook, "MGS 2: Demo Blur - Act");
        SceneDepth::SetOverlayEndCallback(&MGS2DemoBlur::DrawInto);
    }
    else
    {
        spdlog::error("MGS 2: Demo Blur: Act pattern scan failed.");
    }
}
