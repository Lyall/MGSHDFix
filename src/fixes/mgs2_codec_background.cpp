#include "stdafx.h"

#include "mgs2_codec_background.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"

#include "logging.hpp"

#include <atomic>

// The codec background was modulated by cyan in past implementations.
// However, HDC/MC uses kST_GrayScale (EngineSupport\Shaders\FX.fx -> ps_grayscale()), which is shared with the VR mission clear effect, and thus lacks modulation.
// To avoid affecting the VR effect, hook BP_PostFX_MGS2_CodexInOut() and swap the binding only during that period.
//
// The backdrop is a snapshot the codec asks for once, through a render packet. A cutscene ending parks the renderer and
// the scripted call that follows un-parks it, so that one packet can land on a frame that never draws and gets thrown
// away. The codec then shows whatever was left in the snapshot buffer. If that happens, ask again.

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

    // SBP_PFX_CodexInOut::needPreviousFrameCopy, set on exactly one packet per codec session.
    constexpr ptrdiff_t kNeedPreviousFrameCopyOffset = 0x20;

    safetyhook::InlineHook codexInOutHook;
    SafetyHookMid          copyArmHook {};

    // Set when c_indemo GetResources() asks for the snapshot, cleared by the first pass that actually runs.
    std::atomic<bool> copyArmed { false };

    void __fastcall MGS2_CodexInOut(void* pData)
    {
        int& needCopy = *reinterpret_cast<int*>(static_cast<uint8_t*>(pData) + kNeedPreviousFrameCopyOffset);
        if (copyArmed.exchange(false, std::memory_order_relaxed) && needCopy == 0)
        {
            needCopy = 1;
            spdlog::info("MGS2_CodecBackground: backdrop snapshot was dropped by an undrawn frame, asked again.");
        }

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

    if (uint8_t* codexInOutResult = Memory::PatternScan(baseModule, "48 8B C4 53 48 81 EC ?? ?? ?? ?? 48 8B D9", "bp\\shared\\BP_RenderFX.cpp -> BP_PostFX_MGS2_CodexInOut()"))
    {
        codexInOutHook = safetyhook::create_inline(reinterpret_cast<void*>(codexInOutResult), reinterpret_cast<void*>(MGS2_CodexInOut));
        LOG_HOOK(codexInOutHook, "MGS2_CodecBackground: bp/shared/BP_RenderFX.cpp -> BP_PostFX_MGS2_CodexInOut()")
    }
    else
    {
        spdlog::error("MGS2_CodecBackground: Failed to locate BP_PostFX_MGS2_CodexInOut.");
        return;
    }

    if (uint8_t* armResult = Memory::PatternScan(baseModule, "C7 86 ?? ?? ?? ?? ?? ?? ?? ?? 48 85 C9", "mgs2x\\source\\user\\mode\\codec\\c_indemo.c -> GetResources() [needPreviousFrameCopy]"))
    {
        copyArmHook = safetyhook::create_mid(armResult, [](SafetyHookContext&) { copyArmed.store(true, std::memory_order_relaxed); });
        LOG_HOOK(copyArmHook, "MGS2_CodecBackground: mgs2x/source/user/mode/codec/c_indemo.c -> GetResources() [needPreviousFrameCopy]")
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
