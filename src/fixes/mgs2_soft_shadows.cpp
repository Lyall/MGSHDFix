#include "stdafx.h"
#include "mgs2_soft_shadows.hpp"

#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"

namespace
{
    // The game's blur pass ends up as a plain copy on D3D11, so do a real one.
    constexpr int   kDownShift = 1;
    constexpr float kTapTexels = 0.75f;

    ComPtr<ID3D11VertexShader>      g_vs;
    ComPtr<ID3D11PixelShader>       g_ps;
    ComPtr<ID3D11SamplerState>      g_sampler;
    ComPtr<ID3D11BlendState>        g_blend;
    ComPtr<ID3D11DepthStencilState> g_dss;
    ComPtr<ID3D11RasterizerState>   g_rs;
    ComPtr<ID3D11Buffer>            g_cb;
    bool g_failed = false;

    struct Level
    {
        ComPtr<ID3D11Texture2D>          tex;
        ComPtr<ID3D11RenderTargetView>   rtv;
        ComPtr<ID3D11ShaderResourceView> srv;
        UINT w = 0, h = 0;
    };
    constexpr int kPing = kDownShift;
    constexpr int kChain = kDownShift + 1;
    Level g_chain[kChain];

    ComPtr<ID3D11RenderTargetView>   g_targetRTV;
    ComPtr<ID3D11ShaderResourceView> g_targetSRV;
    ID3D11Texture2D* g_viewsFor = nullptr;
    UINT g_targetW = 0, g_targetH = 0;

    ComPtr<ID3D11Texture2D> g_blurTarget;   // keep a ref, the game frees these
    UINT g_blurDim = 0;
    UINT g_biggestDim = 0;
    bool g_pending = false;
    bool g_inBlur = false;                  // our own passes come back through the hook

    const char* kBlurShader = R"(
    Texture2D    src : register(t0);
    SamplerState smp : register(s0);
    cbuffer Params : register(b0) { float4 gDir; };

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0)
    {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    }

    float4 PS(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target
    {
        float2 d = gDir.xy;
        if (d.x == 0.0 && d.y == 0.0) return src.Sample(smp, uv);

        float4 c = src.Sample(smp, uv) * 0.2270270270;
        c += (src.Sample(smp, uv + d) + src.Sample(smp, uv - d)) * 0.1945945946;
        c += (src.Sample(smp, uv + d * 2.0) + src.Sample(smp, uv - d * 2.0)) * 0.1216216216;
        c += (src.Sample(smp, uv + d * 3.0) + src.Sample(smp, uv - d * 3.0)) * 0.0540540541;
        c += (src.Sample(smp, uv + d * 4.0) + src.Sample(smp, uv - d * 4.0)) * 0.0162162162;
        return c;
    }
    )";

    bool EnsureResources(ID3D11Device* dev)
    {
        if (g_failed) return false;
        if (g_vs && g_ps) return true;
        if (!g_D3D11Hooks.D3DCompileFunc) { g_failed = true; return false; }

        ComPtr<ID3DBlob> vsb, psb, err;
        bool ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kBlurShader, strlen(kBlurShader), nullptr, nullptr, nullptr,
                      "VS", "vs_5_0", 0, 0, vsb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (ok) ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kBlurShader, strlen(kBlurShader), nullptr, nullptr, nullptr,
                      "PS", "ps_5_0", 0, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (!ok)
        {
            spdlog::error("MGS2: Soft Shadows - blur shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
            g_failed = true;
            return false;
        }
        if (FAILED(dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, g_vs.GetAddressOf())) ||
            FAILED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, g_ps.GetAddressOf())))
        {
            g_failed = true;
            return false;
        }

        D3D11_SAMPLER_DESC sd {};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        dev->CreateSamplerState(&sd, g_sampler.GetAddressOf());

        D3D11_BLEND_DESC bd {};
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&bd, g_blend.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC dd {};
        dev->CreateDepthStencilState(&dd, g_dss.GetAddressOf());

        D3D11_RASTERIZER_DESC rd {};
        rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = FALSE;
        dev->CreateRasterizerState(&rd, g_rs.GetAddressOf());

        D3D11_BUFFER_DESC cbd {};
        cbd.ByteWidth = 16;
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&cbd, nullptr, g_cb.GetAddressOf());

        if (!g_sampler || !g_blend || !g_dss || !g_rs || !g_cb) { g_failed = true; return false; }
        return true;
    }

    bool EnsureViews(ID3D11Device* dev, ID3D11Texture2D* target)
    {
        if (g_viewsFor == target && g_targetRTV) return true;

        for (auto& c : g_chain) { c.tex.Reset(); c.rtv.Reset(); c.srv.Reset(); }
        g_targetRTV.Reset(); g_targetSRV.Reset();
        g_viewsFor = nullptr;

        D3D11_TEXTURE2D_DESC d {};
        target->GetDesc(&d);
        if (FAILED(dev->CreateRenderTargetView(target, nullptr, g_targetRTV.GetAddressOf()))) return false;
        if (FAILED(dev->CreateShaderResourceView(target, nullptr, g_targetSRV.GetAddressOf()))) return false;

        UINT w = d.Width, h = d.Height;
        for (int i = 0; i < kChain; i++)
        {
            if (i < kDownShift)
            {
                w = (w > 1) ? w / 2 : 1;
                h = (h > 1) ? h / 2 : 1;
            }

            D3D11_TEXTURE2D_DESC sd = d;
            sd.Width = w; sd.Height = h;
            sd.MipLevels = 1; sd.ArraySize = 1;
            sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
            sd.Usage = D3D11_USAGE_DEFAULT;
            sd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            sd.CPUAccessFlags = 0; sd.MiscFlags = 0;
            if (FAILED(dev->CreateTexture2D(&sd, nullptr, g_chain[i].tex.GetAddressOf()))) return false;
            if (FAILED(dev->CreateRenderTargetView(g_chain[i].tex.Get(), nullptr, g_chain[i].rtv.GetAddressOf()))) return false;
            if (FAILED(dev->CreateShaderResourceView(g_chain[i].tex.Get(), nullptr, g_chain[i].srv.GetAddressOf()))) return false;
            g_chain[i].w = w; g_chain[i].h = h;
        }

        g_targetW = d.Width; g_targetH = d.Height;
        g_viewsFor = target;
        spdlog::info("MGS2: Soft Shadows - {}x{} projector shadow, blurring at {}x{}",
            d.Width, d.Height, g_chain[kDownShift - 1].w, g_chain[kDownShift - 1].h);
        return true;
    }

    void Pass(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, ID3D11ShaderResourceView* srv,
              UINT w, UINT h, float dx, float dy)
    {
        D3D11_MAPPED_SUBRESOURCE m {};
        if (SUCCEEDED(ctx->Map(g_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m)))
        {
            const float v[4] = { dx, dy, 0.0f, 0.0f };
            memcpy(m.pData, v, sizeof(v));
            ctx->Unmap(g_cb.Get(), 0);
        }

        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(0, 1, &nullSRV);
        ctx->OMSetRenderTargets(1, &rtv, nullptr);

        D3D11_VIEWPORT vp {};
        vp.Width = static_cast<float>(w);
        vp.Height = static_cast<float>(h);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);

        ctx->PSSetShaderResources(0, 1, &srv);
        ctx->Draw(3, 0);
    }

    void Blur(ID3D11DeviceContext* ctx, ID3D11Texture2D* target)
    {
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev || !EnsureResources(dev) || !EnsureViews(dev, target)) return;

        ComPtr<ID3D11RenderTargetView> oRTV; ComPtr<ID3D11DepthStencilView> oDSV;
        ctx->OMGetRenderTargets(1, oRTV.GetAddressOf(), oDSV.GetAddressOf());
        ComPtr<ID3D11VertexShader> oVS; ComPtr<ID3D11PixelShader> oPS;
        ctx->VSGetShader(oVS.GetAddressOf(), nullptr, nullptr);
        ctx->PSGetShader(oPS.GetAddressOf(), nullptr, nullptr);
        ComPtr<ID3D11InputLayout> oIL; ctx->IAGetInputLayout(oIL.GetAddressOf());
        D3D11_PRIMITIVE_TOPOLOGY oTopo; ctx->IAGetPrimitiveTopology(&oTopo);
        ComPtr<ID3D11ShaderResourceView> oSRV; ctx->PSGetShaderResources(0, 1, oSRV.GetAddressOf());
        ComPtr<ID3D11SamplerState> oSmp; ctx->PSGetSamplers(0, 1, oSmp.GetAddressOf());
        ComPtr<ID3D11Buffer> oCB; ctx->PSGetConstantBuffers(0, 1, oCB.GetAddressOf());
        ComPtr<ID3D11BlendState> oBS; float oBF[4]; UINT oMask;
        ctx->OMGetBlendState(oBS.GetAddressOf(), oBF, &oMask);
        ComPtr<ID3D11DepthStencilState> oDSS; UINT oSR; ctx->OMGetDepthStencilState(oDSS.GetAddressOf(), &oSR);
        ComPtr<ID3D11RasterizerState> oRS; ctx->RSGetState(oRS.GetAddressOf());
        UINT oNVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_VIEWPORT oVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] {};
        ctx->RSGetViewports(&oNVP, oVP);

        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_vs.Get(), nullptr, 0);
        ctx->PSSetShader(g_ps.Get(), nullptr, 0);
        ID3D11SamplerState* smp = g_sampler.Get();
        ctx->PSSetSamplers(0, 1, &smp);
        ID3D11Buffer* cb = g_cb.Get();
        ctx->PSSetConstantBuffers(0, 1, &cb);
        const float bf[4] = { 0, 0, 0, 0 };
        ctx->OMSetBlendState(g_blend.Get(), bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_dss.Get(), 0);
        ctx->RSSetState(g_rs.Get());

        // Halving is what takes the aliasing out.
        Pass(ctx, g_chain[0].rtv.Get(), g_targetSRV.Get(), g_chain[0].w, g_chain[0].h, 0.0f, 0.0f);
        for (int i = 1; i < kDownShift; i++)
        {
            Pass(ctx, g_chain[i].rtv.Get(), g_chain[i - 1].srv.Get(), g_chain[i].w, g_chain[i].h, 0.0f, 0.0f);
        }

        Level& lo = g_chain[kDownShift - 1];
        Level& ping = g_chain[kPing];
        Pass(ctx, ping.rtv.Get(), lo.srv.Get(), ping.w, ping.h, kTapTexels / lo.w, 0.0f);
        Pass(ctx, lo.rtv.Get(), ping.srv.Get(), lo.w, lo.h, 0.0f, kTapTexels / lo.h);

        Pass(ctx, g_targetRTV.Get(), lo.srv.Get(), g_targetW, g_targetH, 0.0f, 0.0f);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        ctx->PSSetShaderResources(0, 1, &nullSRV);

        ID3D11RenderTargetView* rRTV = oRTV.Get();
        ctx->OMSetRenderTargets(1, &rRTV, oDSV.Get());
        ctx->VSSetShader(oVS.Get(), nullptr, 0);
        ctx->PSSetShader(oPS.Get(), nullptr, 0);
        ctx->IASetInputLayout(oIL.Get());
        ctx->IASetPrimitiveTopology(oTopo);
        ID3D11ShaderResourceView* rSRV = oSRV.Get();
        ctx->PSSetShaderResources(0, 1, &rSRV);
        ID3D11SamplerState* rSmp = oSmp.Get();
        ctx->PSSetSamplers(0, 1, &rSmp);
        ID3D11Buffer* rCB = oCB.Get();
        ctx->PSSetConstantBuffers(0, 1, &rCB);
        ctx->OMSetBlendState(oBS.Get(), oBF, oMask);
        ctx->OMSetDepthStencilState(oDSS.Get(), oSR);
        ctx->RSSetState(oRS.Get());
        if (oNVP) ctx->RSSetViewports(oNVP, oVP);
    }
}

void MGS2SoftShadows::NoteSquareTarget(ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, UINT dim)
{
    if (!bEnabled || g_inBlur) return;

    // The game copies its shadow into the smaller target, so that is the one to blur.
    if (dim > g_biggestDim)
    {
        g_biggestDim = dim;
        if (g_blurTarget.Get() == tex) { g_blurTarget.Reset(); g_blurDim = 0; }
    }
    else if (dim != 0 && dim < g_biggestDim && (!g_blurTarget || dim <= g_blurDim))
    {
        g_blurTarget = tex;
        g_blurDim = dim;
    }

    if (tex != nullptr && tex == g_blurTarget.Get())
    {
        g_pending = true;
    }
    else if (g_pending)
    {
        g_pending = false;
        g_inBlur = true;
        Blur(ctx, g_blurTarget.Get());
        g_inBlur = false;
    }
}

void MGS2SoftShadows::Reset()
{
    for (auto& c : g_chain) { c.tex.Reset(); c.rtv.Reset(); c.srv.Reset(); }
    g_targetRTV.Reset(); g_targetSRV.Reset();
    g_viewsFor = nullptr;
    g_blurTarget.Reset();
    g_blurDim = 0;
    g_biggestDim = 0;
    g_pending = false;
}
