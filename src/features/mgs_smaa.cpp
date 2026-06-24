#include "stdafx.h"
#include "mgs_smaa.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"
#include "logging.hpp"
#include "scene_depth.hpp"

#include "Textures/AreaTex.h"
#include "Textures/SearchTex.h"

static const char* kSMAAShader = R"(
#define SMAA_THRESHOLD                        0.061
#define SMAA_DEPTH_THRESHOLD                  0.010
#define SMAA_MAX_SEARCH_STEPS                 32
#define SMAA_MAX_SEARCH_STEPS_DIAG            16
#define SMAA_CORNER_ROUNDING                  25
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0
#define SMAA_REPROJECTION                     0
#define SMAA_PREDICATION                      0
#define SMAA_INCLUDE_VS                       1
#define SMAA_INCLUDE_PS                       1
#define SMAA_HLSL_4_1                         1

cbuffer SMAACB : register(b0) { float4 SMAA_RT_METRICS; }

Texture2D colorTex  : register(t0);
Texture2D edgesTex  : register(t1);
Texture2D blendTex  : register(t2);
Texture2D areaTex   : register(t3);
Texture2D searchTex : register(t4);

#include "SMAA.hlsl"

void EdgeDetectionVS(uint id : SV_VertexID,
    out float4 svPos : SV_Position, out float2 uv : TEXCOORD0, out float4 off[3] : TEXCOORD1)
{
    uv    = float2((id << 1) & 2, id & 2);
    svPos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    SMAAEdgeDetectionVS(uv, off);
}
float2 EdgeDetectionPS(float4 pos : SV_Position,
    float2 uv : TEXCOORD0, float4 off[3] : TEXCOORD1) : SV_Target
{
    return SMAAColorEdgeDetectionPS(uv, off, colorTex);
}

void BlendWeightVS(uint id : SV_VertexID,
    out float4 svPos : SV_Position, out float2 uv : TEXCOORD0,
    out float2 pixcoord : TEXCOORD1, out float4 off[3] : TEXCOORD2)
{
    uv    = float2((id << 1) & 2, id & 2);
    svPos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    SMAABlendingWeightCalculationVS(uv, pixcoord, off);
}
float4 BlendWeightPS(float4 pos : SV_Position,
    float2 uv : TEXCOORD0, float2 pixcoord : TEXCOORD1, float4 off[3] : TEXCOORD2) : SV_Target
{
    return SMAABlendingWeightCalculationPS(uv, pixcoord, off, edgesTex, areaTex, searchTex, 0);
}

void NeighborhoodVS(uint id : SV_VertexID,
    out float4 svPos : SV_Position, out float2 uv : TEXCOORD0, out float4 off : TEXCOORD1)
{
    uv    = float2((id << 1) & 2, id & 2);
    svPos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    SMAANeighborhoodBlendingVS(uv, off);
}
float4 NeighborhoodPS(float4 pos : SV_Position,
    float2 uv : TEXCOORD0, float4 off : TEXCOORD1) : SV_Target
{
    return SMAANeighborhoodBlendingPS(uv, off, colorTex, blendTex);
}

)";

struct FileInclude : ID3DInclude
{
    STDMETHOD(Open)(D3D_INCLUDE_TYPE, LPCSTR name, LPCVOID, LPCVOID* ppData, UINT* pBytes) override
    {
        auto path = (sExePath / sFixPath / name).string();
        FILE* f = nullptr;
        if (fopen_s(&f, path.c_str(), "rb") || !f) return E_FAIL;
        fseek(f, 0, SEEK_END);
        UINT sz = static_cast<UINT>(ftell(f));
        fseek(f, 0, SEEK_SET);
        void* buf = malloc(sz);
        fread(buf, 1, sz, f);
        fclose(f);
        *ppData = buf; *pBytes = sz;
        return S_OK;
    }
    STDMETHOD(Close)(LPCVOID pData) override { free(const_cast<void*>(pData)); return S_OK; }
};

// ---------------------------------------------------------------------------
// D3D11 resources
// ---------------------------------------------------------------------------
namespace
{
    ComPtr<ID3D11VertexShader>       vsEdge, vsBlend, vsNeighbor;
    ComPtr<ID3D11PixelShader>        psEdge, psBlend, psNeighbor;
    ComPtr<ID3D11Buffer>             cbSMAA;
    ComPtr<ID3D11Texture2D>          texColorCopy, texEdges, texBlend;
    ComPtr<ID3D11ShaderResourceView> srvColor, srvEdges, srvBlend, srvArea, srvSearch;
    ComPtr<ID3D11RenderTargetView>   rtvEdges, rtvBlend;
    ComPtr<ID3D11SamplerState>       sampLinear, sampPoint;
    ComPtr<ID3D11RasterizerState>    rsState;
    ComPtr<ID3D11DepthStencilState>  dssState;
    ComPtr<ID3D11BlendState>         bsOpaque;
    UINT g_width = 0, g_height = 0;

    bool CreateRTs(ID3D11Device* dev, UINT w, UINT h, DXGI_FORMAT bbFmt)
    {
        texColorCopy.Reset(); srvColor.Reset();
        texEdges.Reset();     srvEdges.Reset(); rtvEdges.Reset();
        texBlend.Reset();     srvBlend.Reset(); rtvBlend.Reset();

        auto make = [&](DXGI_FORMAT fmt, ComPtr<ID3D11Texture2D>& tex,
                        ComPtr<ID3D11ShaderResourceView>& srv,
                        ComPtr<ID3D11RenderTargetView>* rtv) -> bool
        {
            D3D11_TEXTURE2D_DESC d = {};
            d.Width = w; d.Height = h; d.MipLevels = 1; d.ArraySize = 1;
            d.Format = fmt; d.SampleDesc.Count = 1; d.Usage = D3D11_USAGE_DEFAULT;
            d.BindFlags = D3D11_BIND_SHADER_RESOURCE | (rtv ? D3D11_BIND_RENDER_TARGET : 0);
            if (FAILED(dev->CreateTexture2D(&d, nullptr, tex.GetAddressOf()))) return false;
            if (FAILED(dev->CreateShaderResourceView(tex.Get(), nullptr, srv.GetAddressOf()))) return false;
            if (rtv && FAILED(dev->CreateRenderTargetView(tex.Get(), nullptr, rtv->GetAddressOf()))) return false;
            return true;
        };

        if (!make(bbFmt,                      texColorCopy, srvColor,  nullptr))   return false;
        if (!make(DXGI_FORMAT_R8G8_UNORM,     texEdges,     srvEdges,  &rtvEdges)) return false;
        if (!make(DXGI_FORMAT_R8G8B8A8_UNORM, texBlend,     srvBlend,  &rtvBlend)) return false;

        g_width = w; g_height = h;
        return true;
    }
}

// ---------------------------------------------------------------------------
void SMAA_AA::Init()
{
    if (!(eGameType & (MGS2|MGS3)))
    {
        return;
    }
    if (!bEnabled)
    {
        return;
    }

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("SMAA: D3DCompile not found"); 
        return;
    }

    FileInclude inc;

    auto* dev = g_D3D11Hooks.d3dDevice.Get();

    auto compile = [&](const char* entry, const char* target, auto& out) -> bool
    {
        ComPtr<ID3DBlob> blob, err;
        HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kSMAAShader, strlen(kSMAAShader), "smaa_wrapper.hlsl",
                                    nullptr, &inc, entry, target, 0, 0,
                                    blob.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr)) {
            spdlog::error("SMAA '{}': {}", entry, err ? static_cast<char*>(err->GetBufferPointer()) : "unknown");
            return false;
        }
        if constexpr (std::is_same_v<std::decay_t<decltype(out)>, ComPtr<ID3D11VertexShader>>)
            dev->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out.GetAddressOf());
        else
            dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out.GetAddressOf());
        return true;
    };

    if (!compile("EdgeDetectionVS", "vs_5_0", vsEdge))     goto fail;
    if (!compile("EdgeDetectionPS", "ps_5_0", psEdge))     goto fail;
    if (!compile("BlendWeightVS",   "vs_5_0", vsBlend))    goto fail;
    if (!compile("BlendWeightPS",   "ps_5_0", psBlend))    goto fail;
    if (!compile("NeighborhoodVS",  "vs_5_0", vsNeighbor)) goto fail;
    if (!compile("NeighborhoodPS",  "ps_5_0", psNeighbor)) goto fail;

    {
        D3D11_BUFFER_DESC cbd = {};
        cbd.ByteWidth = 16; cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&cbd, nullptr, cbSMAA.GetAddressOf());
    }

    // Area texture (160x560 RG8) - from Textures/AreaTex.h
    {
        D3D11_TEXTURE2D_DESC d = {};
        d.Width = AREATEX_WIDTH; d.Height = AREATEX_HEIGHT; d.MipLevels = 1; d.ArraySize = 1;
        d.Format = DXGI_FORMAT_R8G8_UNORM; d.SampleDesc.Count = 1;
        d.Usage = D3D11_USAGE_IMMUTABLE; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA init = { areaTexBytes, AREATEX_PITCH, 0 };
        ComPtr<ID3D11Texture2D> tex;
        dev->CreateTexture2D(&d, &init, tex.GetAddressOf());
        dev->CreateShaderResourceView(tex.Get(), nullptr, srvArea.GetAddressOf());
    }

    // Search texture (64x16 R8) - from Textures/SearchTex.h
    {
        D3D11_TEXTURE2D_DESC d = {};
        d.Width = SEARCHTEX_WIDTH; d.Height = SEARCHTEX_HEIGHT; d.MipLevels = 1; d.ArraySize = 1;
        d.Format = DXGI_FORMAT_R8_UNORM; d.SampleDesc.Count = 1;
        d.Usage = D3D11_USAGE_IMMUTABLE; d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA init = { searchTexBytes, SEARCHTEX_PITCH, 0 };
        ComPtr<ID3D11Texture2D> tex;
        dev->CreateTexture2D(&d, &init, tex.GetAddressOf());
        dev->CreateShaderResourceView(tex.Get(), nullptr, srvSearch.GetAddressOf());
    }

    {
        D3D11_SAMPLER_DESC sd = {};
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        dev->CreateSamplerState(&sd, sampLinear.GetAddressOf());
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        dev->CreateSamplerState(&sd, sampPoint.GetAddressOf());
    }

    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE;
        dev->CreateRasterizerState(&rd, rsState.GetAddressOf());
    }

    {
        D3D11_DEPTH_STENCIL_DESC dsd = {};
        dev->CreateDepthStencilState(&dsd, dssState.GetAddressOf());
    }

    {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&bd, bsOpaque.GetAddressOf());
    }

    bInitialized   = true;
    spdlog::info("SMAA initialized.");

    SceneDepth::SetEndOf3DCallback(&SMAA_AA::Draw);
    return;

fail:
    spdlog::info("SMAA: shader compilation failed");
}

// ---------------------------------------------------------------------------
void SMAA_AA::Draw(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* /*depth*/)
{
    if (!bInitialized) return;

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    auto* dev = g_D3D11Hooks.d3dDevice.Get();

    // Get the texture from the scene colour RTV
    ComPtr<ID3D11Resource> res;
    sceneColor->GetResource(res.GetAddressOf());
    ComPtr<ID3D11Texture2D> bb;
    res.As(&bb);

    D3D11_TEXTURE2D_DESC bbDesc;
    bb->GetDesc(&bbDesc);

    if (bbDesc.Width != g_width || bbDesc.Height != g_height)
    {
        if (!CreateRTs(dev, bbDesc.Width, bbDesc.Height, bbDesc.Format)) {
            spdlog::error("SMAA: RT resize failed {}x{}", bbDesc.Width, bbDesc.Height);
            return;
        }
    }

    // Update CB
    {
        D3D11_MAPPED_SUBRESOURCE m;
        ctx->Map(cbSMAA.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
        float data[4] = { 1.f / bbDesc.Width, 1.f / bbDesc.Height,
                          static_cast<float>(bbDesc.Width), static_cast<float>(bbDesc.Height) };
        memcpy(m.pData, data, 16);
        ctx->Unmap(cbSMAA.Get(), 0);
    }

    // Flip-model workaround: copy backbuffer to SRV-able texture
    ctx->CopyResource(texColorCopy.Get(), bb.Get());


    D3D11_VIEWPORT vp = { 0, 0, static_cast<float>(bbDesc.Width), static_cast<float>(bbDesc.Height), 0, 1 };

    // ---- Save state ----
    ID3D11RenderTargetView*  oldRTV[8] = {};
    ID3D11DepthStencilView*  oldDSV    = nullptr;
    ID3D11BlendState*        oldBS     = nullptr;
    ID3D11DepthStencilState* oldDSS    = nullptr;
    ID3D11RasterizerState*   oldRS     = nullptr;
    ID3D11VertexShader*      oldVS     = nullptr;
    ID3D11PixelShader*       oldPS     = nullptr;
    ID3D11InputLayout*       oldIL     = nullptr;
    ID3D11Buffer*            oldCB[1]  = {};
    ID3D11ShaderResourceView* oldSRV[5] = {};
    ID3D11SamplerState*      oldSamp[2] = {};
    D3D11_PRIMITIVE_TOPOLOGY oldTopo;
    D3D11_VIEWPORT           oldVP[1]; UINT numVP = 1;
    float oldBF[4]; UINT oldBMask, oldStRef;

    ctx->OMGetRenderTargets(8, oldRTV, &oldDSV);
    ctx->OMGetBlendState(&oldBS, oldBF, &oldBMask);
    ctx->OMGetDepthStencilState(&oldDSS, &oldStRef);
    ctx->RSGetState(&oldRS);
    ctx->RSGetViewports(&numVP, oldVP);
    ctx->VSGetShader(&oldVS, nullptr, nullptr);
    ctx->PSGetShader(&oldPS, nullptr, nullptr);
    ctx->IAGetInputLayout(&oldIL);
    ctx->IAGetPrimitiveTopology(&oldTopo);
    ctx->VSGetConstantBuffers(0, 1, oldCB);
    ctx->PSGetShaderResources(0, 5, oldSRV);
    ctx->PSGetSamplers(0, 2, oldSamp);

    // ---- Common state ----
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->RSSetState(rsState.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(dssState.Get(), 0);
    ctx->OMSetBlendState(bsOpaque.Get(), nullptr, 0xFFFFFFFF);
    ctx->VSSetConstantBuffers(0, 1, cbSMAA.GetAddressOf());
    ctx->PSSetConstantBuffers(0, 1, cbSMAA.GetAddressOf());
    ID3D11SamplerState* samplers[2] = { sampLinear.Get(), sampPoint.Get() };
    ctx->PSSetSamplers(0, 2, samplers);

    // ---- Pass 1: edge detection ----
    {
        const float clear[4] = {};
        ctx->ClearRenderTargetView(rtvEdges.Get(), clear);
        ctx->OMSetRenderTargets(1, rtvEdges.GetAddressOf(), nullptr);
        ctx->VSSetShader(vsEdge.Get(), nullptr, 0);
        ctx->PSSetShader(psEdge.Get(), nullptr, 0);
        ctx->PSSetShaderResources(0, 1, srvColor.GetAddressOf());
        ctx->Draw(3, 0);
    }

    // ---- Pass 2: blend weights ----
    {
        const float clear[4] = {};
        ctx->ClearRenderTargetView(rtvBlend.Get(), clear);
        ctx->OMSetRenderTargets(1, rtvBlend.GetAddressOf(), nullptr);
        ctx->VSSetShader(vsBlend.Get(), nullptr, 0);
        ctx->PSSetShader(psBlend.Get(), nullptr, 0);
        ID3D11ShaderResourceView* srvs[5] = { nullptr, srvEdges.Get(), nullptr, srvArea.Get(), srvSearch.Get() };
        ctx->PSSetShaderResources(0, 5, srvs);
        ctx->Draw(3, 0);
    }

    // ---- Pass 3: neighborhood blending -> backbuffer ----
    {
        ctx->OMSetRenderTargets(1, &sceneColor, nullptr);
        ctx->VSSetShader(vsNeighbor.Get(), nullptr, 0);
        ctx->PSSetShader(psNeighbor.Get(), nullptr, 0);
        ID3D11ShaderResourceView* srvs[3] = { srvColor.Get(), srvEdges.Get(), srvBlend.Get() };
        ctx->PSSetShaderResources(0, 3, srvs);
        ctx->Draw(3, 0);
    }

    // ---- Restore state ----
    ctx->OMSetRenderTargets(8, oldRTV, oldDSV);
    ctx->OMSetBlendState(oldBS, oldBF, oldBMask);
    ctx->OMSetDepthStencilState(oldDSS, oldStRef);
    ctx->RSSetState(oldRS);
    ctx->RSSetViewports(numVP, oldVP);
    ctx->VSSetShader(oldVS, nullptr, 0);
    ctx->PSSetShader(oldPS, nullptr, 0);
    ctx->IASetInputLayout(oldIL);
    ctx->IASetPrimitiveTopology(oldTopo);
    ctx->VSSetConstantBuffers(0, 1, oldCB);
    ctx->PSSetShaderResources(0, 5, oldSRV);
    ctx->PSSetSamplers(0, 2, oldSamp);

    for (auto* r : oldRTV)  if (r) r->Release();
    if (oldDSV)  oldDSV->Release();
    if (oldBS)   oldBS->Release();
    if (oldDSS)  oldDSS->Release();
    if (oldRS)   oldRS->Release();
    if (oldVS)   oldVS->Release();
    if (oldPS)   oldPS->Release();
    if (oldIL)   oldIL->Release();
    if (oldCB[0])  oldCB[0]->Release();
    for (auto* r : oldSRV)  if (r) r->Release();
    for (auto* r : oldSamp) if (r) r->Release();
}
