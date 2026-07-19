#include "stdafx.h"
#include "mgs2_shimmer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "mgs2_shimmer_noise_map.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"
#include "gamevars.hpp"
//#include "input_handler.hpp"
#include "logging.hpp"
#include "scene_depth.hpp"

using namespace MGS2_StatusFlags;

namespace
{
    const char* kShimmerShader = R"(
    #define BLOB_SPEED 1.5  // 1.0 = PS2 original speed

    Texture2D    g_tex          : register(t0);
    Texture2D    g_noise        : register(t1);
    SamplerState g_sampler      : register(s0);
    SamplerState g_sampler_point : register(s1);

    cbuffer CB : register(b0) {
        float  time;
        float2 resolution;
        float  topAlpha;
        float  bottomAlpha;
        float2 drift;
        float  _pad;
    }

    float hash1(float n) 
    { 
        return frac(sin(n * 127.1) * 43758.5453); 
    }

    float BlobMask(float2 uv, float time, float2 drift, float2 res)
    {
        float2 uvPx       = uv * res;
        float  blobRadius = 96.0 * res.x / 512.0;           // 96 = PS2 sprite size in px
        float  mask       = 0;

        for (int i = 0; i < 32; i++)
        {
            float fi = float(i);
            float sx = (hash1(fi + 0.1) * 90.0 + 60.0) * BLOB_SPEED * (hash1(fi + 11.3) > 0.5 ? 1.0 : -1.0);
            float sy = (hash1(fi + 7.2) * 90.0 + 60.0) * BLOB_SPEED * (hash1(fi + 23.7) > 0.5 ? 1.0 : -1.0);

            float2 blobPos;
            blobPos.x = frac(hash1(fi + 3.1) + sx * time / 512.0 + drift.x / 512.0) * res.x;
            blobPos.y = frac(hash1(fi + 5.9) + sy * time / 448.0 + drift.y / 448.0) * res.y;

            float2 localUV  = (uvPx - blobPos) / blobRadius;
            float  edgeFade = saturate(1.0 - length(localUV));              // minor quality improvement from ps2 -> alpha-taper the edges of the noise map so it looks a bit better at modern resolutions. -afevis
            float  n        = g_noise.Sample(g_sampler_point, frac(localUV * 0.5 + 0.5)).a;
            mask += n * edgeFade * 1.25; // opacity added by each blob over a given pixel
        }
        return saturate(mask);
    }

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0) {
        uv  = float2((id << 1) & 2, id & 2);
        pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    }

    float4 PS(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
        float blob     = BlobMask(uv, time, drift, resolution);
        float strength = lerp(topAlpha, bottomAlpha, uv.y) * blob;

        float2 off1 = float2(0.0, -(4.0 / 448.0) * (uv.y - 0.65) * 2.0);   // (x/448.0) = base vertical shift (px), (uv.y - x) = up/down zeroing. higher = more up.
        float2 off2 = float2(-(6.0 / 512.0) * (uv.x - 0.61) * 2.0,  0.0);  // (x/512.0) = base horizontal shift (px), (uv.x - x) = left/right zero zeroing. higher = more left.

        float4 c1 = g_tex.Sample(g_sampler, uv + off1);
        float4 c2 = g_tex.Sample(g_sampler, uv + off2);
        float4 c  = (c1 + c2) * 0.5;

        c.rgb = lerp(c.rgb, float3(0.8, 0.8, 0.8), blob * 0.035);
        c.a   = strength;
        return c;
    }
    )";

    ComPtr<ID3DBlob>                 vsBlob;
    ComPtr<ID3DBlob>                 psBlob;
    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11Buffer>             cb;
    ComPtr<ID3D11BlendState>         blend;
    ComPtr<ID3D11RasterizerState>    rs;
    ComPtr<ID3D11DepthStencilState>  dss;
    ComPtr<ID3D11SamplerState>       sampler;
    ComPtr<ID3D11SamplerState>       samplerPoint;
    ComPtr<ID3D11Texture2D>          captureTex;
    ComPtr<ID3D11ShaderResourceView> captureSRV;
    ComPtr<ID3D11Texture2D>          noiseTex;
    ComPtr<ID3D11ShaderResourceView> noiseSRV;
    UINT captureW = 0, captureH = 0;

    bool  bIsActive = false;
    float driftX = 0.f;
    float driftY = 0.f;

    void SetParams(ID3D11DeviceContext* ctx,
                   float t, float resX, float resY,
                   float topAlpha, float bottomAlpha,
                   float dx, float dy)
    {
        D3D11_MAPPED_SUBRESOURCE m;
        ctx->Map(cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
        float data[8] = { t, resX, resY, topAlpha, bottomAlpha, dx, dy, 0.f };
        memcpy(m.pData, data, 32);
        ctx->Unmap(cb.Get(), 0);
    }


    bool bHooksSet = false;

}

void MGS2_ShimmerEffect::SetupHooks()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    spdlog::info("MGS2_ShimmerEffect: Setting up hooks...");

    MAKE_HOOK_MID(baseModule, "4C 8B 7C 24 ?? 48 8B 5C 24 ?? 48 83 C4 ?? 5F 5E C3", "MGS2_ShimmerEffect: Act", {
        bIsActive = true;
                  });

    MAKE_HOOK_MID(baseModule, "40 53 48 83 EC ?? 48 8B D9 48 8B 89 ?? ?? ?? ?? 48 85 C9 74 ?? E8 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? 48 85 C9 74 ?? 48 83 C4 ?? 5B E9 ?? ?? ?? ?? 48 83 C4 ?? 5B C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 40 56", "MGS2_ShimmerEffect: Die", {
        bIsActive = false;
        driftX = 0.f;
        driftY = 0.f;
                  });

    /*
    g_InputHandler.RegisterHotkey(VK_NUMPAD8, "DEBUG - SHIMMER HEIGHT", []()
                                  {
                                      spdlog::info("linkvar - camera x {}, camera y {}, camera z {}", MGS2_LinkVarBuf::GM_CameraX.get(), MGS2_LinkVarBuf::GM_CameraY.get(), MGS2_LinkVarBuf::GM_CameraZ.get());
                                  });
                                  */

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("MGS2_ShimmerEffect: Failed to get D3DCompile");
        return;
    }

    ComPtr<ID3DBlob> err;

    HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kShimmerShader, strlen(kShimmerShader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS2_ShimmerEffect: Failed to compile vertex shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        return;
    }

    err.Reset();

    hr = g_D3D11Hooks.D3DCompileFunc(kShimmerShader, strlen(kShimmerShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS2_ShimmerEffect: Failed to compile pixel shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        vsBlob.Reset();
        return;
    }

    bHooksSet = true;

    spdlog::info("MGS2_ShimmerEffect: Hooks set.");
}

void MGS2_ShimmerEffect::Init()
{
    if (!bHooksSet)
    {
        spdlog::error("MGS2_ShimmerEffect: Hooks not set!");
        return;
    }

    if (bShaderLoaded)
    {
        spdlog::error("MGS2_ShimmerEffect: Shader already loaded");
        return;
    }

    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!dev)
    {
        spdlog::error("MGS2_ShimmerEffect: D3D11 device is not initialized");
        return;
    }

    if (!vsBlob || !psBlob)
    {
        spdlog::error("MGS2_ShimmerEffect: Shader bytecode was not compiled");
        return;
    }

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf());
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf());

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = 32;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&cbd, nullptr, cb.GetAddressOf());

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&sd, sampler.GetAddressOf());

    D3D11_SAMPLER_DESC psd = {};
    psd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    psd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    psd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    psd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    psd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    psd.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&psd, samplerPoint.GetAddressOf());

    int tw, th, tc;
    unsigned char* pixels = stbi_load_from_memory(mgs2_shimmer_noise_map::PngBlob.data(), static_cast<int>(mgs2_shimmer_noise_map::PngBlobSize), &tw, &th, &tc, 4);
    if (pixels)
    {
        for (int i = 3; i < tw * th * 4; i += 4)
        {
            pixels[i] = static_cast<uint8_t>(std::min(255, pixels[i] * 2));
        }

        D3D11_TEXTURE2D_DESC td = {};
        td.Width = tw;
        td.Height = th;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc = { 1, 0 };
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd = {};
        srd.pSysMem = pixels;
        srd.SysMemPitch = tw * 4;
        dev->CreateTexture2D(&td, &srd, noiseTex.GetAddressOf());
        dev->CreateShaderResourceView(noiseTex.Get(), nullptr, noiseSRV.GetAddressOf());
        stbi_image_free(pixels);
    }
    else
    {
        spdlog::error("MGS2_ShimmerEffect: Failed to decode noise texture");
        return;
    }

    D3D11_BLEND_DESC bd = {};
    auto& rt = bd.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D11_BLEND_OP_ADD;
    rt.SrcBlendAlpha = D3D11_BLEND_ZERO;
    rt.DestBlendAlpha = D3D11_BLEND_ONE;
    rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    dev->CreateBlendState(&bd, blend.GetAddressOf());

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
    SceneDepth::SetEndOf3DCallback(&MGS2_ShimmerEffect::Draw, SceneDepth::PRIORITY_HAZE);

    spdlog::info("MGS2_ShimmerEffect initialized.");
}

void MGS2_ShimmerEffect::Draw(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* /*depth*/)
{
    if (!bIsActive)
    {
        return;
    }

    if (g_GameVars.GV_PauseLevel() & MGS2_GV_PAUSE_PAUSE || g_GameVars.Get_GM_GameStatus() & STATE_DISP_GAMEOVER || (g_GameVars.GM_MenuStatus() & (MENU_WEAPON_OPEN | MENU_ITEM_OPEN | MENU_RADIO_ON)))
    {
        return;
    }

    if (MGS2_LinkVarBuf::GM_CameraY.get() <= -5000) //on the elevator
    {
        return;
    }

//    spdlog::info("tried to draw shimmer, camera y: {}", MGS2_LinkVarBuf::GM_CameraY.get()); 


    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    auto* dev = g_D3D11Hooks.d3dDevice.Get();

    static ULONGLONG lastTick = GetTickCount64();
    ULONGLONG nowTick = GetTickCount64();
    float dt = (nowTick - lastTick) * 0.001f;
    lastTick = nowTick;
    float t = nowTick * 0.001f;

    static constexpr float kAngleToRad = 2.0f * std::numbers::pi_v<float> / 4096.0f;

    float camY = static_cast<float>(MGS2_LinkVarBuf::GM_CameraY.get());
    float camRotX = static_cast<float>(MGS2_LinkVarBuf::GM_CamRotX.get()) * kAngleToRad;
    float camRotY = static_cast<float>(MGS2_LinkVarBuf::GM_CamRotY.get()) * kAngleToRad;

    float len = std::max(0.0f, std::min(fabsf(camY) - 2000.0f, 16000.0f));
    float distFade = 1.0f - len / 16000.0f;
    float topAlpha = std::abs(sinf(camRotX)) * distFade;
    float bottomAlpha = distFade;

    driftX += sinf(camRotY) * dt * 60.0f;
    driftY += sinf(camRotX) * dt * 60.0f;

    ComPtr<ID3D11Resource> sceneRes;
    sceneColor->GetResource(sceneRes.GetAddressOf());
    ComPtr<ID3D11Texture2D> sceneTex;
    if (FAILED(sceneRes.As(&sceneTex)))
        return;

    D3D11_TEXTURE2D_DESC bbDesc;
    sceneTex->GetDesc(&bbDesc);

    if (!captureTex || captureW != bbDesc.Width || captureH != bbDesc.Height)
    {
        captureSRV.Reset();
        captureTex.Reset();

        D3D11_TEXTURE2D_DESC td = bbDesc;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.CPUAccessFlags = 0;
        td.MiscFlags = 0;
        td.SampleDesc = { 1, 0 };
        dev->CreateTexture2D(&td, nullptr, captureTex.GetAddressOf());

        D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
        srvd.Format = td.Format;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvd.Texture2D.MipLevels = 1;
        dev->CreateShaderResourceView(captureTex.Get(), &srvd, captureSRV.GetAddressOf());

        captureW = bbDesc.Width;
        captureH = bbDesc.Height;
    }

    ctx->CopyResource(captureTex.Get(), sceneTex.Get());

    D3D11_VIEWPORT vp = { 0, 0, (float)bbDesc.Width, (float)bbDesc.Height, 0.f, 1.f };

    ID3D11RenderTargetView* oldRTV[8] = {};
    ID3D11DepthStencilView* oldDSV = nullptr;
    ID3D11BlendState* oldBlend = nullptr;
    ID3D11DepthStencilState* oldDSS = nullptr;
    ID3D11RasterizerState* oldRS = nullptr;
    ID3D11VertexShader* oldVS = nullptr;
    ID3D11PixelShader* oldPS = nullptr;
    ID3D11InputLayout* oldIL = nullptr;
    ID3D11ShaderResourceView* oldSRV[2] = {};
    ID3D11SamplerState* oldSamp[2] = {};
    ID3D11Buffer* oldCB = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY  oldTopo;
    D3D11_VIEWPORT            oldVP[1] = {};
    UINT  oldBlendMask = 0, oldStencilRef = 0, numVP = 1;
    float oldBlendFactor[4] = {};

    ctx->OMGetRenderTargets(8, oldRTV, &oldDSV);
    ctx->OMGetBlendState(&oldBlend, oldBlendFactor, &oldBlendMask);
    ctx->OMGetDepthStencilState(&oldDSS, &oldStencilRef);
    ctx->RSGetState(&oldRS);
    ctx->RSGetViewports(&numVP, oldVP);
    ctx->VSGetShader(&oldVS, nullptr, nullptr);
    ctx->PSGetShader(&oldPS, nullptr, nullptr);
    ctx->IAGetInputLayout(&oldIL);
    ctx->IAGetPrimitiveTopology(&oldTopo);
    ctx->PSGetShaderResources(0, 2, oldSRV);
    ctx->PSGetSamplers(0, 2, oldSamp);
    ctx->PSGetConstantBuffers(0, 1, &oldCB);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);
    ctx->PSSetShaderResources(0, 1, captureSRV.GetAddressOf());
    ctx->PSSetShaderResources(1, 1, noiseSRV.GetAddressOf());
    ctx->PSSetSamplers(0, 1, sampler.GetAddressOf());
    ctx->PSSetSamplers(1, 1, samplerPoint.GetAddressOf());
    ctx->PSSetConstantBuffers(0, 1, cb.GetAddressOf());
    ctx->OMSetBlendState(blend.Get(), nullptr, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(dss.Get(), 0);
    ctx->RSSetState(rs.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetRenderTargets(1, &sceneColor, nullptr);

    SetParams(ctx, t, (float)bbDesc.Width, (float)bbDesc.Height, topAlpha, bottomAlpha, driftX, driftY);
    ctx->Draw(3, 0);

    ctx->OMSetRenderTargets(8, oldRTV, oldDSV);
    ctx->OMSetBlendState(oldBlend, oldBlendFactor, oldBlendMask);
    ctx->OMSetDepthStencilState(oldDSS, oldStencilRef);
    ctx->RSSetState(oldRS);
    ctx->RSSetViewports(numVP, oldVP);
    ctx->VSSetShader(oldVS, nullptr, 0);
    ctx->PSSetShader(oldPS, nullptr, 0);
    ctx->IASetInputLayout(oldIL);
    ctx->IASetPrimitiveTopology(oldTopo);
    ctx->PSSetShaderResources(0, 2, oldSRV);
    ctx->PSSetSamplers(0, 2, oldSamp);
    ctx->PSSetConstantBuffers(0, 1, &oldCB);

    for (auto* r : oldRTV)  if (r) r->Release();
    for (auto* s : oldSRV)  if (s) s->Release();
    for (auto* s : oldSamp) if (s) s->Release();
    if (oldDSV)   oldDSV->Release();
    if (oldBlend) oldBlend->Release();
    if (oldDSS)   oldDSS->Release();
    if (oldRS)    oldRS->Release();
    if (oldVS)    oldVS->Release();
    if (oldPS)    oldPS->Release();
    if (oldIL)    oldIL->Release();
    if (oldCB)    oldCB->Release();
}
