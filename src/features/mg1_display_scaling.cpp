// ReSharper disable CommentTypo
#include "stdafx.h"

#include "mg1_display_scaling.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "input_handler.hpp"

#include "logging.hpp"

namespace
{

    bool bEnabled = true;

    //4:3 = scaleX = 1.10416667f x 1.0f;
    //4:3 fullscreened = 1.25f x 1.13207547f
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    constexpr float posX = 0.5f;
    constexpr float posY = 0.49953704f;

    const char* kDisplayModShader = R"(
    cbuffer CB : register(b0)
    {
        float Display_ScaleX;
        float Display_ScaleY;
        float Display_PosX;
        float Display_PosY;
        float BufferWidth;
        float BufferHeight;
        float pad0;
        float pad1;
    }

    Texture2D    BackBufferTex : register(t0);
    SamplerState BackBufferSmp : register(s0);

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD)
    {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    }

    float4 PS(float4 pos : SV_Position, float2 texCoord : TEXCOORD) : SV_Target
    {
        const float3 pivot = float3(0.5, 0.5, 0.0);
        const float3 mulUV = float3(texCoord.x, texCoord.y, 1.0);

        const float ScaleX = BufferWidth  * Display_ScaleX;
        const float ScaleY = BufferHeight * Display_ScaleY;

        const float3x3 positionMatrix = float3x3(
             1,             0,            0,
             0,             1,            0,
            -Display_PosX, -Display_PosY, 1
        );
        const float3x3 scaleMatrix = float3x3(
            1.0 / ScaleX, 0,            0,
            0,            1.0 / ScaleY, 0,
            0,            0,            1
        );

        const float3 SumUV = mul(
            mul(mulUV, positionMatrix) * float3(BufferWidth, BufferHeight, 1.0),
            scaleMatrix);

        const float2 sampleUV = SumUV.rg + pivot.rg;
        const bool   inBounds = all(sampleUV == saturate(sampleUV));
        return inBounds ? BackBufferTex.Sample(BackBufferSmp, sampleUV) : float4(0, 0, 0, 1);
    }
    )";

    ComPtr<ID3DBlob>                 vsBlob;
    ComPtr<ID3DBlob>                 psBlob;
    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11Buffer>             cb;
    ComPtr<ID3D11SamplerState>       smp;
    ComPtr<ID3D11RasterizerState>    rs;
    ComPtr<ID3D11DepthStencilState>  dss;
    ComPtr<ID3D11BlendState>         blendOpaque;
    ComPtr<ID3D11Texture2D>          copyTex;
    ComPtr<ID3D11ShaderResourceView> copySRV;

    bool bShaderLoaded = false;
    UINT copyWidth     = 0;
    UINT copyHeight    = 0;

    struct alignas(16) CBLayout
    {
        float scaleX;
        float scaleY;
        float posX;
        float posY;
        float bufferWidth;
        float bufferHeight;
        float pad0;
        float pad1;
    };

    void UpdateCB(ID3D11DeviceContext* ctx, float w, float h)
    {
        D3D11_MAPPED_SUBRESOURCE m;
        ctx->Map(cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
        auto* d        = static_cast<CBLayout*>(m.pData);
        d->scaleX      = scaleX;
        d->scaleY      = scaleY;
        d->posX        = posX;
        d->posY        = posY;
        d->bufferWidth  = w;
        d->bufferHeight = h;
        d->pad0 = d->pad1 = 0.f;
        ctx->Unmap(cb.Get(), 0);
    }

    bool EnsureCopyTex(ID3D11Device* dev, UINT w, UINT h, DXGI_FORMAT fmt)
    {
        if (copyTex && w == copyWidth && h == copyHeight)
            return true;

        copyTex.Reset();
        copySRV.Reset();

        D3D11_TEXTURE2D_DESC td = {};
        td.Width            = w;
        td.Height           = h;
        td.MipLevels        = 1;
        td.ArraySize        = 1;
        td.Format           = fmt;
        td.SampleDesc.Count = 1;
        td.Usage            = D3D11_USAGE_DEFAULT;
        td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(dev->CreateTexture2D(&td, nullptr, copyTex.GetAddressOf())))
        {
            spdlog::error("MG1_DisplayScaling: Failed to create backbuffer copy texture");
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format                    = fmt;
        sd.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels       = 1;

        if (FAILED(dev->CreateShaderResourceView(copyTex.Get(), &sd, copySRV.GetAddressOf())))
        {
            spdlog::error("MG1_DisplayScaling: Failed to create backbuffer copy SRV");
            copyTex.Reset();
            return false;
        }

        copyWidth  = w;
        copyHeight = h;
        return true;
    }



}

void MG1_DisplayScaling::Setup()
{
    if (!(eGameType & MG))
    {
        return;
    }

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("MG1_DisplayScaling: Failed to get D3DCompile");
        return;
    }

    spdlog::info("MG1_DisplayScaling: Compiling shader...");

    ComPtr<ID3DBlob> err;

    HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kDisplayModShader, strlen(kDisplayModShader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MG1_DisplayScaling: Failed to compile vertex shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        return;
    }

    err.Reset();

    hr = g_D3D11Hooks.D3DCompileFunc(kDisplayModShader, strlen(kDisplayModShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MG1_DisplayScaling: Failed to compile pixel shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        vsBlob.Reset();
        return;
    }

    spdlog::info("MG1_DisplayScaling: Shader compiled successfully");

    if (bCropBorders && bCorrectTo4x3)
    {
        scaleX = 1.25f;
        scaleY = 1.13207547f;
        spdlog::info("MG1_DisplayScaling: Cropping borders & correcting to 4:3");
    }
    else if (bCropBorders)
    {
        scaleX = scaleY = 1.13207547f;
        spdlog::info("MG1_DisplayScaling: Cropping borders");
    }
    else if (bCorrectTo4x3)
    {
        scaleX = 1.10416667f;
        spdlog::info("MG1_DisplayScaling: Correcting to 4:3");
    }
    else
    {
        spdlog::info("MG1_DisplayScaling: Correcting viewport positioning");
    }
    
}

void MG1_DisplayScaling::Init()
{
    if (!(eGameType & MG))
    {
        return;
    }

    if (bShaderLoaded)
        return;

    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!dev)
    {
        spdlog::error("MG1_DisplayScaling: D3D11 device is not initialized");
        return;
    }

    if (!vsBlob || !psBlob)
    {
        spdlog::error("MG1_DisplayScaling: Shader bytecode was not compiled");
        return;
    }

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf());
    dev->CreatePixelShader(psBlob->GetBufferPointer(),  psBlob->GetBufferSize(),  nullptr, ps.GetAddressOf());

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth      = sizeof(CBLayout);
    cbd.Usage          = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&cbd, nullptr, cb.GetAddressOf());

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MaxLOD         = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&sd, smp.GetAddressOf());

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    dev->CreateRasterizerState(&rd, rs.GetAddressOf());

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable    = FALSE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dev->CreateDepthStencilState(&dsd, dss.GetAddressOf());

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable           = FALSE;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&bd, blendOpaque.GetAddressOf());

    vsBlob.Reset();
    psBlob.Reset();

    bShaderLoaded = true;
    spdlog::info("MG1_DisplayScaling initialized.");

    /*
    const auto apply = []()
        {
            scaleX = std::clamp(scaleX, 0.001f, 5.0f);
            scaleY = std::clamp(scaleY, 0.001f, 5.0f);
            posX = std::clamp(posX, -2.0f, 2.0f);
            posY = std::clamp(posY, -2.0f, 2.0f);

            spdlog::info("DisplayMod: scaleX={:.8f} scaleY={:.8f} posX={:.8f} posY={:.8f}",
                         scaleX, scaleY, posX, posY);
        };

        // Numpad 7/9 : scaleX -/+
        // Numpad 1/3 : scaleY -/+
        // Numpad 4/6 : posX -/+
        // Numpad 8/2 : posY -/+
        // Numpad 5   : reset


    static const float stepX = 1.0f / static_cast<float>(CustomResolutionAndBorderless::iOutputResX);
    static const float stepY = 1.0f / static_cast<float>(CustomResolutionAndBorderless::iOutputResY);
    g_InputHandler.RegisterHotkey(VK_NUMPAD1, "display scaleX-", [apply]() { scaleX -= stepX; apply(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD3, "display scaleX+", [apply]() { scaleX += stepX; apply(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD2, "display scaleY-", [apply]() { scaleY -= stepY; apply(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD5, "display scaleY+", [apply]() { scaleY += stepY; apply(); });

    g_InputHandler.RegisterHotkey(VK_NUMPAD7, "display posX-", [apply]() { posX -= stepX; apply(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD9, "display posX+", [apply]() { posX += stepX; apply(); });
    g_InputHandler.RegisterHotkey(VK_DIVIDE, "display posY-", [apply]() { posY -= stepY; apply(); });
    g_InputHandler.RegisterHotkey(VK_NUMPAD8, "display posY+", [apply]() { posY += stepY; apply(); });

    g_InputHandler.RegisterHotkey(VK_NUMPAD0, "display reset", [apply]()
                                      {
                                          scaleX = 1.131f;
                                          scaleY = 1.130f;
                                          posX = 0.5f;
                                          posY = 0.49953705f;
                                          apply();
                                      });
                                      */
}

void MG1_DisplayScaling::Draw(IDXGISwapChain* swap)
{
    if (!bShaderLoaded)
        return;

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    auto* dev = g_D3D11Hooks.d3dDevice.Get();

    ComPtr<ID3D11Texture2D> backbuf;
    swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuf.GetAddressOf());

    D3D11_TEXTURE2D_DESC bbDesc;
    backbuf->GetDesc(&bbDesc);

    if (!EnsureCopyTex(dev, bbDesc.Width, bbDesc.Height, bbDesc.Format))
        return;

    ctx->CopyResource(copyTex.Get(), backbuf.Get());

    ComPtr<ID3D11RenderTargetView> rtv;
    dev->CreateRenderTargetView(backbuf.Get(), nullptr, rtv.GetAddressOf());

    D3D11_VIEWPORT vp = { 0, 0, (float)bbDesc.Width, (float)bbDesc.Height, 0.f, 1.f };

    UpdateCB(ctx, (float)bbDesc.Width, (float)bbDesc.Height);

    // Save state
    ID3D11RenderTargetView*   oldRTV[8]  = {};
    ID3D11DepthStencilView*   oldDSV     = nullptr;
    ID3D11BlendState*         oldBlend   = nullptr;
    ID3D11DepthStencilState*  oldDSS     = nullptr;
    ID3D11RasterizerState*    oldRS      = nullptr;
    ID3D11VertexShader*       oldVS      = nullptr;
    ID3D11PixelShader*        oldPS      = nullptr;
    ID3D11InputLayout*        oldIL      = nullptr;
    ID3D11Buffer*             oldPSCB[1] = {};
    ID3D11Buffer*             oldVB[1]   = {};
    ID3D11ShaderResourceView* oldSRV[1]  = {};
    ID3D11SamplerState*       oldSmp[1]  = {};
    D3D11_PRIMITIVE_TOPOLOGY  oldTopology;
    D3D11_VIEWPORT            oldVP[1]   = {};
    UINT oldBlendMask = 0, oldStencilRef = 0, oldStride = 0, oldOffset = 0, numVP = 1;
    float oldBlendFactor[4] = {};

    ctx->OMGetRenderTargets(8, oldRTV, &oldDSV);
    ctx->OMGetBlendState(&oldBlend, oldBlendFactor, &oldBlendMask);
    ctx->OMGetDepthStencilState(&oldDSS, &oldStencilRef);
    ctx->RSGetState(&oldRS);
    ctx->RSGetViewports(&numVP, oldVP);
    ctx->VSGetShader(&oldVS, nullptr, nullptr);
    ctx->PSGetShader(&oldPS, nullptr, nullptr);
    ctx->PSGetConstantBuffers(0, 1, oldPSCB);
    ctx->PSGetShaderResources(0, 1, oldSRV);
    ctx->PSGetSamplers(0, 1, oldSmp);
    ctx->IAGetInputLayout(&oldIL);
    ctx->IAGetPrimitiveTopology(&oldTopology);
    ctx->IAGetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);

    // Set pipeline
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, cb.GetAddressOf());
    ctx->PSSetShaderResources(0, 1, copySRV.GetAddressOf());
    ctx->PSSetSamplers(0, 1, smp.GetAddressOf());
    ctx->RSSetState(rs.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(dss.Get(), 0);
    ctx->OMSetBlendState(blendOpaque.Get(), nullptr, 0xFFFFFFFF);
    ctx->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

    ctx->Draw(3, 0);

    // Restore state
    ctx->OMSetRenderTargets(8, oldRTV, oldDSV);
    ctx->OMSetBlendState(oldBlend, oldBlendFactor, oldBlendMask);
    ctx->OMSetDepthStencilState(oldDSS, oldStencilRef);
    ctx->RSSetState(oldRS);
    ctx->RSSetViewports(numVP, oldVP);
    ctx->VSSetShader(oldVS, nullptr, 0);
    ctx->PSSetShader(oldPS, nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, oldPSCB);
    ctx->PSSetShaderResources(0, 1, oldSRV);
    ctx->PSSetSamplers(0, 1, oldSmp);
    ctx->IASetInputLayout(oldIL);
    ctx->IASetPrimitiveTopology(oldTopology);
    ctx->IASetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);

    // Release refs added by GetXxx
    for (auto* r : oldRTV) if (r) r->Release();
    if (oldDSV)     oldDSV->Release();
    if (oldBlend)   oldBlend->Release();
    if (oldDSS)     oldDSS->Release();
    if (oldRS)      oldRS->Release();
    if (oldVS)      oldVS->Release();
    if (oldPS)      oldPS->Release();
    if (oldPSCB[0]) oldPSCB[0]->Release();
    if (oldSRV[0])  oldSRV[0]->Release();
    if (oldSmp[0])  oldSmp[0]->Release();
    if (oldIL)      oldIL->Release();
    if (oldVB[0])   oldVB[0]->Release();
}
