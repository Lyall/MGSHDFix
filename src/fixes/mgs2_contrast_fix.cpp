// ReSharper disable CommentTypo
#include "stdafx.h"

#include "mgs2_contrast_fix.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"
#include "gamevars.hpp"
//#include "input_handler.hpp"

#include "logging.hpp"

//t00a2d      -> Tanker Opening Demo
//t12a3d      -> The Seizure of Metal gear Demo | Ocelot blows the bomb
//t12a4d      -> The Seizure of Metal gear Demo | Grulukovich body floating in water, marines shooting RAY.
//p001_03_p02 -> P001_03_P02 Opening Infiltration 3, Polygon Demo 2 (NM Skull Suit)
//p004_01_p01 -> P004_01_P01 First access to the node 1 Polygon Demo 1 (MC)
//p005_01_p01 -> P005_01_P01 Raiden elevator ascent 1 Polygon Demo 1 (MC)
//p012_01_p01 -> P012_01_P01 Fortune encounter 1 polygon demo 1 (MC)
//p068_01_p01 -> P068_01_P01 Prior to sniping Vamp 1 polygon demo 1
//p070_07_p06 -> P070_07_P06 AG launch 7 polygon demo 6 (KF+MC) | Shell 1 Core, level B2, Computer Room
//p070_09_p08 -> P070_09_P08 AG launch 9 polygon demo 8 | Shell 1 core, level B2, In front of the AG Entrance 
//p080_11_p06 -> P080_11_M05 AG Ascent 11 movie demo 5 (MC) | Fortune Miracle

//w32a        -> Vamp at Emma ?? Or does the sun have contrast when climbing down the oil fence? or is it the missing orange ambient?
//w32b        -> Vamp at Emma ??

using namespace MGS2_StatusFlags;

namespace
{
    const char* kContrastShader = R"(
    cbuffer CB : register(b0) { float4 col; }
    void VS(uint id : SV_VertexID, out float4 pos : SV_Position) {
        float2 uv = float2((id << 1) & 2, id & 2);
        pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    }
    float4 PS() : SV_Target { return col; }
    )";

    ComPtr<ID3DBlob>                vsBlob;
    ComPtr<ID3DBlob>                psBlob;
    ComPtr<ID3D11VertexShader>      vs;
    ComPtr<ID3D11PixelShader>       ps;
    ComPtr<ID3D11Buffer>            cb;
    ComPtr<ID3D11BlendState>        blendSub;       // Cd - Cs
    ComPtr<ID3D11BlendState>        blendNega;      // Cs - Cd
    ComPtr<ID3D11BlendState>        blendMul;       // Cd * factor (factor <= 1)
    ComPtr<ID3D11BlendState>        blendBrighten;  // Cd * factor (factor > 1)
    ComPtr<ID3D11RasterizerState>   rs;
    ComPtr<ID3D11DepthStencilState> dss;


    void SetColor(ID3D11DeviceContext* ctx, float r, float g, float b, float a)
    {
        D3D11_MAPPED_SUBRESOURCE m;
        ctx->Map(cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
        float col[4] = { r, g, b, a };
        memcpy(m.pData, col, 16);
        ctx->Unmap(cb.Get(), 0);
    }




    safetyhook::InlineHook MGS2_Die512_hook;
    /// user\\skoba\\weapon\\ai_ray_layout.c -> NewAiRaySight() -> Die()
    int64_t __fastcall MGS2_Die512(int64_t a1)
    {
        MGS2_ContrastShader::ClearOverride();
        //  spdlog::info("user\skoba\weapon\ai_ray_layout.c -> NewAiRaySight() -> Die(): clearing contrast shader override.");

        return MGS2_Die512_hook.call<int64_t>(a1);
    }


    MGS2_ContrastShader::ContrastWork overrideContrastWork = {};
    bool bOverrideActive = false;
};

void MGS2_ContrastShader::SetOverride(int keepR, int keepG, int keepB, int keepA, int negaFlag)
{
    overrideContrastWork = {};

    overrideContrastWork.keep_r_plus = keepR;
    overrideContrastWork.keep_g_plus = keepG;
    overrideContrastWork.keep_b_plus = keepB;
    overrideContrastWork.keep_a_plus = keepA;
    overrideContrastWork.nega_posi_flag = negaFlag;

    bOverrideActive = true;
}

void MGS2_ContrastShader::ClearOverride()
{
    bOverrideActive = false;
    overrideContrastWork = {};
}

MGS2_ContrastShader::ContrastWork* MGS2_ContrastShader::GetActiveWork()
{
    if (bOverrideActive)
    {
        return &overrideContrastWork;
    }

    if (!pContrastWork)
    {
        return nullptr;
    }

    return *pContrastWork;
}

void MGS2_ContrastShader::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS2_ContrastShader: Disabled in config, skipping shader compilation.");
        return;
    }

    spdlog::info("MGS2_ContrastShader: Setting up contrast shader...");

    if (uint8_t* MGS2_ContrastWorkScanResult = Memory::PatternScan(baseModule, "48 8B 0D ?? ?? ?? ?? 83 F8 ?? 41 8B F1 41 8B E8 44 8B F2 8D 78 ?? 0F 4E F8 48 85 C9 0F 85 ?? ?? ?? ?? 45 33 C9 48 89 5C 24 ?? BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 48 8B C8 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 81 4B ?? ?? ?? ?? ?? 33 C0 48 89 43 ?? 48 8B CB 48 89 43 ?? 48 89 43 ?? 48 8D 05 ?? ?? ?? ?? 48 89 43 ?? E8 ?? ?? ?? ?? 48 8B CB 85 C0 79 ?? E8 ?? ?? ?? ?? 33 C0 48 8B 5C 24 ?? 48 8B 6C 24 ?? 48 8B 74 24 ?? 48 83 C4 ?? 41 5F 41 5E 5F C3 8B 84 24 ?? ?? ?? ?? 44 8B 8C 24 ?? ?? ?? ?? 44 8B 84 24", "OK_FADE_IO_WORK_1"))
    {
        pContrastWork = reinterpret_cast<ContrastWork**>(Memory::GetRelativeOffset(MGS2_ContrastWorkScanResult + 3));
    }
    else
    {
        spdlog::error("MGS2_ContrastShader: Failed to locate OK_FADE_IO_WORK_1 structure");
        return;
    }

    spdlog::info("MGS2_ContrastShader: Located OK_FADE_IO_WORK_1 structure successfully");

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("MGS2_ContrastShader: Failed to get D3DCompile");
        return;
    }

    ComPtr<ID3DBlob> err;

    HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kContrastShader, strlen(kContrastShader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS2_ContrastShader: Failed to compile vertex shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        return;
    }

    err.Reset();

    hr = g_D3D11Hooks.D3DCompileFunc(kContrastShader, strlen(kContrastShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS2_ContrastShader: Failed to compile pixel shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        vsBlob.Reset();
        return;
    }

    spdlog::info("MGS2_ContrastShader: Compiled contrast shader successfully");

    MAKE_HOOK_MID(baseModule, "48 89 B3 ?? ?? ?? ?? 89 83", "NewAiRaySight -> Act -> MsgDie()", {
            MGS2_ContrastShader::SetOverride(6, 15, 15, 189, 0);
                  });

    if (uint8_t* MGS2_Die512ScanResult = Memory::PatternScan(baseModule, "4C 8D 05 ?? ?? ?? ?? 89 B8 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 80 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CB", "MGS 2: user\\skoba\\weapon\\ai_ray_layout.c -> NewAiRaySight() -> Die()"))
    {
        const uintptr_t MGS2_Die512Address = Memory::GetRipRelativeAddress(MGS2_Die512ScanResult, 3, 7);

        MGS2_Die512_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS2_Die512Address), reinterpret_cast<void*>(MGS2_Die512));
        LOG_HOOK(MGS2_Die512_hook, "MGS 2: Clear Contrast Shader Override : Die_512")
    }


      /*
    MAKE_HOOK_MID(baseModule, "48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 85 C0 75 ?? B8", "L2D_ReleaseLayout", {
    spdlog::info("ctx.rcx {}", ctx.rcx);
        if (ctx.rcx == 0 && g_GameVars.IsStage(MGS2Stages::D080P01))
        {
            MGS2_ContrastShader::ClearOverride();
        }
        spdlog::info("hit");
        });
        */


        /*

        static int contrastR = 64;
    static int contrastG = 112;
    static int contrastB = 112;
    static int contrastA = 176;
    static int contrastNega = 0;
    static int contrastStep = 1;

    const auto applyContrast = []()
        {
            contrastR = std::clamp(contrastR, 0, 255);
            contrastG = std::clamp(contrastG, 0, 255);
            contrastB = std::clamp(contrastB, 0, 255);
            contrastA = std::clamp(contrastA, 0, 255);
            contrastNega = std::clamp(contrastNega, 0, 1);

            MGS2_ContrastShader::SetOverride(contrastR, contrastG, contrastB, contrastA, contrastNega);

            spdlog::info(
                "Contrast override: R={} G={} B={} A={} Nega={}",
                contrastR,
                contrastG,
                contrastB,
                contrastA,
                contrastNega
            );
        };

    g_InputHandler.RegisterHeldHotkey('1', "contrast r+", [applyContrast]() { contrastR += contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('2', "contrast r-", [applyContrast]() { contrastR -= contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('3', "contrast g+", [applyContrast]() { contrastG += contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('4', "contrast g-", [applyContrast]() { contrastG -= contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('5', "contrast b+", [applyContrast]() { contrastB += contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('6', "contrast b-", [applyContrast]() { contrastB -= contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('7', "contrast a+", [applyContrast]() { contrastA += contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('8', "contrast a-", [applyContrast]() { contrastA -= contrastStep; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('9', "contrast nega on", [applyContrast]() { contrastNega = 1; applyContrast(); });
    g_InputHandler.RegisterHeldHotkey('0', "contrast nega off", [applyContrast]() { contrastNega = 0; applyContrast(); });
    */
}
void MGS2_ContrastShader::Init()
{
    if (!(eGameType & MGS2))
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
        spdlog::error("MGS2_ContrastShader: D3D11 device is not initialized");
        return;
    }

    if (!vsBlob || !psBlob)
    {
        spdlog::error("MGS2_ContrastShader: Shader bytecode was not compiled");
        return;
    }

    dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf());
    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf());

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = 16;
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&cbd, nullptr, cb.GetAddressOf());

    auto makeBlend = [&](D3D11_BLEND src, D3D11_BLEND dst, D3D11_BLEND_OP op, ComPtr<ID3D11BlendState>& out) {
        D3D11_BLEND_DESC bd = {};
        auto& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = src;
        rt.DestBlend = dst;
        rt.BlendOp = op;
        rt.SrcBlendAlpha = D3D11_BLEND_ZERO;
        rt.DestBlendAlpha = D3D11_BLEND_ONE;
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&bd, out.GetAddressOf());
        };

    makeBlend(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_REV_SUBTRACT, blendSub);
    makeBlend(D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_SUBTRACT, blendNega);
    makeBlend(D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD, blendMul);
    makeBlend(D3D11_BLEND_DEST_COLOR, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, blendBrighten);

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
    spdlog::info("MGS2_ContrastShader initialized.");
}

void MGS2_ContrastShader::Draw(IDXGISwapChain* swap, int keepR, int keepG, int keepB, int keepA, int negaFlag)
{
    if (!bShaderLoaded)
    {
        return;
    }
    if (keepR == 0 && keepG == 0 && keepB == 0 && keepA == 128)
    {
        return;
    }
    if (g_GameVars.GM_MenuStatus() & MENU_RADIO_ON) //cutscenes don't always clear the work packet when there's a codec call in the same sequence. this stops a stale state where the shader is still applied during those calls.
    {
        return;
    }

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    auto* dev = g_D3D11Hooks.d3dDevice.Get();

    // save previous draw state
    ID3D11RenderTargetView* oldRTV[8] = {};
    ID3D11DepthStencilView* oldDSV = nullptr;
    ID3D11BlendState* oldBlend = nullptr;
    ID3D11DepthStencilState* oldDSS = nullptr;
    ID3D11RasterizerState* oldRS = nullptr;
    ID3D11VertexShader* oldVS = nullptr;
    ID3D11PixelShader* oldPS = nullptr;
    ID3D11InputLayout* oldIL = nullptr;
    ID3D11Buffer* oldPSCB[1] = {};
    ID3D11Buffer* oldVB[1] = {};
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
    ctx->PSGetConstantBuffers(0, 1, oldPSCB);

    // get backbuffer RTV
    ComPtr<ID3D11Texture2D> backbuf;
    swap->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backbuf.GetAddressOf());
    ComPtr<ID3D11RenderTargetView> rtv;
    dev->CreateRenderTargetView(backbuf.Get(), nullptr, rtv.GetAddressOf());

    D3D11_TEXTURE2D_DESC bbDesc;
    backbuf->GetDesc(&bbDesc);
    D3D11_VIEWPORT vp = { 0, 0, (float)bbDesc.Width, (float)bbDesc.Height, 0.f, 1.f };

    // set pipeline
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, cb.GetAddressOf());
    ctx->RSSetState(rs.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetDepthStencilState(dss.Get(), 0);
    ctx->OMSetRenderTargets(1, rtv.GetAddressOf(), nullptr);

    // pass 1: color subtract/add
    if (keepR || keepG || keepB)
    {
        SetColor(ctx, keepR / 255.f, keepG / 255.f, keepB / 255.f, 1.f);
        ctx->OMSetBlendState(negaFlag ? blendNega.Get() : blendSub.Get(), nullptr, 0xFFFFFFFF);
        ctx->Draw(3, 0);
    }

    // pass 2: brightness multiply
    if (keepA != 128)
    {
        if (keepA <= 128)
        {
            float f = keepA / 128.f;
            SetColor(ctx, f, f, f, 1.f);
            ctx->OMSetBlendState(blendMul.Get(), nullptr, 0xFFFFFFFF);
        }
        else
        {
            float excess = (keepA - 128) / 128.f;
            SetColor(ctx, excess, excess, excess, 1.f);
            ctx->OMSetBlendState(blendBrighten.Get(), nullptr, 0xFFFFFFFF);
        }
        ctx->Draw(3, 0);
    }

    // restore previous draw state
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
    ctx->PSSetConstantBuffers(0, 1, oldPSCB);

    // release refs added by GetXxx
    for (auto* r : oldRTV) if (r) r->Release();
    if (oldDSV)     oldDSV->Release();
    if (oldBlend)   oldBlend->Release();
    if (oldDSS)     oldDSS->Release();
    if (oldRS)      oldRS->Release();
    if (oldVS)      oldVS->Release();
    if (oldPS)      oldPS->Release();
    if (oldIL)      oldIL->Release();
    if (oldVB[0])   oldVB[0]->Release();
    if (oldPSCB[0]) oldPSCB[0]->Release();

}

