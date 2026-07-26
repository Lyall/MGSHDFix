#include "stdafx.h"

#include "mgs2_codec_background.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"

#include "logging.hpp"

// The codec background was modulated by cyan in past implementations.
// However, HDC/MC uses kST_GrayScale (EngineSupport\Shaders\FX.fx -> ps_grayscale()), which is shared with the VR mission clear effect, and thus lacks modulation.
// To avoid affecting the VR effect, hook BP_PostFX_MGS2_CodexInOut() and swap the binding only during that period.

namespace
{
    const char* kCodecBackgroundShader = R"(
    Texture2D    tTexture : register(t0);
    SamplerState sTexture : register(s0);

    static const float3 kMonoCyan          = float3(80.0, 200.0, 200.0); // MONO_R, MONO_G, MONO_B (mgs2x\source\user\mode\codec\c_indemo.h)
    static const float  kGSModulateDivisor = 128.0;                      // GS modulate: 128 = 1.0

    struct PixelInput_GrayScale
    {
        float4 projPos : SV_Position;
        float4 color   : TEXCOORD0;
        float2 texUV   : TEXCOORD1;
    };

    float4 PS(PixelInput_GrayScale input) : SV_Target
    {
        float4 outputColor = tTexture.Sample(sTexture, input.texUV);
        float3 grayWeights = float3(0.2989, 0.5870, 0.1140);
        float3 grayColor   = dot(grayWeights, outputColor.xyz);
        grayColor *= 15.0 / 31.0;
        grayColor *= kMonoCyan / kGSModulateDivisor;
        outputColor.xyz = lerp(outputColor.xyz, grayColor, input.color.w);
        return outputColor;
    }
    )";

    ComPtr<ID3DBlob>          psBlob;
    ComPtr<ID3D11PixelShader> ps;

    uintptr_t grayShaderSlotAddress = 0;

    // CCompiledShader: vertex shader at +0x00, pixel shader at +0x08
    // (CCompiledShader::BeginPass passes them to VSSetShader / PSSetShader respectively).
    constexpr ptrdiff_t kCompiledShaderPixelShaderOffset = 0x08;

    safetyhook::InlineHook codexInOutHook;

    void __fastcall MGS2_CodexInOut(void* pData)
    {
        void* compiledShader = grayShaderSlotAddress ? *reinterpret_cast<void**>(grayShaderSlotAddress) : nullptr;

        void** pixelShaderField = nullptr;
        void* savedPixelShader = nullptr;

        if (compiledShader && ps)
        {
            pixelShaderField = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(compiledShader) + kCompiledShaderPixelShaderOffset);
            savedPixelShader = *pixelShaderField;
            *pixelShaderField = ps.Get();
        }

        codexInOutHook.call<void>(pData);

        if (pixelShaderField)
        {
            *pixelShaderField = savedPixelShader;
        }
    }
}

void MGS2_CodecBackground::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS2_CodecBackground: Disabled in config, skipping shader compilation.");
        return;
    }

    spdlog::info("MGS2_CodecBackground: Setting up codec background shader...");

    if (uint8_t* codexInOutResult = Memory::PatternScan(baseModule, "48 8B C4 53 48 81 EC ?? ?? ?? ?? 48 8B D9 0F 29 78 D8", "bp\\shared\\BP_RenderFX.cpp -> BP_PostFX_MGS2_CodexInOut()"))
    {
        codexInOutHook = safetyhook::create_inline(reinterpret_cast<void*>(codexInOutResult), reinterpret_cast<void*>(MGS2_CodexInOut));
        LOG_HOOK(codexInOutHook, "MGS2_CodecBackground: bp/shared/BP_RenderFX.cpp -> BP_PostFX_MGS2_CodexInOut()")
    }
    else
    {
        spdlog::error("MGS2_CodecBackground: Failed to locate BP_PostFX_MGS2_CodexInOut.");
        return;
    }

    if (uint8_t* grayShaderSlotResult = Memory::PatternScan(baseModule, "48 8B 0D ?? ?? ?? ?? 45 33 C0 33 D2 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 33 D2 E8 ?? ?? ?? ?? 48 8B 0D", "bp\\shared\\BP_RenderFX.cpp -> BP_PostFX_MGS2_CodexInOut() [FX::kST_GrayScale shader slot]"))
    {
        // MOV RCX,[rip+disp32]: disp at +3, instruction length 7
        grayShaderSlotAddress = Memory::GetRipRelativeAddress(grayShaderSlotResult, 3, 7);
    }
    else
    {
        spdlog::error("MGS2_CodecBackground: Failed to locate kST_GrayScale.");
        return;
    }

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("MGS2_CodecBackground: Failed to get D3DCompile.");
        return;
    }

    ComPtr<ID3DBlob> err;

    HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kCodecBackgroundShader, strlen(kCodecBackgroundShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS2_CodecBackground: Failed to compile pixel shader: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
        return;
    }

    spdlog::info("MGS2_CodecBackground: Compiled pixel shader successfully.");
}

void MGS2_CodecBackground::Init()
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
        spdlog::error("MGS2_CodecBackground: D3D11 device is not initialized.");
        return;
    }

    if (!psBlob)
    {
        spdlog::error("MGS2_CodecBackground: Pixel shader bytecode was not compiled.");
        return;
    }

    dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf());

    psBlob.Reset();

    bShaderLoaded = true;
    spdlog::info("MGS2_CodecBackground initialized.");
}