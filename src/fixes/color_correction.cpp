// ReSharper disable CommentTypo
#include "stdafx.h"

#include "color_correction.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"
#include "logging.hpp"

namespace
{
    // Vibrance + Curves based off CeeJayDK's SweetFX shaders.
    // Intelligently boosts saturation: undersaturated pixels get a larger boost
    // than pixels that are already vivid, avoiding oversaturation.
    // then apply a gentle S-curve to luma for a touch more contrast without clipping.
    const char* kShader = R"(
    Texture2D    backBuffer : register(t0);
    SamplerState smpl       : register(s0);

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TexCoord)
    {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    }

    float4 PS(float4 pos : SV_Position, float2 uv : TexCoord) : SV_Target
    {
        float3 color = backBuffer.Sample(smpl, uv).rgb;

        static const float3 coefLuma = float3(0.212656, 0.715158, 0.072186);

        // Vibrance
        const float kVibrance = 0.15;
        float luma             = dot(coefLuma, color);
        float max_color        = max(color.r, max(color.g, color.b));
        float min_color        = min(color.r, min(color.g, color.b));
        float color_saturation = max_color - min_color;

        color = lerp(luma, color, 1.0 + (kVibrance * (1.0 - (sign(kVibrance) * color_saturation))));

        // Gamma Curve
        const float kContrast = 0.225;
        luma = dot(coefLuma, color);                    // recompute post-vibrance
        float3 chroma = color - luma;
        float s = luma;
        s = s * s * s * (s * (s * 6.0 - 15.0) + 10.0);  // Perlin's smootherstep
        luma  = lerp(luma, s, kContrast);               // blend by contrast
        color = luma + chroma;

        return float4(color, 1.0);
    }
    )";

    ComPtr<ID3DBlob>                 vsBlob;
    ComPtr<ID3DBlob>                 psBlob;
    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11SamplerState>       smpl;
    ComPtr<ID3D11Texture2D>          tempTex;
    ComPtr<ID3D11ShaderResourceView> tempSRV;
    ComPtr<ID3D11RasterizerState>    rs;
    ComPtr<ID3D11DepthStencilState>  dss;
    D3D11_TEXTURE2D_DESC             tempDesc = {};
}

void ColorCorrection::Setup()
{
    if (!(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("Gamma Correction: Disabled in config, skipping shader compilation.");
        return;
    }

    spdlog::info("Gamma Correction: Compiling shaders...");


    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("Gamma Correction: Failed to get D3DCompile");
        return;
    }

    ComPtr<ID3DBlob> err;

    HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("Gamma Correction: VS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
        return;
    }

    err.Reset();

    hr = g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("Gamma Correction: PS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
        vsBlob.Reset();
        return;
    }

    spdlog::info("Gamma Correction: Shaders compiled successfully");
}

void ColorCorrection::Init()
{
    if (!(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    if (!bEnabled)
    {
        return;
    }

    if (bShaderLoaded)
    {
        return;
    }

    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!dev)
    {
        spdlog::error("Gamma Correction: D3D11 device not initialized");
        return;
    }

    if (!vsBlob || !psBlob)
    {
        spdlog::error("Gamma Correction: Shader bytecode missing");
        return;
    }

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf());
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf());

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    dev->CreateSamplerState(&sd, smpl.GetAddressOf());

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    dev->CreateRasterizerState(&rd, rs.GetAddressOf());

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = FALSE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dev->CreateDepthStencilState(&dsd, dss.GetAddressOf());

    vsBlob.Reset();
    psBlob.Reset();

    bShaderLoaded = true;
    spdlog::info("Gamma Correction: Initialized");
}

void ColorCorrection::Draw(IDXGISwapChain* swap)
{
    if (!bShaderLoaded)
        return;

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    auto* dev = g_D3D11Hooks.d3dDevice.Get();

    ComPtr<ID3D11Texture2D> backbuf;
    swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuf.GetAddressOf());

    D3D11_TEXTURE2D_DESC bbDesc;
    backbuf->GetDesc(&bbDesc);

    // Recreate the read-copy if the backbuffer dimensions changed
    if (!tempTex || tempDesc.Width != bbDesc.Width || tempDesc.Height != bbDesc.Height)
    {
        tempTex.Reset();
        tempSRV.Reset();

        D3D11_TEXTURE2D_DESC td = bbDesc;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.CPUAccessFlags = 0;
        td.MiscFlags = 0;
        dev->CreateTexture2D(&td, nullptr, tempTex.GetAddressOf());
        dev->CreateShaderResourceView(tempTex.Get(), nullptr, tempSRV.GetAddressOf());
        tempDesc = bbDesc;
    }

    ctx->CopyResource(tempTex.Get(), backbuf.Get());

    ComPtr<ID3D11RenderTargetView> rtv;
    dev->CreateRenderTargetView(backbuf.Get(), nullptr, rtv.GetAddressOf());

    D3D11_VIEWPORT vp = { 0.f, 0.f, (float)bbDesc.Width, (float)bbDesc.Height, 0.f, 1.f };

    // save state
    ID3D11RenderTargetView* oldRTV[8] = {};
    ID3D11DepthStencilView* oldDSV = nullptr;
    ID3D11BlendState* oldBlend = nullptr;
    ID3D11DepthStencilState* oldDSS = nullptr;
    ID3D11RasterizerState* oldRS = nullptr;
    ID3D11VertexShader* oldVS = nullptr;
    ID3D11PixelShader* oldPS = nullptr;
    ID3D11InputLayout* oldIL = nullptr;
    ID3D11Buffer* oldVB[1] = {};
    ID3D11ShaderResourceView* oldSRV[1] = {};
    ID3D11SamplerState* oldSmpl[1] = {};
    D3D11_PRIMITIVE_TOPOLOGY oldTopology;
    D3D11_VIEWPORT           oldVP[1] = {};
    UINT oldBlendMask = 0, oldStencilRef = 0, oldStride = 0, oldOffset = 0, numVP = 1;
    float oldBlendFactor[4] = {};

    ctx->OMGetRenderTargets(8, oldRTV, &oldDSV);
    ctx->OMGetBlendState(&oldBlend, oldBlendFactor, &oldBlendMask);
    ctx->OMGetDepthStencilState(&oldDSS, &oldStencilRef);
    ctx->RSGetState(&oldRS);
    ctx->RSGetViewports(&numVP, oldVP);
    ctx->VSGetShader(&oldVS, nullptr, nullptr);
    ctx->PSGetShader(&oldPS, nullptr, nullptr);
    ctx->IAGetInputLayout(&oldIL);
    ctx->IAGetPrimitiveTopology(&oldTopology);
    ctx->IAGetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);
    ctx->PSGetShaderResources(0, 1, oldSRV);
    ctx->PSGetSamplers(0, 1, oldSmpl);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);
    ctx->PSSetShaderResources(0, 1, tempSRV.GetAddressOf());
    ctx->PSSetSamplers(0, 1, smpl.GetAddressOf());
    ctx->RSSetState(rs.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(dss.Get(), 0);
    ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    ctx->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

    ctx->Draw(3, 0);

    // restore state
    ctx->OMSetRenderTargets(8, oldRTV, oldDSV);
    ctx->OMSetBlendState(oldBlend, oldBlendFactor, oldBlendMask);
    ctx->OMSetDepthStencilState(oldDSS, oldStencilRef);
    ctx->RSSetState(oldRS);
    ctx->RSSetViewports(numVP, oldVP);
    ctx->VSSetShader(oldVS, nullptr, 0);
    ctx->PSSetShader(oldPS, nullptr, 0);
    ctx->IASetInputLayout(oldIL);
    ctx->IASetPrimitiveTopology(oldTopology);
    ctx->IASetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);
    ctx->PSSetShaderResources(0, 1, oldSRV);
    ctx->PSSetSamplers(0, 1, oldSmpl);

    for (auto* r : oldRTV) if (r) r->Release();
    if (oldDSV)     oldDSV->Release();
    if (oldBlend)   oldBlend->Release();
    if (oldDSS)     oldDSS->Release();
    if (oldRS)      oldRS->Release();
    if (oldVS)      oldVS->Release();
    if (oldPS)      oldPS->Release();
    if (oldIL)      oldIL->Release();
    if (oldVB[0])   oldVB[0]->Release();
    if (oldSRV[0])  oldSRV[0]->Release();
    if (oldSmpl[0]) oldSmpl[0]->Release();
}
