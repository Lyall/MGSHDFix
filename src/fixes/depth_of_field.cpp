#include "stdafx.h"
#include "depth_of_field.hpp"

#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "d3d11_api.hpp"
#include "gamevars.hpp"
#include "helper.hpp"
#include "scene_depth.hpp"
#ifndef RELEASE_BUILD
#include "input_handler.hpp"
#endif 
#include "logging.hpp"

namespace
{
    constexpr int kMgs2RegUvOffset0 = 96;
    constexpr int kMgs2RegUvOffset3 = 99;
    constexpr int kMgs3RegUvOffset0 = 0x1E;
    constexpr int kMgs3RegUvOffset3 = 0x21;
    constexpr int kFarFocusMaxPlaneCount = 16;
    constexpr int kMGS3FocusMaxPlaneCount = 64;
    constexpr UINT kMGS3FocusPyramidMipCount = 7;
    constexpr float kMGS3FocusSpreadBoost = 2.35f;
    constexpr uint64_t kMGS3PendingNearFocusMaxFrameAge = 2;
    constexpr float kFarFocusBlurWeight0 = 4.0f;
    constexpr float kFarFocusBlurWeight12 = 0.2f;
    constexpr float kFarFocusBlurWeight34 = 0.03125f;
    constexpr float kFarFocusBlurWeight56 = 0.00625f;
    constexpr ptrdiff_t kMgs3FarFocusBlurCallOffset = 0x5F;
    constexpr ptrdiff_t kMgs3BlurUvRegisterUploadCallOffset = 0x13;
    constexpr float kNearFocusMinDepthSpan = 0.00025f;
    constexpr int kNearFocusMinPlaneCount = 6;
    constexpr uint64_t kDepthFuncLEqual = 3;
    constexpr uint64_t kDepthFuncGEqual = 6;
    constexpr size_t kTrackedNearFocusPacketCount = 16;
    constexpr size_t kTrackedNearFocusWorkCount = 16;
    constexpr size_t kTrackedMGS3FocusPacketCount = 32;
    constexpr unsigned int kCmdPostFxFarFocus = 0x3F;
    constexpr unsigned int kMGS3CmdPostFxFarFocus = 0x31;
    constexpr int kDmapackNormal = 0x0001;
    constexpr int kDmapackInvisible123 = 0x0020 | 0x0040 | 0x0080;
    constexpr int kDmapackPhaseAfter = 0x04;
    constexpr ptrdiff_t kFocusWorkDisableOffset = 0x60;
    constexpr ptrdiff_t kFocusWorkMaxPlaneOffset = 0x64;
    constexpr ptrdiff_t kFocusWorkFocusNearOffset = 0x70;
    constexpr ptrdiff_t kFocusWorkFocusFarOffset = 0x74;
    constexpr ptrdiff_t kFocusWorkDmapackOffset = 0x80;
    constexpr ptrdiff_t kDmapackBpCallbackParamOffset = 0x28;
    constexpr ptrdiff_t kDmapackBpRenderCallbackOffset = 0x30;
    constexpr uint32_t kNearFocusSetId = 0x00BBAD24;
    constexpr uint32_t kNearFocusDemoId = 0x01000002;

    using BpRbAllocFn = void*(__fastcall*)(int);
    using BpRbAddCommandFn = void(__fastcall*)(unsigned int, void*);
    using DmapackRenderCallbackFn = void(__fastcall*)(void*);
    using SetDepthFuncFn = void(__fastcall*)(void*, uint64_t);

    bool bCutsceneNeedsSpecialHandling = false; //for per-cutscene effect skip handling.
    bool bIsD12T3 = false;
    int iNearEffectCount = 0;


    struct FocusSourcePacket
    {
        int maxPlane;
        float focusNear;
        float focusFar;
    };

    struct MGS3FarFocusPacket
    {
        int alpha;
        int maxPlane;
        float focusNear;
        float focusFar;
    };

    struct PendingMGS3FocusPacket
    {
        MGS3FarFocusPacket packet {};
        uint64_t frameIndex = 0;
        bool valid = false;
    };

    struct MGS3SRect
    {
        int x1;
        int y1;
        int x2;
        int y2;
    };

    enum class MGS3FocusSide
    {
        Far,
        Near,
    };

    enum class MGS3FocusSourceKind
    {
        Unknown,
        Near,
    };

    struct TrackedFocusPacket
    {
        uintptr_t address = 0;
        FocusSourcePacket packet {};
    };

    struct TrackedNearFocusWork
    {
        uintptr_t work = 0;
        uintptr_t dmapack = 0;
        uintptr_t originalParam = 0;
        DmapackRenderCallbackFn originalRender = nullptr;
    };

    struct TrackedMGS3FocusPacket
    {
        uintptr_t address = 0;
        MGS3FarFocusPacket packet {};
        MGS3FocusSourceKind sourceKind = MGS3FocusSourceKind::Unknown;
        bool applyFix = false;
    };

    struct TrackedMGS3NearFocusWork
    {
        uintptr_t work = 0;
        uintptr_t entry = 0;
        int alpha = 0;
        void(__fastcall* originalCallback)(void*) = nullptr;
    };

    SafetyHookInline SetVertexRegistersHook {};
    SafetyHookMid FarFocusBlurBeginHook {};
    SafetyHookMid FarFocusBlurEndHook {};
    SafetyHookMid NearFocusDepthFuncHook {};
    SafetyHookMid MGS3FarFocusDispatchHook {};
    SafetyHookMid MGS3FarFocusCompositeDrawHook {};
    SafetyHookMid MGS3NearFocusWorkLinkHook {};
    SafetyHookInline NearFocusSetHook {};
    SafetyHookInline NearFocusDemoHook {};
    SafetyHookInline BpRbAddCommandHook {};
    SafetyHookInline FarFocusCommandHook {};
    std::array<TrackedFocusPacket, kTrackedNearFocusPacketCount> gNearFocusPackets {};
    std::array<TrackedNearFocusWork, kTrackedNearFocusWorkCount> gNearFocusWorks {};
    std::array<TrackedMGS3FocusPacket, kTrackedMGS3FocusPacketCount> gMGS3FocusPackets {};
    std::array<TrackedMGS3NearFocusWork, kTrackedMGS3FocusPacketCount> gMGS3NearFocusWorks {};
    PendingMGS3FocusPacket gMGS3PendingNearFocusPacket {};
    size_t gNearFocusPacketWriteIndex = 0;
    size_t gNearFocusWorkWriteIndex = 0;
    size_t gMGS3FocusPacketWriteIndex = 0;
    size_t gMGS3NearFocusWorkWriteIndex = 0;
    BpRbAllocFn gBpRbAlloc = nullptr;
    BpRbAddCommandFn gBpRbAddCommand = nullptr;
    DmapackRenderCallbackFn gNearFocusOriginalRender = nullptr;
    SetDepthFuncFn gSetDepthFunc = nullptr;
    void* gDepthFuncBackend = nullptr;
    ComPtr<ID3D11VertexShader> gMGS3DofFocusVS;
    ComPtr<ID3D11PixelShader> gMGS3DofDepthPS;
    ComPtr<ID3D11Buffer> gMGS3DofFocusConstants;
    ComPtr<ID3D11Texture2D> gMGS3DofFocusSourceTexture;
    ComPtr<ID3D11ShaderResourceView> gMGS3DofFocusSourceSRV;
    ComPtr<ID3D11Resource> gMGS3DofFocusSourceFrameTarget;
    ComPtr<ID3D11SamplerState> gMGS3DofFocusSampler;
    ComPtr<ID3D11RasterizerState> gMGS3DofFocusRasterizerState;
    ComPtr<ID3D11DepthStencilState> gMGS3DofFocusDepthDisabledState;
    ComPtr<ID3D11BlendState> gMGS3DofFocusBlendState;
    D3D11_TEXTURE2D_DESC gMGS3DofFocusSourceDesc {};
    ID3D11Device* gMGS3DofFocusDevice = nullptr;
    uint64_t gMGS3DofFrameIndex = 0;
    uint64_t gMGS3DofFocusSourceFrameIndex = UINT64_MAX;
    UINT gMGS3DofFocusSourceMipCount = 1;
    bool gMGS3DofFocusSourceCanGenerateMips = false;
    bool gMGS3DofFocusReady = false;
    bool gMGS3DofFocusFailed = false;

    struct MGS3DofFocusConstants
    {
        float sourceRect[4] {};
        float sourceSizeAndSpread[4] {};
        float color[4] {};
        float planeData[kMGS3FocusMaxPlaneCount][4] {};
    };

    uintptr_t gMGS3FarFocusCompositeDrawReturn = 0;

    thread_local bool gInsideFarFocusBlur = false;
    thread_local bool gInsideNearFocusComposite = false;
    thread_local bool gExecutingNearFocusCommand = false;
    thread_local bool gInsideNearFocusAddCommand = false;
    thread_local bool gInsideOriginalNearFocusCallback = false;
    thread_local bool gOriginalNearFocusCommandSeen = false;
    thread_local uintptr_t gActiveNearFocusWork = 0;
    thread_local const FocusSourcePacket* gActiveNearFocusSource = nullptr;

    bool QueueNearFocusPacket(void* work);
    bool QueueNearFocusPacketFromSource(FocusSourcePacket source);
    bool BuildNearFocusSourceFromParam(uintptr_t paramAddress, FocusSourcePacket& source);
    bool StoreMGS3NearFocusPacketFromWork(uintptr_t work, int alpha);
    void __fastcall MGS3NearFocusCallback_Hook(void* param);
    bool IsReasonableMGS3SRect(const MGS3SRect& rect);

    void ResetMGS3DofDepthFocus()
    {
        gMGS3DofFocusVS.Reset();
        gMGS3DofDepthPS.Reset();
        gMGS3DofFocusConstants.Reset();
        gMGS3DofFocusSourceTexture.Reset();
        gMGS3DofFocusSourceSRV.Reset();
        gMGS3DofFocusSourceFrameTarget.Reset();
        gMGS3DofFocusSampler.Reset();
        gMGS3DofFocusRasterizerState.Reset();
        gMGS3DofFocusDepthDisabledState.Reset();
        gMGS3DofFocusBlendState.Reset();
        gMGS3DofFocusSourceDesc = {};
        gMGS3DofFocusDevice = nullptr;
        gMGS3DofFocusSourceFrameIndex = UINT64_MAX;
        gMGS3DofFocusSourceMipCount = 1;
        gMGS3DofFocusSourceCanGenerateMips = false;
        gMGS3DofFocusReady = false;
    }

    bool EnsureMGS3DofDepthFocus()
    {
        ID3D11Device* device = g_D3D11Hooks.d3dDevice.Get();
        if (!device)
        {
            return false;
        }

        if (gMGS3DofFocusDevice != device)
        {
            ResetMGS3DofDepthFocus();
            gMGS3DofFocusDevice = device;
            gMGS3DofFocusFailed = false;
        }

        if (gMGS3DofFocusReady)
        {
            return true;
        }

        if (gMGS3DofFocusFailed)
        {
            return false;
        }

        if (!g_D3D11Hooks.D3DCompileFunc)
        {
            gMGS3DofFocusFailed = true;
            spdlog::warn("MGS 3: Depth of Field: depth focus shader unavailable; D3DCompile missing.");
            return false;
        }

        const char* shader = R"(
            cbuffer FocusConstants : register(b0)
            {
                float4 sourceRect;
                float4 sourceSizeAndSpread;
                float4 focusColor;
                float4 planeData[64];
            };

            Texture2D focusSource : register(t0);
            Texture2D sceneDepth : register(t1);
            SamplerState focusSampler : register(s0);

            struct VSOut
            {
                float4 pos : SV_Position;
                float2 uv : TEXCOORD0;
            };

            VSOut FocusVS(uint id : SV_VertexID)
            {
                float2 uv = float2((id << 1) & 2, id & 2);
                VSOut output;
                output.pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
                output.uv = uv;
                return output;
            }

            float3 SampleBlurredSource(float2 sourceUv, float spread, float maxMip, float2 sourceSize)
            {
                static const float2 kernel[12] = {
                    float2( 0.40,  0.00),
                    float2(-0.40,  0.00),
                    float2( 0.00,  0.40),
                    float2( 0.00, -0.40),
                    float2( 0.30,  0.30),
                    float2(-0.30,  0.30),
                    float2( 0.30, -0.30),
                    float2(-0.30, -0.30),
                    float2( 0.78,  0.22),
                    float2(-0.78, -0.22),
                    float2( 0.22, -0.78),
                    float2(-0.22,  0.78)
                };

                float lod = clamp(log2(max(spread * 0.22, 1.0)), 0.0, maxMip);
                float2 radius = max(spread, 1.0) / sourceSize;
                float3 color = focusSource.SampleLevel(focusSampler, sourceUv, lod).rgb * 0.16;

                [unroll]
                for (int i = 0; i < 8; ++i)
                {
                    color += focusSource.SampleLevel(focusSampler, sourceUv + kernel[i] * radius, lod).rgb * 0.08;
                }

                [unroll]
                for (int j = 8; j < 12; ++j)
                {
                    color += focusSource.SampleLevel(focusSampler, sourceUv + kernel[j] * radius, lod).rgb * 0.05;
                }

                return color;
            }

            float FocusRangeAmount(float depth, float4 packet, bool nearSide)
            {
                float focusNear = packet.x;
                float focusFar = packet.y;
                float span = max(focusNear - focusFar, 0.000001);
                float amount = nearSide ? (depth - focusFar) / span : (focusNear - depth) / span;
                return smoothstep(0.0, 1.0, saturate(amount));
            }

            float FocusPacketAlpha(float amount, float4 packet)
            {
                float layerAlpha = saturate(packet.z);
                float planeCount = max(packet.w, 1.0);
                return saturate(1.0 - pow(max(1.0 - layerAlpha, 0.0001), amount * planeCount));
            }

            float4 DepthFocusPS(VSOut input) : SV_Target
            {
                float2 basePixel = sourceRect.xy + input.uv * sourceRect.zw;
                float2 sourceSize = max(sourceSizeAndSpread.xy, float2(1.0, 1.0));
                float2 sourceUv = basePixel / sourceSize;
                int2 depthPixel = int2(clamp(basePixel, float2(0.0, 0.0), sourceSize - 1.0));
                float depth = sceneDepth.Load(int3(depthPixel, 0)).r;

                float farAmount = FocusRangeAmount(depth, planeData[0], false);
                float nearAmount = FocusRangeAmount(depth, planeData[1], true);
                float farAlpha = FocusPacketAlpha(farAmount, planeData[0]);
                float nearAlpha = FocusPacketAlpha(nearAmount, planeData[1]);

                float farSpread = sourceSizeAndSpread.z * farAmount;
                float nearSpread = focusColor.a * nearAmount;
                float spread = max(farSpread, nearSpread);
                float maxMip = sourceSizeAndSpread.w;
                float alpha = max(farAlpha, nearAlpha);
                float3 color = SampleBlurredSource(sourceUv, spread, maxMip, sourceSize) * focusColor.rgb;
                return float4(color, alpha);
            }
        )";

        ComPtr<ID3DBlob> vsBlob;
        ComPtr<ID3DBlob> depthPsBlob;
        ComPtr<ID3DBlob> err;
        HRESULT hr = g_D3D11Hooks.D3DCompileFunc(shader, strlen(shader), nullptr, nullptr, nullptr, "FocusVS", "vs_5_0", 0, 0, vsBlob.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr))
        {
            gMGS3DofFocusFailed = true;
            spdlog::warn("MGS 3: Depth of Field: depth focus VS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
            return false;
        }

        err.Reset();
        hr = g_D3D11Hooks.D3DCompileFunc(shader, strlen(shader), nullptr, nullptr, nullptr, "DepthFocusPS", "ps_5_0", 0, 0, depthPsBlob.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr))
        {
            gMGS3DofFocusFailed = true;
            spdlog::warn("MGS 3: Depth of Field: depth focus PS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
            return false;
        }

        if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, gMGS3DofFocusVS.GetAddressOf())) ||
            FAILED(device->CreatePixelShader(depthPsBlob->GetBufferPointer(), depthPsBlob->GetBufferSize(), nullptr, gMGS3DofDepthPS.GetAddressOf())))
        {
            gMGS3DofFocusFailed = true;
            spdlog::warn("MGS 3: Depth of Field: depth focus shader creation failed.");
            return false;
        }

        D3D11_BUFFER_DESC constantsDesc {};
        constantsDesc.ByteWidth = sizeof(MGS3DofFocusConstants);
        constantsDesc.Usage = D3D11_USAGE_DEFAULT;
        constantsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(device->CreateBuffer(&constantsDesc, nullptr, gMGS3DofFocusConstants.GetAddressOf())))
        {
            gMGS3DofFocusFailed = true;
            spdlog::warn("MGS 3: Depth of Field: depth focus constants creation failed.");
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(device->CreateSamplerState(&samplerDesc, gMGS3DofFocusSampler.GetAddressOf())))
        {
            gMGS3DofFocusFailed = true;
            spdlog::warn("MGS 3: Depth of Field: depth focus sampler creation failed.");
            return false;
        }

        D3D11_RASTERIZER_DESC rasterizerDesc {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        if (FAILED(device->CreateRasterizerState(&rasterizerDesc, gMGS3DofFocusRasterizerState.GetAddressOf())))
        {
            gMGS3DofFocusFailed = true;
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depthDesc {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        if (FAILED(device->CreateDepthStencilState(&depthDesc, gMGS3DofFocusDepthDisabledState.GetAddressOf())))
        {
            gMGS3DofFocusFailed = true;
            return false;
        }

        D3D11_BLEND_DESC blendDesc {};
        auto& blendTarget = blendDesc.RenderTarget[0];
        blendTarget.BlendEnable = TRUE;
        blendTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendTarget.BlendOp = D3D11_BLEND_OP_ADD;
        blendTarget.SrcBlendAlpha = D3D11_BLEND_ZERO;
        blendTarget.DestBlendAlpha = D3D11_BLEND_ONE;
        blendTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(device->CreateBlendState(&blendDesc, gMGS3DofFocusBlendState.GetAddressOf())))
        {
            gMGS3DofFocusFailed = true;
            return false;
        }

        gMGS3DofFocusReady = true;
        spdlog::info("MGS 3: Depth of Field: depth renderer initialized.");
        return true;
    }

    bool SameMGS3DofFocusSourceDesc(const D3D11_TEXTURE2D_DESC& lhs, const D3D11_TEXTURE2D_DESC& rhs)
    {
        return lhs.Width == rhs.Width &&
            lhs.Height == rhs.Height &&
            lhs.MipLevels == rhs.MipLevels &&
            lhs.ArraySize == rhs.ArraySize &&
            lhs.Format == rhs.Format &&
            lhs.SampleDesc.Count == rhs.SampleDesc.Count &&
            lhs.SampleDesc.Quality == rhs.SampleDesc.Quality;
    }

    UINT GetMGS3DofFocusMipCount(UINT width, UINT height)
    {
        UINT mipCount = 1;
        while (mipCount < kMGS3FocusPyramidMipCount && (width > 1 || height > 1))
        {
            width = std::max<UINT>(width / 2, 1);
            height = std::max<UINT>(height / 2, 1);
            ++mipCount;
        }

        return mipCount;
    }

    bool CanGenerateMGS3DofFocusMips(ID3D11Device* device, DXGI_FORMAT format)
    {
        UINT support = 0;
        if (!device || FAILED(device->CheckFormatSupport(format, &support)))
        {
            return false;
        }

        constexpr UINT requiredSupport = D3D11_FORMAT_SUPPORT_RENDER_TARGET |
            D3D11_FORMAT_SUPPORT_SHADER_SAMPLE |
            D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;
        return (support & requiredSupport) == requiredSupport;
    }

    bool CopyMGS3DofFocusSourceFromCurrentTarget(ID3D11RenderTargetView* currentTarget, bool reuseCurrentFrame = false)
    {
        ID3D11Device* device = g_D3D11Hooks.d3dDevice.Get();
        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!device || !context || !currentTarget)
        {
            return false;
        }

        ComPtr<ID3D11Resource> targetResource;
        currentTarget->GetResource(targetResource.GetAddressOf());
        ComPtr<ID3D11Texture2D> targetTexture;
        if (!targetResource || FAILED(targetResource.As(&targetTexture)) || !targetTexture)
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC targetDesc {};
        targetTexture->GetDesc(&targetDesc);
        if (targetDesc.SampleDesc.Count != 1 || targetDesc.Width == 0 || targetDesc.Height == 0)
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC sourceDesc = targetDesc;
        const bool useMipPyramid = CanGenerateMGS3DofFocusMips(device, sourceDesc.Format);
        sourceDesc.MipLevels = useMipPyramid ? GetMGS3DofFocusMipCount(sourceDesc.Width, sourceDesc.Height) : 1;
        sourceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (useMipPyramid ? D3D11_BIND_RENDER_TARGET : 0);
        sourceDesc.CPUAccessFlags = 0;
        sourceDesc.Usage = D3D11_USAGE_DEFAULT;
        sourceDesc.MiscFlags = useMipPyramid ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

        if (reuseCurrentFrame &&
            gMGS3DofFocusSourceSRV &&
            gMGS3DofFocusSourceFrameTarget.Get() == targetResource.Get() &&
            gMGS3DofFocusSourceFrameIndex == gMGS3DofFrameIndex &&
            SameMGS3DofFocusSourceDesc(gMGS3DofFocusSourceDesc, sourceDesc))
        {
            return true;
        }

        if (!gMGS3DofFocusSourceTexture || !SameMGS3DofFocusSourceDesc(gMGS3DofFocusSourceDesc, sourceDesc))
        {
            gMGS3DofFocusSourceTexture.Reset();
            gMGS3DofFocusSourceSRV.Reset();
            gMGS3DofFocusSourceFrameTarget.Reset();
            gMGS3DofFocusSourceFrameIndex = UINT64_MAX;
            gMGS3DofFocusSourceMipCount = 1;
            gMGS3DofFocusSourceCanGenerateMips = false;
            if (FAILED(device->CreateTexture2D(&sourceDesc, nullptr, gMGS3DofFocusSourceTexture.GetAddressOf())) ||
                FAILED(device->CreateShaderResourceView(gMGS3DofFocusSourceTexture.Get(), nullptr, gMGS3DofFocusSourceSRV.GetAddressOf())))
            {
                gMGS3DofFocusSourceTexture.Reset();
                gMGS3DofFocusSourceSRV.Reset();
                gMGS3DofFocusSourceFrameTarget.Reset();
                gMGS3DofFocusSourceDesc = {};
                gMGS3DofFocusSourceFrameIndex = UINT64_MAX;
                gMGS3DofFocusSourceMipCount = 1;
                gMGS3DofFocusSourceCanGenerateMips = false;
                return false;
            }
            gMGS3DofFocusSourceDesc = sourceDesc;
            gMGS3DofFocusSourceMipCount = std::max<UINT>(sourceDesc.MipLevels, 1);
            gMGS3DofFocusSourceCanGenerateMips = useMipPyramid && gMGS3DofFocusSourceMipCount > 1;
        }

        context->CopySubresourceRegion(gMGS3DofFocusSourceTexture.Get(), 0, 0, 0, 0, targetTexture.Get(), 0, nullptr);
        if (gMGS3DofFocusSourceCanGenerateMips)
        {
            // Build a small source pyramid so wide blur samples stay cheap.
            context->GenerateMips(gMGS3DofFocusSourceSRV.Get());
        }
        gMGS3DofFocusSourceFrameTarget = targetResource;
        gMGS3DofFocusSourceFrameIndex = gMGS3DofFrameIndex;
        return true;
    }

    struct MGS3DofDepthFocusPassState
    {
        ID3D11RenderTargetView* oldRTV[8] = {};
        ID3D11DepthStencilView* oldDSV = nullptr;
        ID3D11BlendState* oldBlendState = nullptr;
        ID3D11DepthStencilState* oldDepthState = nullptr;
        ID3D11RasterizerState* oldRasterizerState = nullptr;
        ID3D11VertexShader* oldVS = nullptr;
        ID3D11GeometryShader* oldGS = nullptr;
        ID3D11PixelShader* oldPS = nullptr;
        ID3D11InputLayout* oldInputLayout = nullptr;
        ID3D11Buffer* oldVertexBuffer = nullptr;
        ID3D11Buffer* oldVSConstants = nullptr;
        ID3D11Buffer* oldPSConstants = nullptr;
        ID3D11ShaderResourceView* oldSRV[2] = {};
        ID3D11SamplerState* oldSampler = nullptr;
        D3D11_PRIMITIVE_TOPOLOGY oldTopology {};
        D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
        UINT oldViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        UINT oldBlendMask = 0;
        UINT oldStencilRef = 0;
        UINT oldStride = 0;
        UINT oldOffset = 0;
        float oldBlendFactor[4] = {};
        bool saved = false;
    };

    void ReleaseMGS3DofDepthFocusPassState(MGS3DofDepthFocusPassState& state)
    {
        for (auto* rtv : state.oldRTV)
        {
            if (rtv)
            {
                rtv->Release();
            }
        }
        if (state.oldDSV) state.oldDSV->Release();
        if (state.oldBlendState) state.oldBlendState->Release();
        if (state.oldDepthState) state.oldDepthState->Release();
        if (state.oldRasterizerState) state.oldRasterizerState->Release();
        if (state.oldVS) state.oldVS->Release();
        if (state.oldGS) state.oldGS->Release();
        if (state.oldPS) state.oldPS->Release();
        if (state.oldInputLayout) state.oldInputLayout->Release();
        if (state.oldVertexBuffer) state.oldVertexBuffer->Release();
        if (state.oldVSConstants) state.oldVSConstants->Release();
        if (state.oldPSConstants) state.oldPSConstants->Release();
        for (auto* srv : state.oldSRV)
        {
            if (srv) srv->Release();
        }
        if (state.oldSampler) state.oldSampler->Release();
        state = {};
    }

    bool SaveMGS3DofDepthFocusPassState(ID3D11DeviceContext* context, MGS3DofDepthFocusPassState& state)
    {
        if (!context)
        {
            return false;
        }

        context->OMGetRenderTargets(8, state.oldRTV, &state.oldDSV);
        context->OMGetBlendState(&state.oldBlendState, state.oldBlendFactor, &state.oldBlendMask);
        context->OMGetDepthStencilState(&state.oldDepthState, &state.oldStencilRef);
        context->RSGetState(&state.oldRasterizerState);
        context->RSGetViewports(&state.oldViewportCount, state.oldViewports);
        context->VSGetShader(&state.oldVS, nullptr, nullptr);
        context->GSGetShader(&state.oldGS, nullptr, nullptr);
        context->PSGetShader(&state.oldPS, nullptr, nullptr);
        context->VSGetConstantBuffers(0, 1, &state.oldVSConstants);
        context->PSGetConstantBuffers(0, 1, &state.oldPSConstants);
        context->PSGetShaderResources(0, static_cast<UINT>(std::size(state.oldSRV)), state.oldSRV);
        context->PSGetSamplers(0, 1, &state.oldSampler);
        context->IAGetInputLayout(&state.oldInputLayout);
        context->IAGetPrimitiveTopology(&state.oldTopology);
        context->IAGetVertexBuffers(0, 1, &state.oldVertexBuffer, &state.oldStride, &state.oldOffset);
        state.saved = true;
        return true;
    }

    void RestoreMGS3DofDepthFocusPass(ID3D11DeviceContext* context, MGS3DofDepthFocusPassState& state)
    {
        if (!context || !state.saved)
        {
            ReleaseMGS3DofDepthFocusPassState(state);
            return;
        }

        context->OMSetRenderTargets(8, state.oldRTV, state.oldDSV);
        context->OMSetBlendState(state.oldBlendState, state.oldBlendFactor, state.oldBlendMask);
        context->OMSetDepthStencilState(state.oldDepthState, state.oldStencilRef);
        context->RSSetState(state.oldRasterizerState);
        context->RSSetViewports(state.oldViewportCount, state.oldViewports);
        context->VSSetShader(state.oldVS, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &state.oldVSConstants);
        context->GSSetShader(state.oldGS, nullptr, 0);
        context->PSSetShader(state.oldPS, nullptr, 0);
        context->PSSetConstantBuffers(0, 1, &state.oldPSConstants);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(state.oldSRV)), state.oldSRV);
        context->PSSetSamplers(0, 1, &state.oldSampler);
        context->IASetInputLayout(state.oldInputLayout);
        context->IASetPrimitiveTopology(state.oldTopology);
        context->IASetVertexBuffers(0, 1, &state.oldVertexBuffer, &state.oldStride, &state.oldOffset);

        ReleaseMGS3DofDepthFocusPassState(state);
    }

    bool BeginMGS3DofDepthFocusPass(MGS3DofDepthFocusPassState& state)
    {
        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!context || !EnsureMGS3DofDepthFocus())
        {
            return false;
        }

        if (!SaveMGS3DofDepthFocusPassState(context, state))
        {
            return false;
        }

        if (!state.oldRTV[0] ||
            !state.oldDSV ||
            state.oldViewportCount == 0 ||
            !CopyMGS3DofFocusSourceFromCurrentTarget(state.oldRTV[0], true) ||
            !gMGS3DofFocusSourceSRV)
        {
            ReleaseMGS3DofDepthFocusPassState(state);
            return false;
        }

        ID3D11RenderTargetView* currentRTV = state.oldRTV[0];
        ID3D11ShaderResourceView* sourceSRVs[2] = { gMGS3DofFocusSourceSRV.Get(), nullptr };
        ID3D11SamplerState* sampler = gMGS3DofFocusSampler.Get();
        ID3D11Buffer* constantBuffer = gMGS3DofFocusConstants.Get();

        context->IASetInputLayout(nullptr);
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(gMGS3DofFocusVS.Get(), nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &constantBuffer);
        context->GSSetShader(nullptr, nullptr, 0);
        context->PSSetShader(gMGS3DofDepthPS.Get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 1, &constantBuffer);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(sourceSRVs)), sourceSRVs);
        context->PSSetSamplers(0, 1, &sampler);
        context->RSSetState(gMGS3DofFocusRasterizerState.Get());
        context->RSSetViewports(1, state.oldViewports);
        context->OMSetRenderTargets(1, &currentRTV, state.oldDSV);
        context->OMSetDepthStencilState(gMGS3DofFocusDepthDisabledState.Get(), 0);
        context->OMSetBlendState(gMGS3DofFocusBlendState.Get(), nullptr, 0xFFFFFFFF);
        return true;
    }

    bool IsUltrawide()
    {
        if (CustomResolutionAndBorderless::iInternalResX <= 0 || CustomResolutionAndBorderless::iInternalResY <= 0)
        {
            return false;
        }

        constexpr float nativeAspect = 16.0f / 9.0f;
        const float aspect = static_cast<float>(CustomResolutionAndBorderless::iInternalResX) / static_cast<float>(CustomResolutionAndBorderless::iInternalResY);
        return aspect > nativeAspect;
    }

    bool LooksLikeBlurUvOffset(const float* regs)
    {
        if (!regs)
        {
            return false;
        }

        float maxAbs = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            if (!std::isfinite(regs[i]))
            {
                return false;
            }

            maxAbs = std::max(maxAbs, std::abs(regs[i]));
        }

        return maxAbs > 0.000001f && maxAbs <= 0.08f;
    }

    std::pair<int, int> GetBlurUvRegisterRange()
    {
        if (eGameType & MGS3)
        {
            return { kMgs3RegUvOffset0, kMgs3RegUvOffset3 };
        }

        return { kMgs2RegUvOffset0, kMgs2RegUvOffset3 };
    }

    void ScaleActiveBlurAxis(float* regs, float multiplier)
    {
        const bool horizontalPass = (std::abs(regs[0]) > std::abs(regs[1])) || (std::abs(regs[2]) > std::abs(regs[3]));
        const bool verticalPass = (std::abs(regs[1]) > std::abs(regs[0])) || (std::abs(regs[3]) > std::abs(regs[2]));

        if (horizontalPass && !verticalPass)
        {
            regs[0] *= multiplier;
            regs[2] *= multiplier;
            return;
        }

        if (verticalPass && !horizontalPass)
        {
            regs[1] *= multiplier;
            regs[3] *= multiplier;
            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            regs[i] *= multiplier;
        }
    }

    bool IsReasonableFocusSourcePacket(const FocusSourcePacket* packet)
    {
        return packet &&
               packet->maxPlane >= 2 &&
               packet->maxPlane <= 64 &&
               std::isfinite(packet->focusNear) &&
               std::isfinite(packet->focusFar) &&
               packet->focusNear > 0.0f &&
               packet->focusNear < 1.0f &&
               packet->focusFar > 0.0f &&
               packet->focusFar < 1.0f &&
               packet->focusNear > packet->focusFar;
    }

    bool IsReasonableMGS3FarFocusPacket(const MGS3FarFocusPacket* packet)
    {
        return packet &&
               packet->alpha >= 0 &&
               packet->alpha <= 128 &&
               packet->maxPlane >= 0 &&
               packet->maxPlane <= 64 &&
               std::isfinite(packet->focusNear) &&
               std::isfinite(packet->focusFar) &&
               std::abs(packet->focusNear) <= 16.0f &&
               std::abs(packet->focusFar) <= 16.0f;
    }

    bool ShouldApplyMGS3FocusFix(const MGS3FarFocusPacket& packet)
    {
        if (packet.alpha == 0 ||
            packet.maxPlane <= 1 ||
            packet.focusNear <= packet.focusFar ||
            !std::isfinite(packet.focusNear) ||
            !std::isfinite(packet.focusFar))
        {
            return false;
        }

        return packet.focusNear > 0.0f &&
               packet.focusNear < 1.0f &&
               packet.focusFar > 0.0f &&
               packet.focusFar < 1.0f;
    }

    float NormalizeDepthValue(float depth)
    {
        if (!std::isfinite(depth))
        {
            return depth;
        }

        if (depth < 0.0f)
        {
            depth = (depth + 1.0f) * 0.5f;
        }

        return std::clamp(depth, 0.000001f, 0.999999f);
    }

    bool SameFocusPacket(const FocusSourcePacket& lhs, const FocusSourcePacket& rhs)
    {
        return lhs.maxPlane == rhs.maxPlane &&
               lhs.focusNear == rhs.focusNear &&
               lhs.focusFar == rhs.focusFar;
    }

    void WidenNearFocusPacket(FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return;
        }

        const float span = std::abs(packet->focusNear - packet->focusFar);
        if (span >= kNearFocusMinDepthSpan)
        {
            return;
        }

        const float center = (packet->focusNear + packet->focusFar) * 0.5f;
        const float halfSpan = kNearFocusMinDepthSpan * 0.5f;
        packet->focusNear = std::clamp(center + halfSpan, 0.000001f, 0.999999f);
        packet->focusFar = std::clamp(center - halfSpan, 0.000001f, 0.999999f);
    }

    bool PrepareFocusSource(FocusSourcePacket& source)
    {
        if (source.maxPlane < 1 ||
            source.maxPlane > 64 ||
            !std::isfinite(source.focusNear) ||
            !std::isfinite(source.focusFar))
        {
            return false;
        }
#ifndef RELEASE_BUILD
        spdlog::info("Original focus source: maxPlane = {}, focusNear = {}, focusFar = {}", source.maxPlane, source.focusNear, source.focusFar);
       // if (source.maxPlane == 8)
       // {
       //     return false;
       // }
#endif
        source.maxPlane = std::max(source.maxPlane, kNearFocusMinPlaneCount);
        source.focusNear = NormalizeDepthValue(source.focusNear);
        source.focusFar = NormalizeDepthValue(source.focusFar);

        if (source.focusNear < source.focusFar)
        {
            std::swap(source.focusNear, source.focusFar);
        }

        if (source.focusNear <= source.focusFar)
        {
            const float center = (source.focusNear + source.focusFar) * 0.5f;
            const float halfSpan = kNearFocusMinDepthSpan * 0.5f;
            source.focusNear = std::clamp(center + halfSpan, 0.000001f, 0.999999f);
            source.focusFar = std::clamp(center - halfSpan, 0.000001f, 0.999999f);
        }

        WidenNearFocusPacket(&source);
        return IsReasonableFocusSourcePacket(&source);
    }

    ptrdiff_t ModuleOffset(uintptr_t address)
    {
        const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(baseModule);
        return address >= moduleBase ? static_cast<ptrdiff_t>(address - moduleBase) : -1;
    }

    std::vector<uintptr_t> FindStageEntryFunctions(uint32_t id)
    {
        std::vector<uintptr_t> functions {};
        auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(baseModule);
        auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(baseModule) + dosHeader->e_lfanew);
        const size_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto* scanBytes = reinterpret_cast<uint8_t*>(baseModule);

        for (size_t i = 0; i + 16 <= sizeOfImage; ++i)
        {
            if (*reinterpret_cast<uint32_t*>(scanBytes + i) != id)
            {
                continue;
            }

            const uintptr_t function = *reinterpret_cast<uintptr_t*>(scanBytes + i + 8);
            if (!Memory::IsExecutable(reinterpret_cast<void*>(function)))
            {
                continue;
            }

            if (std::find(functions.begin(), functions.end(), function) == functions.end())
            {
                functions.push_back(function);
            }
        }

        return functions;
    }

    uint8_t* FindFunctionStart(uint8_t* interior)
    {
        if (!interior)
        {
            return nullptr;
        }

        constexpr size_t searchBack = 0x1000;
        for (size_t offset = 1; offset < searchBack; ++offset)
        {
            uint8_t* candidate = interior - offset;
            if (!Memory::IsReadable(candidate - 1, 17))
            {
                continue;
            }

            if (candidate[-1] != 0xCC || candidate[0] == 0xCC)
            {
                continue;
            }

            if (candidate[0] == 0x40 || candidate[0] == 0x48 || candidate[0] == 0x4C || candidate[0] == 0x53 ||
                candidate[0] == 0x55 || candidate[0] == 0x56 || candidate[0] == 0x57)
            {
                return candidate;
            }
        }

        return nullptr;
    }

    TrackedNearFocusWork* FindTrackedNearFocusWork(uintptr_t work)
    {
        for (TrackedNearFocusWork& tracked : gNearFocusWorks)
        {
            if (tracked.work == work)
            {
                return &tracked;
            }
        }

        return nullptr;
    }

    bool IsTrackedNearFocusWork(uintptr_t work)
    {
        return FindTrackedNearFocusWork(work) != nullptr;
    }

    TrackedNearFocusWork& TrackNearFocusWorkAddress(uintptr_t work, uintptr_t dmapack)
    {
        if (TrackedNearFocusWork* tracked = FindTrackedNearFocusWork(work))
        {
            tracked->dmapack = dmapack;
            return *tracked;
        }

        TrackedNearFocusWork& tracked = gNearFocusWorks[gNearFocusWorkWriteIndex++ % gNearFocusWorks.size()];
        tracked = {};
        tracked.work = work;
        tracked.dmapack = dmapack;
        return tracked;
    }

    void NearFocusRenderCallback(void* work)
    {
        bool originalCommandQueued = false;

        auto runOriginalCallback = [&](DmapackRenderCallbackFn callback, void* callbackParam, uintptr_t activeWork, const FocusSourcePacket* activeSource) {
            const bool previousInsideCallback = gInsideOriginalNearFocusCallback;
            const bool previousCommandSeen = gOriginalNearFocusCommandSeen;
            const uintptr_t previousActiveWork = gActiveNearFocusWork;
            const FocusSourcePacket* previousActiveSource = gActiveNearFocusSource;

            gInsideOriginalNearFocusCallback = true;
            gOriginalNearFocusCommandSeen = false;
            gActiveNearFocusWork = activeWork;
            gActiveNearFocusSource = activeSource;

            callback(callbackParam);
            const bool commandSeen = gOriginalNearFocusCommandSeen;

            gInsideOriginalNearFocusCallback = previousInsideCallback;
            gOriginalNearFocusCommandSeen = previousCommandSeen;
            gActiveNearFocusWork = previousActiveWork;
            gActiveNearFocusSource = previousActiveSource;

            return commandSeen;
        };

        if (TrackedNearFocusWork* tracked = FindTrackedNearFocusWork(reinterpret_cast<uintptr_t>(work)))
        {
            if (tracked->originalRender)
            {
                originalCommandQueued = runOriginalCallback(
                    tracked->originalRender,
                    reinterpret_cast<void*>(tracked->originalParam),
                    tracked->work,
                    nullptr);
            }

            if (!originalCommandQueued)
            {
                QueueNearFocusPacket(reinterpret_cast<void*>(tracked->work));
            }

            return;
        }

        FocusSourcePacket bufferedSource {};
        if (gNearFocusOriginalRender &&
            BuildNearFocusSourceFromParam(reinterpret_cast<uintptr_t>(work), bufferedSource))
        {
            originalCommandQueued = runOriginalCallback(gNearFocusOriginalRender, work, 0, &bufferedSource);
            if (!originalCommandQueued)
            {
                QueueNearFocusPacketFromSource(bufferedSource);
            }
            return;
        }

        QueueNearFocusPacket(work);
    }

    bool LooksLikeNearFocusDmapack(uintptr_t dmapack)
    {
        if (!dmapack ||
            !Memory::IsReadable(reinterpret_cast<void*>(dmapack), kDmapackBpRenderCallbackOffset + sizeof(uintptr_t)))
        {
            return false;
        }

        const int flag = Memory::ReadField<int>(dmapack, 0x00);
        const int16_t phase = Memory::ReadField<int16_t>(dmapack, 0x04);
        const int16_t priority = Memory::ReadField<int16_t>(dmapack, 0x06);

        if ((flag & kDmapackNormal) == 0 ||
            (flag & kDmapackInvisible123) != kDmapackInvisible123 ||
            phase != kDmapackPhaseAfter ||
            priority < 0 ||
            priority > 255)
        {
            return false;
        }

        return Memory::IsWritable(reinterpret_cast<void*>(dmapack + kDmapackBpCallbackParamOffset), sizeof(uintptr_t)) &&
               Memory::IsWritable(reinterpret_cast<void*>(dmapack + kDmapackBpRenderCallbackOffset), sizeof(uintptr_t));
    }

    bool LooksLikeNearFocusWork(uintptr_t workAddress)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(workAddress), kFocusWorkDmapackOffset + sizeof(uintptr_t)))
        {
            return false;
        }

        const int disable = Memory::ReadField<int>(workAddress, kFocusWorkDisableOffset);
        const int maxPlane = Memory::ReadField<int>(workAddress, kFocusWorkMaxPlaneOffset);
        const float focusNear = Memory::ReadField<float>(workAddress, kFocusWorkFocusNearOffset);
        const float focusFar = Memory::ReadField<float>(workAddress, kFocusWorkFocusFarOffset);

        if ((disable != 0 && disable != 1) ||
            maxPlane < 1 ||
            maxPlane > 64 ||
            !std::isfinite(focusNear) ||
            !std::isfinite(focusFar) ||
            std::abs(focusNear) > 16.0f ||
            std::abs(focusFar) > 16.0f)
        {
            return false;
        }

        return LooksLikeNearFocusDmapack(Memory::ReadField<uintptr_t>(workAddress, kFocusWorkDmapackOffset));
    }

    const TrackedMGS3FocusPacket* FindTrackedMGS3FocusPacketConst(uintptr_t packetAddress)
    {
        if (!packetAddress)
        {
            return nullptr;
        }

        const size_t trackedCount = std::min(gMGS3FocusPacketWriteIndex, gMGS3FocusPackets.size());
        for (size_t i = 0; i < trackedCount; ++i)
        {
            const size_t index = (gMGS3FocusPacketWriteIndex + gMGS3FocusPackets.size() - 1 - i) % gMGS3FocusPackets.size();
            const TrackedMGS3FocusPacket& tracked = gMGS3FocusPackets[index];
            if (tracked.address == packetAddress)
            {
                return &tracked;
            }
        }

        return nullptr;
    }

    char LowerAscii(char ch)
    {
        return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
    }

    bool ContainsAsciiCaseInsensitive(const std::string& text, const char* needle)
    {
        if (!needle || !*needle)
        {
            return true;
        }

        const size_t needleLength = std::strlen(needle);
        if (text.size() < needleLength)
        {
            return false;
        }

        for (size_t offset = 0; offset + needleLength <= text.size(); ++offset)
        {
            bool matched = true;
            for (size_t needleIndex = 0; needleIndex < needleLength; ++needleIndex)
            {
                if (LowerAscii(text[offset + needleIndex]) != LowerAscii(needle[needleIndex]))
                {
                    matched = false;
                    break;
                }
            }

            if (matched)
            {
                return true;
            }
        }

        return false;
    }

    std::string ReadMGS3CallbackSourceString(uintptr_t sourceString)
    {
        std::string result {};
        if (!sourceString)
        {
            return result;
        }

        constexpr size_t maxSourceLength = 260;
        result.reserve(96);
        for (size_t index = 0; index < maxSourceLength; ++index)
        {
            const char ch = Memory::ReadField<char>(sourceString, static_cast<ptrdiff_t>(index), '\0');
            if (ch == '\0')
            {
                break;
            }

            if (static_cast<unsigned char>(ch) < 0x20)
            {
                break;
            }

            result.push_back(ch);
        }

        return result;
    }

    bool IsMGS3NearFocusCallbackEntry(uintptr_t entry)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(entry), 0x58))
        {
            return false;
        }

        const uintptr_t sourceString = Memory::ReadField<uintptr_t>(entry, 0x50, 0);
        return ContainsAsciiCaseInsensitive(ReadMGS3CallbackSourceString(sourceString), "nearfocus");
    }

    bool IsMGS3NearFocusPacket(const TrackedMGS3FocusPacket* tracked)
    {
        return tracked && tracked->sourceKind == MGS3FocusSourceKind::Near;
    }

    TrackedMGS3NearFocusWork* FindTrackedMGS3NearFocusWork(uintptr_t work)
    {
        if (!work)
        {
            return nullptr;
        }

        const size_t trackedCount = std::min(gMGS3NearFocusWorkWriteIndex, gMGS3NearFocusWorks.size());
        for (size_t i = 0; i < trackedCount; ++i)
        {
            const size_t index = (gMGS3NearFocusWorkWriteIndex + gMGS3NearFocusWorks.size() - 1 - i) % gMGS3NearFocusWorks.size();
            TrackedMGS3NearFocusWork& tracked = gMGS3NearFocusWorks[index];
            if (tracked.work == work)
            {
                return &tracked;
            }
        }

        return nullptr;
    }

    TrackedMGS3NearFocusWork* FindTrackedMGS3NearFocusEntry(uintptr_t entry)
    {
        if (!entry)
        {
            return nullptr;
        }

        const size_t trackedCount = std::min(gMGS3NearFocusWorkWriteIndex, gMGS3NearFocusWorks.size());
        for (size_t i = 0; i < trackedCount; ++i)
        {
            const size_t index = (gMGS3NearFocusWorkWriteIndex + gMGS3NearFocusWorks.size() - 1 - i) % gMGS3NearFocusWorks.size();
            TrackedMGS3NearFocusWork& tracked = gMGS3NearFocusWorks[index];
            if (tracked.entry == entry)
            {
                return &tracked;
            }
        }

        return nullptr;
    }

    void TrackMGS3NearFocusWork(
        uintptr_t work,
        int alpha,
        uintptr_t entry = 0,
        void(__fastcall* originalCallback)(void*) = nullptr)
    {
        if (!work)
        {
            return;
        }

        if (TrackedMGS3NearFocusWork* existing = FindTrackedMGS3NearFocusWork(work))
        {
            if (!entry)
            {
                entry = existing->entry;
            }
            if (!originalCallback)
            {
                originalCallback = existing->originalCallback;
            }
        }

        if (!originalCallback)
        {
            if (TrackedMGS3NearFocusWork* existingEntry = FindTrackedMGS3NearFocusEntry(entry))
            {
                originalCallback = existingEntry->originalCallback;
            }
        }

        TrackedMGS3NearFocusWork& tracked = gMGS3NearFocusWorks[gMGS3NearFocusWorkWriteIndex++ % gMGS3NearFocusWorks.size()];
        tracked.work = work;
        tracked.entry = entry;
        tracked.alpha = std::clamp(alpha, 0, 128);
        tracked.originalCallback = originalCallback;
    }

    int FindTrackedMGS3NearFocusWorkAlpha(uintptr_t work, int fallback)
    {
        if (TrackedMGS3NearFocusWork* tracked = FindTrackedMGS3NearFocusWork(work))
        {
            return tracked->alpha;
        }

        return fallback;
    }

    void(__fastcall* FindTrackedMGS3NearFocusCallback(uintptr_t work))(void*)
    {
        if (TrackedMGS3NearFocusWork* tracked = FindTrackedMGS3NearFocusWork(work))
        {
            return tracked->originalCallback;
        }

        return nullptr;
    }

    void __fastcall MGS3NearFocusCallback_Hook(void* param)
    {
        const uintptr_t work = reinterpret_cast<uintptr_t>(param);
        const int fallbackAlpha = Memory::IsReadable(param, 0x9C)
            ? static_cast<int>(Memory::ReadField<int16_t>(work, 0x88, 0))
            : 0;

        StoreMGS3NearFocusPacketFromWork(work, FindTrackedMGS3NearFocusWorkAlpha(work, fallbackAlpha));

        if (auto* originalCallback = FindTrackedMGS3NearFocusCallback(work))
        {
            originalCallback(param);
        }
    }

    void LinkMGS3NearFocusWorkToCallbackEntry(uintptr_t work, int currentAlpha)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(work), 0x9C))
        {
            return;
        }

        const uintptr_t entry = Memory::ReadField<uintptr_t>(work, 0x68, 0);
        if (!IsMGS3NearFocusCallbackEntry(entry))
        {
            return;
        }

        auto* callbackSlot = reinterpret_cast<uintptr_t*>(entry + 0x40);
        void(__fastcall* originalCallback)(void*) = nullptr;
        if (Memory::IsReadable(callbackSlot, sizeof(*callbackSlot)))
        {
            const uintptr_t callback = *callbackSlot;
            const uintptr_t hookCallback = reinterpret_cast<uintptr_t>(&MGS3NearFocusCallback_Hook);
            if (callback == hookCallback)
            {
                if (TrackedMGS3NearFocusWork* trackedEntry = FindTrackedMGS3NearFocusEntry(entry))
                {
                    originalCallback = trackedEntry->originalCallback;
                }
            }
            else if (Memory::IsExecutable(reinterpret_cast<void*>(callback)))
            {
                originalCallback = reinterpret_cast<void(__fastcall*)(void*)>(callback);
                if (Memory::IsWritable(callbackSlot, sizeof(*callbackSlot)))
                {
                    *callbackSlot = hookCallback;
                }
            }
        }

        TrackMGS3NearFocusWork(work, currentAlpha, entry, originalCallback);

        auto* callbackParam = reinterpret_cast<uintptr_t*>(entry + 0x38);
        if (Memory::IsWritable(callbackParam, sizeof(*callbackParam)))
        {
            *callbackParam = work;
        }
    }

    void TrackMGS3FocusPacket(const MGS3FarFocusPacket* packet, MGS3FocusSourceKind sourceKind = MGS3FocusSourceKind::Unknown)
    {
        if (!Memory::IsReadable(packet, sizeof(MGS3FarFocusPacket)) ||
            !IsReasonableMGS3FarFocusPacket(packet))
        {
            return;
        }

        const uintptr_t packetAddress = reinterpret_cast<uintptr_t>(packet);
        const TrackedMGS3FocusPacket* previousTracked = FindTrackedMGS3FocusPacketConst(packetAddress);

        TrackedMGS3FocusPacket& tracked = gMGS3FocusPackets[gMGS3FocusPacketWriteIndex++ % gMGS3FocusPackets.size()];
        tracked = {};
        tracked.address = packetAddress;
        tracked.packet = *packet;
        tracked.sourceKind = sourceKind;
        tracked.applyFix = ShouldApplyMGS3FocusFix(tracked.packet);

        if (tracked.sourceKind == MGS3FocusSourceKind::Unknown && previousTracked)
        {
            tracked.sourceKind = previousTracked->sourceKind;
        }
    }

    bool BuildMGS3NearFocusPacketFromWork(uintptr_t work, int alpha, MGS3FarFocusPacket& packet)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(work), 0x9C))
        {
            return false;
        }

        packet = MGS3FarFocusPacket {
            std::clamp(alpha, 0, 128),
            Memory::ReadField<int>(work, 0x54, 0),
            Memory::ReadField<float>(work, 0x58, 0.0f),
            Memory::ReadField<float>(work, 0x5C, 0.0f),
        };

        return ShouldApplyMGS3FocusFix(packet);
    }

    bool StoreMGS3NearFocusPacketFromWork(uintptr_t work, int alpha)
    {
        MGS3FarFocusPacket packet {};
        if (!BuildMGS3NearFocusPacketFromWork(work, alpha, packet))
        {
            return false;
        }

        gMGS3PendingNearFocusPacket.packet = packet;
        gMGS3PendingNearFocusPacket.frameIndex = gMGS3DofFrameIndex;
        gMGS3PendingNearFocusPacket.valid = true;
        return true;
    }

    bool ConsumeMGS3PendingNearFocusPacket(MGS3FarFocusPacket& packet)
    {
        if (!gMGS3PendingNearFocusPacket.valid ||
            !ShouldApplyMGS3FocusFix(gMGS3PendingNearFocusPacket.packet))
        {
            return false;
        }

        if (gMGS3DofFrameIndex < gMGS3PendingNearFocusPacket.frameIndex ||
            gMGS3DofFrameIndex - gMGS3PendingNearFocusPacket.frameIndex > kMGS3PendingNearFocusMaxFrameAge)
        {
            gMGS3PendingNearFocusPacket.valid = false;
            return false;
        }

        packet = gMGS3PendingNearFocusPacket.packet;
        gMGS3PendingNearFocusPacket.valid = false;
        return true;
    }

    bool ShouldApplyTrackedMGS3FocusFix(const MGS3FarFocusPacket* packet)
    {
        const uintptr_t packetAddress = reinterpret_cast<uintptr_t>(packet);
        if (const TrackedMGS3FocusPacket* tracked = FindTrackedMGS3FocusPacketConst(packetAddress))
        {
            return tracked->applyFix;
        }

        if (!Memory::IsReadable(packet, sizeof(MGS3FarFocusPacket)) ||
            !IsReasonableMGS3FarFocusPacket(packet))
        {
            return false;
        }

        return ShouldApplyMGS3FocusFix(*packet);
    }

    bool IsReasonableMGS3SRect(const MGS3SRect& rect)
    {
        const int width = rect.x2 - rect.x1;
        const int height = rect.y2 - rect.y1;
        return width > 0 &&
               height > 0 &&
               width <= 8192 &&
               height <= 8192 &&
               std::abs(rect.x1) <= 8192 &&
               std::abs(rect.y1) <= 8192 &&
               std::abs(rect.x2) <= 16384 &&
               std::abs(rect.y2) <= 16384;
    }

    struct MGS3PreparedFocusDraw
    {
        const MGS3FarFocusPacket* packet = nullptr;
        MGS3SRect fullRect {};
        int focusPixelScale = 0;
    };

    bool PrepareMGS3FocusDraw(
        uintptr_t stackBase,
        const MGS3FarFocusPacket* packet,
        MGS3FocusSide focusSide,
        MGS3PreparedFocusDraw& prepared)
    {
        if (!packet)
        {
            return false;
        }

        const MGS3SRect fullRect = Memory::ReadField<MGS3SRect>(stackBase, 0x60);
        if (!IsReasonableMGS3SRect(fullRect))
        {
            return false;
        }

        const int height = fullRect.y2 - fullRect.y1;
        const float configuredSpread = std::clamp(g_DepthOfFieldFixes.fBlurUvMultiplier / 5.0f, 1.0f, 4.0f);
        const int baseFocusPixelScale = std::clamp(static_cast<int>((height / 480.0f + 0.5f) * configuredSpread), 2, 22);
        const bool nearSide = focusSide == MGS3FocusSide::Near;
        const float sideSpreadScale = nearSide ? 1.12f : 1.0f;
        const int focusPixelScale = std::clamp(static_cast<int>(baseFocusPixelScale * sideSpreadScale * kMGS3FocusSpreadBoost + 0.5f), 2, 56);

        prepared = {};
        prepared.packet = packet;
        prepared.fullRect = fullRect;
        prepared.focusPixelScale = focusPixelScale;
        return true;
    }

    bool DrawMGS3DepthWeightedFocus(
        const MGS3PreparedFocusDraw& farDraw,
        const MGS3PreparedFocusDraw* nearDraw,
        ID3D11ShaderResourceView* depthSRV)
    {
        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!context ||
            !gMGS3DofDepthPS ||
            !gMGS3DofFocusConstants ||
            !gMGS3DofFocusDepthDisabledState ||
            !gMGS3DofFocusSourceSRV ||
            !depthSRV ||
            !farDraw.packet ||
            !IsReasonableMGS3SRect(farDraw.fullRect))
        {
            return false;
        }

        const MGS3PreparedFocusDraw* activeNear = (nearDraw && nearDraw->packet) ? nearDraw : nullptr;
        const int width = farDraw.fullRect.x2 - farDraw.fullRect.x1;
        const int height = farDraw.fullRect.y2 - farDraw.fullRect.y1;

        MGS3DofFocusConstants constants {};
        constants.sourceRect[0] = static_cast<float>(farDraw.fullRect.x1);
        constants.sourceRect[1] = static_cast<float>(farDraw.fullRect.y1);
        constants.sourceRect[2] = static_cast<float>(width);
        constants.sourceRect[3] = static_cast<float>(height);
        constants.sourceSizeAndSpread[0] = static_cast<float>(gMGS3DofFocusSourceDesc.Width);
        constants.sourceSizeAndSpread[1] = static_cast<float>(gMGS3DofFocusSourceDesc.Height);
        constants.sourceSizeAndSpread[2] = static_cast<float>(farDraw.focusPixelScale);
        constants.sourceSizeAndSpread[3] = static_cast<float>(std::max<UINT>(gMGS3DofFocusSourceMipCount, 1) - 1);
        constants.color[0] = 1.0f;
        constants.color[1] = 1.0f;
        constants.color[2] = 1.0f;
        constants.color[3] = static_cast<float>(activeNear ? activeNear->focusPixelScale : 0);

        const auto writePacket = [&](int index, const MGS3PreparedFocusDraw* draw) {
            if (!draw || !draw->packet)
            {
                constants.planeData[index][0] = 1.0f;
                constants.planeData[index][1] = 0.0f;
                constants.planeData[index][2] = 0.0f;
                constants.planeData[index][3] = 1.0f;
                return;
            }

            constants.planeData[index][0] = draw->packet->focusNear;
            constants.planeData[index][1] = draw->packet->focusFar;
            constants.planeData[index][2] = std::clamp(draw->packet->alpha / 128.0f, 0.0f, 1.0f);
            constants.planeData[index][3] = static_cast<float>(std::clamp(draw->packet->maxPlane, 1, kMGS3FocusMaxPlaneCount));
        };

        writePacket(0, &farDraw);
        writePacket(1, activeNear);
        context->UpdateSubresource(gMGS3DofFocusConstants.Get(), 0, nullptr, &constants, 0, 0);

        ID3D11ShaderResourceView* srvs[2] = { gMGS3DofFocusSourceSRV.Get(), depthSRV };
        ID3D11Buffer* constantBuffer = gMGS3DofFocusConstants.Get();
        context->PSSetShader(gMGS3DofDepthPS.Get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 1, &constantBuffer);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(srvs)), srvs);
        context->OMSetDepthStencilState(gMGS3DofFocusDepthDisabledState.Get(), 0);

        context->DrawInstanced(3, 1, 0, 0);
        return true;
    }

    void DrawMGS3CombinedFocusSamples(
        uintptr_t stackBase,
        const MGS3FarFocusPacket* farPacket,
        const MGS3FarFocusPacket* nearPacket)
    {
        MGS3PreparedFocusDraw farDraw {};
        if (!PrepareMGS3FocusDraw(stackBase, farPacket, MGS3FocusSide::Far, farDraw))
        {
            return;
        }

        MGS3PreparedFocusDraw nearDraw {};
        const bool hasNear = nearPacket && PrepareMGS3FocusDraw(stackBase, nearPacket, MGS3FocusSide::Near, nearDraw);

        MGS3DofDepthFocusPassState passState {};
        if (!BeginMGS3DofDepthFocusPass(passState))
        {
            return;
        }

        ID3D11ShaderResourceView* depthSRV = SceneDepth::CaptureDepth(passState.oldDSV);
        DrawMGS3DepthWeightedFocus(farDraw, hasNear ? &nearDraw : nullptr, depthSRV);

        RestoreMGS3DofDepthFocusPass(g_D3D11Hooks.d3dDeviceContext.Get(), passState);
    }

    void InstallNearFocusDmapackCallback(void* work)
    {
        const uintptr_t workAddress = reinterpret_cast<uintptr_t>(work);
        const uintptr_t dmapack = Memory::ReadField<uintptr_t>(workAddress, kFocusWorkDmapackOffset);
        if (!LooksLikeNearFocusDmapack(dmapack))
        {
            return;
        }

        const uintptr_t originalParam = Memory::ReadField<uintptr_t>(dmapack, kDmapackBpCallbackParamOffset);
        const uintptr_t originalRender = Memory::ReadField<uintptr_t>(dmapack, kDmapackBpRenderCallbackOffset);
        TrackedNearFocusWork& tracked = TrackNearFocusWorkAddress(workAddress, dmapack);

        if (originalRender != reinterpret_cast<uintptr_t>(NearFocusRenderCallback))
        {
            tracked.originalParam = originalParam;
            tracked.originalRender = Memory::IsExecutable(reinterpret_cast<void*>(originalRender))
                ? reinterpret_cast<DmapackRenderCallbackFn>(originalRender)
                : nullptr;

            if (!gNearFocusOriginalRender && tracked.originalRender)
            {
                gNearFocusOriginalRender = tracked.originalRender;
            }
        }

        *reinterpret_cast<uintptr_t*>(dmapack + kDmapackBpCallbackParamOffset) = workAddress;
        *reinterpret_cast<uintptr_t*>(dmapack + kDmapackBpRenderCallbackOffset) = reinterpret_cast<uintptr_t>(NearFocusRenderCallback);

        static bool logged = false;
        if (!logged)
        {
            logged = true;
            spdlog::info("MGS 2: Depth of Field: near focus callback installed.");
        }
    }

    void TrackNearFocusWork(void* work)
    {
        const uintptr_t workAddress = reinterpret_cast<uintptr_t>(work);
        if (!workAddress)
        {
            return;
        }

        if (!LooksLikeNearFocusWork(workAddress))
        {
            return;
        }

        InstallNearFocusDmapackCallback(work);
    }

    bool BuildNearFocusSourceFromWork(uintptr_t workAddress, FocusSourcePacket& source)
    {
        if (!IsTrackedNearFocusWork(workAddress) ||
            Memory::ReadField<int>(workAddress, kFocusWorkDisableOffset) != 0)
        {
            return false;
        }

        source.maxPlane = std::max(Memory::ReadField<int>(workAddress, kFocusWorkMaxPlaneOffset), kNearFocusMinPlaneCount);
        source.focusNear = NormalizeDepthValue(Memory::ReadField<float>(workAddress, kFocusWorkFocusNearOffset));
        source.focusFar = NormalizeDepthValue(Memory::ReadField<float>(workAddress, kFocusWorkFocusFarOffset));

        return PrepareFocusSource(source);
    }

    bool BuildNearFocusSourceFromParam(uintptr_t paramAddress, FocusSourcePacket& source)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(paramAddress), sizeof(FocusSourcePacket)))
        {
            return false;
        }

        source = *reinterpret_cast<const FocusSourcePacket*>(paramAddress);
        return PrepareFocusSource(source);
    }

    void TrackNearFocusPacket(const FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return;
        }

        TrackedFocusPacket& tracked = gNearFocusPackets[gNearFocusPacketWriteIndex++ % gNearFocusPackets.size()];
        tracked.address = reinterpret_cast<uintptr_t>(packet);
        tracked.packet = *packet;
    }

    bool ConsumeNearFocusPacket(const FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return false;
        }

        const uintptr_t address = reinterpret_cast<uintptr_t>(packet);
        for (TrackedFocusPacket& tracked : gNearFocusPackets)
        {
            if (tracked.address == address && SameFocusPacket(tracked.packet, *packet))
            {
                tracked.address = 0;
                return true;
            }
        }

        return false;
    }

    bool IsTrackedNearFocusPacket(const FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return false;
        }

        const uintptr_t address = reinterpret_cast<uintptr_t>(packet);
        for (const TrackedFocusPacket& tracked : gNearFocusPackets)
        {
            if (tracked.address == address && SameFocusPacket(tracked.packet, *packet))
            {
                return true;
            }
        }

        return false;
    }

    void RestoreDefaultDepthFunc()
    {
        if (!gSetDepthFunc || !gDepthFuncBackend)
        {
            return;
        }

        gSetDepthFunc(gDepthFuncBackend, kDepthFuncGEqual);
    }

    void __fastcall FarFocusCommand_Hook(void* packet)
    {
        auto* focusPacket = static_cast<FocusSourcePacket*>(packet);
        const bool isNearFocusPacket = IsTrackedNearFocusPacket(focusPacket);
        const bool previousExecutingNearFocus = gExecutingNearFocusCommand;

        gExecutingNearFocusCommand = isNearFocusPacket;
        FarFocusCommandHook.fastcall<void>(packet);

        if (isNearFocusPacket)
        {
            RestoreDefaultDepthFunc();
            ConsumeNearFocusPacket(focusPacket);
            gInsideNearFocusComposite = false;
        }

        gExecutingNearFocusCommand = previousExecutingNearFocus;
    }

    bool QueueNearFocusPacketFromSource(FocusSourcePacket source)
    {
        if (!gBpRbAlloc || !gBpRbAddCommand || gInsideNearFocusAddCommand)
        {
            return false;
        }
    //    if (bCutsceneNeedsSpecialHandling)
        {
            if (bIsD12T3)
            {
                iNearEffectCount++;
                if (iNearEffectCount >= 80 && iNearEffectCount < 440)
                {
#ifndef RELEASE_BUILD
                    spdlog::info("MGS 2: Depth of Field: skipping near focus packet {:d} for cutscene special handling.", iNearEffectCount);
#endif
                    return false;
                }
            }
        }

        if (!PrepareFocusSource(source))
        {
            return false;
        }

        auto* packet = static_cast<FocusSourcePacket*>(gBpRbAlloc(sizeof(FocusSourcePacket)));
        if (!packet)
        {
            return false;
        }

        *packet = source;
        WidenNearFocusPacket(packet);
        TrackNearFocusPacket(packet);

        gInsideNearFocusAddCommand = true;
        gBpRbAddCommand(kCmdPostFxFarFocus, packet);
        gInsideNearFocusAddCommand = false;

        return true;
    }

    bool QueueNearFocusPacket(void* work)
    {
        FocusSourcePacket source {};
        if (!BuildNearFocusSourceFromWork(reinterpret_cast<uintptr_t>(work), source))
        {
            return false;
        }

        return QueueNearFocusPacketFromSource(source);
    }

    bool PrepareOriginalNearFocusCommand(FocusSourcePacket* packet)
    {
        if (!packet || !Memory::IsReadable(packet, sizeof(FocusSourcePacket)) || !Memory::IsWritable(packet, sizeof(FocusSourcePacket)))
        {
            return false;
        }

        FocusSourcePacket source = *packet;
        if (!PrepareFocusSource(source))
        {
            if (gActiveNearFocusSource)
            {
                source = *gActiveNearFocusSource;
            }
            else if (gActiveNearFocusWork)
            {
                if (!BuildNearFocusSourceFromWork(gActiveNearFocusWork, source))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        if (!PrepareFocusSource(source))
        {
            return false;
        }

        *packet = source;
        TrackNearFocusPacket(packet);
        return true;
    }

    void __fastcall BpRbAddCommand_Hook(unsigned int command, void* packet)
    {
        if (gInsideOriginalNearFocusCallback && command == kCmdPostFxFarFocus)
        {
            if (PrepareOriginalNearFocusCommand(static_cast<FocusSourcePacket*>(packet)))
            {
                gOriginalNearFocusCommandSeen = true;
            }
        }

        BpRbAddCommandHook.fastcall<void>(command, packet);
    }

    void* __fastcall NearFocusSet_Hook(int name, int map)
    {
        void* result = NearFocusSetHook.fastcall<void*>(name, map);
        TrackNearFocusWork(result);
        return result;
    }

    void* __fastcall NearFocusDemo_Hook(int id, void* argv)
    {
        void* result = NearFocusDemoHook.fastcall<void*>(id, argv);
        TrackNearFocusWork(result);
        return result;
    }

    uint8_t* FindFarFocusBlurCallSetup(const char* label)
    {
        return Memory::PatternScan(
            baseModule,
            "F3 44 0F 11 44 24 ?? E8 ?? ?? ?? ?? 48 8B 0D",
            label);
    }

    uint8_t* FindFarFocusMaxPlaneClamp(const char* label)
    {
        return Memory::PatternScan(
            baseModule,
            "39 1D ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 0F 4C 1D ?? ?? ?? ?? 89 9D",
            label);
    }

    bool ResolveRenderBufferHelpers()
    {
        const auto commands = Memory::FindMultiplePatternMatches(
            baseModule,
            "B9 0C 00 00 00 E8 ?? ?? ?? ?? 8B 53 64 89 10 8B 53 70 89 50 04 48 8B D0 8B 4B 74 89 48 08 B9 3F 00 00 00 48 83 C4 20 5B E9");

        if (commands.empty())
        {
            spdlog::error("MGS 2: Depth of Field: focus packet command pattern scan failed.");
            return false;
        }

        uint8_t* allocCall = commands.front() + 0x05;
        uint8_t* addCommandJump = commands.front() + 0x28;
        if (*allocCall != 0xE8 || *addCommandJump != 0xE9)
        {
            spdlog::error("MGS 2: Depth of Field: render-buffer helper call sites were not found.");
            return false;
        }

        gBpRbAlloc = reinterpret_cast<BpRbAllocFn>(Memory::GetRelativeOffset(allocCall + 1));
        gBpRbAddCommand = reinterpret_cast<BpRbAddCommandFn>(Memory::GetRelativeOffset(addCommandJump + 1));

        if (!BpRbAddCommandHook)
        {
            BpRbAddCommandHook = safetyhook::create_inline(
                reinterpret_cast<void*>(gBpRbAddCommand),
                reinterpret_cast<void*>(BpRbAddCommand_Hook));
            LOG_HOOK(BpRbAddCommandHook, "MGS 2: Depth of Field: render-buffer add command")
        }

        spdlog::info("MGS 2: Depth of Field: render-buffer helpers resolved at {:s}+{:X} and {:s}+{:X}.",
                     sExeName.c_str(),
                     ModuleOffset(reinterpret_cast<uintptr_t>(gBpRbAlloc)),
                     sExeName.c_str(),
                     ModuleOffset(reinterpret_cast<uintptr_t>(gBpRbAddCommand)));
        return true;
    }

    void InstallNearFocusCreationHooks()
    {
        const std::vector<uintptr_t> nearSetFunctions = FindStageEntryFunctions(kNearFocusSetId);
        if (!nearSetFunctions.empty())
        {
            NearFocusSetHook = safetyhook::create_inline(reinterpret_cast<void*>(nearSetFunctions.front()), reinterpret_cast<void*>(NearFocusSet_Hook));
            LOG_HOOK(NearFocusSetHook, "MGS 2: Depth of Field: near focus set")
        }
        else
        {
            spdlog::warn("MGS 2: Depth of Field: near focus set stage entry was not found.");
        }

        const std::vector<uintptr_t> nearDemoFunctions = FindStageEntryFunctions(kNearFocusDemoId);
        if (!nearDemoFunctions.empty())
        {
            NearFocusDemoHook = safetyhook::create_inline(reinterpret_cast<void*>(nearDemoFunctions.front()), reinterpret_cast<void*>(NearFocusDemo_Hook));
            LOG_HOOK(NearFocusDemoHook, "MGS 2: Depth of Field: near focus demo")
        }
        else
        {
            spdlog::warn("MGS 2: Depth of Field: near focus demo stage entry was not found.");
        }
    }

    void __fastcall SetVertexRegisters_Hook(void* backend, int startRegister, int numVectors, const float* regs)
    {
        const auto registerRange = GetBlurUvRegisterRange();
        thread_local float scaledRegs[4];
        if (gInsideFarFocusBlur &&
            startRegister > registerRange.first &&
            startRegister <= registerRange.second &&
            numVectors == 1 &&
            LooksLikeBlurUvOffset(regs))
        {
            std::copy(regs, regs + 4, scaledRegs);
            ScaleActiveBlurAxis(scaledRegs, g_DepthOfFieldFixes.fBlurUvMultiplier);
            SetVertexRegistersHook.fastcall<void>(backend, startRegister, numVectors, scaledRegs);
            return;
        }

        SetVertexRegistersHook.fastcall<void>(backend, startRegister, numVectors, regs);
    }

    void InstallFarFocusBlurScopeHooks()
    {
        uint8_t* farFocusBlurCallSetup = FindFarFocusBlurCallSetup("MGS 2: Depth of Field: far focus blur scope");

        if (!farFocusBlurCallSetup)
        {
            return;
        }

        FarFocusBlurBeginHook = safetyhook::create_mid(farFocusBlurCallSetup, [](SafetyHookContext&) {
            gInsideFarFocusBlur = true;
        });
        LOG_HOOK(FarFocusBlurBeginHook, "MGS 2: Depth of Field: far focus blur scope begin")

        FarFocusBlurEndHook = safetyhook::create_mid(farFocusBlurCallSetup + 0x0C, [](SafetyHookContext&) {
            gInsideFarFocusBlur = false;
            gInsideNearFocusComposite = false;
        });
        LOG_HOOK(FarFocusBlurEndHook, "MGS 2: Depth of Field: far focus blur scope end")
    }

    bool InstallNearFocusDepthFuncHook()
    {
        uint8_t* maxPlaneClamp = FindFarFocusMaxPlaneClamp("MGS 2: Depth of Field: near focus depth function");
        if (!maxPlaneClamp)
        {
            return false;
        }

        uint8_t* setDepthFuncCall = maxPlaneClamp - 0x1B;
        if (*setDepthFuncCall != 0xE8)
        {
            spdlog::error("MGS 2: Depth of Field: expected far-focus SetDepthFunc call was not found; near focus disabled.");
            return false;
        }

        gSetDepthFunc = reinterpret_cast<SetDepthFuncFn>(Memory::GetRelativeOffset(setDepthFuncCall + 1));

        NearFocusDepthFuncHook = safetyhook::create_mid(setDepthFuncCall, [](SafetyHookContext& ctx) {
            auto* packet = reinterpret_cast<FocusSourcePacket*>(ctx.r13);
            if (gExecutingNearFocusCommand || IsTrackedNearFocusPacket(packet))
            {
                gDepthFuncBackend = reinterpret_cast<void*>(ctx.rcx);
                ctx.rdx = kDepthFuncLEqual;
                gInsideNearFocusComposite = true;
            }
        });
        LOG_HOOK(NearFocusDepthFuncHook, "MGS 2: Depth of Field: near focus depth function")

        return static_cast<bool>(NearFocusDepthFuncHook);
    }

    bool InstallFarFocusCommandHook()
    {
        uint8_t* maxPlaneClamp = FindFarFocusMaxPlaneClamp("MGS 2: Depth of Field: far focus command");
        if (!maxPlaneClamp)
        {
            return false;
        }

        uint8_t* farFocusCommand = FindFunctionStart(maxPlaneClamp);
        if (!farFocusCommand)
        {
            spdlog::warn("MGS 2: Depth of Field: far focus command function start was not found; depth restore disabled.");
            return false;
        }

        FarFocusCommandHook = safetyhook::create_inline(farFocusCommand, reinterpret_cast<void*>(FarFocusCommand_Hook));
        LOG_HOOK(FarFocusCommandHook, "MGS 2: Depth of Field: far focus command")
        spdlog::info("MGS 2: Depth of Field: far focus command hook target {:s}+{:X}.",
                     sExeName.c_str(),
                     ModuleOffset(reinterpret_cast<uintptr_t>(farFocusCommand)));

        return static_cast<bool>(FarFocusCommandHook);
    }

    void ForceFarFocusBlurEnabled()
    {
        uint8_t* blurGate = Memory::PatternScan(
            baseModule,
            "83 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 45 0F 28 D4",
            "MGS 2: Depth of Field: far focus blur enable gate");

        if (!blurGate)
        {
            return;
        }

        uintptr_t blurEnableAddress = Memory::GetRipRelativeAddress(blurGate, 0x02, 0x07);
        Memory::Write<int>(blurEnableAddress, 1);

        spdlog::info("MGS 2: Depth of Field: far focus blur enabled at {:s}+{:X}.",
                     sExeName.c_str(),
                     blurEnableAddress - reinterpret_cast<uintptr_t>(baseModule));
    }

    void ForceFarFocusMaxPlaneCount()
    {
        uint8_t* maxPlaneClamp = FindFarFocusMaxPlaneClamp("MGS 2: Depth of Field: far focus max plane count");

        if (!maxPlaneClamp)
        {
            return;
        }

        uintptr_t maxPlaneCountAddress = Memory::GetRipRelativeAddress(maxPlaneClamp, 0x02, 0x06);
        Memory::Write<int>(maxPlaneCountAddress, kFarFocusMaxPlaneCount);
        Memory::Write<float>(maxPlaneCountAddress + 0x04, kFarFocusBlurWeight0);
        Memory::Write<float>(maxPlaneCountAddress + 0x08, kFarFocusBlurWeight12);
        Memory::Write<float>(maxPlaneCountAddress + 0x0C, kFarFocusBlurWeight34);
        Memory::Write<float>(maxPlaneCountAddress + 0x10, kFarFocusBlurWeight56);

        spdlog::info("MGS 2: Depth of Field: far focus max plane count set to {} at {:s}+{:X}.",
                     kFarFocusMaxPlaneCount,
                     sExeName.c_str(),
                     maxPlaneCountAddress - reinterpret_cast<uintptr_t>(baseModule));
        spdlog::info("MGS 2: Depth of Field: far focus blur weights set to {}, {}, {}, {}.",
                     kFarFocusBlurWeight0,
                     kFarFocusBlurWeight12,
                     kFarFocusBlurWeight34,
                     kFarFocusBlurWeight56);
    }

    void InstallBlurUvScaleHook()
    {
        uint8_t* blurUvRegisterUpload = Memory::PatternScan(
            baseModule,
            "48 8B 0D ?? ?? ?? ?? 4C 8D 4D ?? 8D 57 60 44 8D 47 01 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 4C 8D 4C 24 ?? 8D 57 61 44 8D 47 01 E8",
            "MGS 2: Depth of Field: blur UV register upload");

        if (!blurUvRegisterUpload)
        {
            return;
        }

        uint8_t* setVertexRegistersCall = blurUvRegisterUpload + 0x12;
        if (*setVertexRegistersCall != 0xE8)
        {
            spdlog::error("MGS 2: Depth of Field: expected SetVertexRegisters call was not found; blur UV scale disabled.");
            return;
        }

        const uintptr_t setVertexRegisters = Memory::GetRelativeOffset(setVertexRegistersCall + 1);
        SetVertexRegistersHook = safetyhook::create_inline(reinterpret_cast<void*>(setVertexRegisters), reinterpret_cast<void*>(SetVertexRegisters_Hook));
        LOG_HOOK(SetVertexRegistersHook, "MGS 2: Depth of Field: blur UV register scale")
    }

    void InstallMGS3NearFocusWorkLinkHook()
    {
        uint8_t* nearFocusPacketReady = Memory::PatternScan(
            baseModule,
            "F3 0F 5C C6 E8 ?? ?? ?? ?? F3 0F 11 43 58 F3 0F 10 05 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 11 43 5C",
            "MGS 3: Depth of Field: nearfocus work link");

        if (!nearFocusPacketReady)
        {
            return;
        }

        uint8_t* hookSite = nearFocusPacketReady + 0x20;
        if (hookSite[0] != 0x45 || hookSite[1] != 0x33 || hookSite[2] != 0xC0)
        {
            spdlog::error("MGS 3: Depth of Field: expected nearfocus post-depth write hook site was not found; synthetic nearfocus packets disabled.");
            return;
        }

        MGS3NearFocusWorkLinkHook = safetyhook::create_mid(hookSite, [](SafetyHookContext& ctx) {
            LinkMGS3NearFocusWorkToCallbackEntry(ctx.rbx, static_cast<int>(ctx.rdi & 0xffffffff));
        });
        LOG_HOOK(MGS3NearFocusWorkLinkHook, "MGS 3: Depth of Field: nearfocus work link")

        if (MGS3NearFocusWorkLinkHook)
        {
            spdlog::info("MGS 3: Depth of Field: nearfocus work link hook installed at {:s}+{:X}.",
                         sExeName.c_str(),
                         hookSite - reinterpret_cast<uint8_t*>(baseModule));
        }
    }

    void InstallMGS3FarFocusDispatchHook()
    {
        uint8_t* farFocusDispatch = Memory::PatternScan(
            baseModule,
            "49 8B 4E 08 E8 ?? ?? ?? ?? EB 6A 49 8B 4E 08 E8 ?? ?? ?? ?? EB 5F",
            "MGS 3: Depth of Field: far focus dispatcher");

        if (!farFocusDispatch)
        {
            return;
        }

        MGS3FarFocusDispatchHook = safetyhook::create_mid(farFocusDispatch, [](SafetyHookContext& ctx) {
            const uintptr_t commandNode = ctx.r14;
            auto* packet = Memory::ReadField<MGS3FarFocusPacket*>(commandNode, 0x08, nullptr);
            if (!Memory::IsReadable(packet, sizeof(MGS3FarFocusPacket)) ||
                !IsReasonableMGS3FarFocusPacket(packet))
            {
                return;
            }

            TrackMGS3FocusPacket(packet);
        });
        LOG_HOOK(MGS3FarFocusDispatchHook, "MGS 3: Depth of Field: far focus dispatcher")

        if (MGS3FarFocusDispatchHook)
        {
            spdlog::info("MGS 3: Depth of Field: far focus dispatcher hook installed at {:s}+{:X}.",
                         sExeName.c_str(),
                         farFocusDispatch - reinterpret_cast<uint8_t*>(baseModule));
        }
    }

    void InstallMGS3FarFocusCompositeDrawHook()
    {
        uint8_t* compositeDrawSetup = Memory::PatternScan(
            baseModule,
            "48 8B 44 35 B0 48 8D 55 80 48 8B 4C 35 A0 0F 28 C7 F3 0F 5C C1 F3 0F 11 54 24 28 45 33 C9 48 89 44 24 20 45 8D 41 01 F3 0F 11 45 8C E8 ?? ?? ?? ??",
            "MGS 3: Depth of Field: far focus composite draw");

        if (!compositeDrawSetup)
        {
            return;
        }

        uint8_t* drawCall = compositeDrawSetup + 0x2C;
        if (*drawCall != 0xE8)
        {
            spdlog::error("MGS 3: Depth of Field: expected far-focus composite draw call was not found; composite injection disabled.");
            return;
        }

        gMGS3FarFocusCompositeDrawReturn = reinterpret_cast<uintptr_t>(drawCall + 0x05);
        MGS3FarFocusCompositeDrawHook = safetyhook::create_mid(drawCall, [](SafetyHookContext& ctx) {
            auto* packet = reinterpret_cast<MGS3FarFocusPacket*>(ctx.r15);
            const auto* tracked = FindTrackedMGS3FocusPacketConst(reinterpret_cast<uintptr_t>(packet));
            if (tracked ? !tracked->applyFix : !ShouldApplyTrackedMGS3FocusFix(packet))
            {
                return;
            }

            const int sourceTexture = static_cast<int>(ctx.r14 & 0xffffffff);
            const float z = Memory::ReadField<float>(ctx.rsp, 0x28, std::numeric_limits<float>::quiet_NaN());

            auto* alphaSlot = reinterpret_cast<float*>(ctx.rbp - 0x74);
            if (Memory::IsWritable(alphaSlot, sizeof(*alphaSlot)))
            {
                *alphaSlot = 0.0f;
            }

            if (sourceTexture == 0 && std::isfinite(z))
            {
                MGS3FarFocusPacket nearPacket {};
                const bool hasNearPacket = ConsumeMGS3PendingNearFocusPacket(nearPacket);
                DrawMGS3CombinedFocusSamples(ctx.rsp, packet, hasNearPacket ? &nearPacket : nullptr);
            }

            if (gMGS3FarFocusCompositeDrawReturn)
            {
                ctx.rip = gMGS3FarFocusCompositeDrawReturn;
            }
        });
        LOG_HOOK(MGS3FarFocusCompositeDrawHook, "MGS 3: Depth of Field: far focus composite draw")

        if (MGS3FarFocusCompositeDrawHook)
        {
            spdlog::info("MGS 3: Depth of Field: far focus composite draw installed at {:s}+{:X}.",
                         sExeName.c_str(),
                         compositeDrawSetup - reinterpret_cast<uint8_t*>(baseModule));
        }
    }

    void DisableMGS3FarFocusBlur()
    {
        uint8_t* blurGate = Memory::PatternScan(
            baseModule,
            "44 39 35 ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 0F 28 F7 F3 0F 10 1D ?? ?? ?? ?? B9 0B 00 00 00",
            "MGS 3: Depth of Field: far focus blur enable gate");

        if (!blurGate)
        {
            return;
        }

        uintptr_t blurEnableAddress = Memory::GetRipRelativeAddress(blurGate, 0x03, 0x07);
        Memory::Write<int>(blurEnableAddress, 0);

        spdlog::info("MGS 3: Depth of Field: far focus blur disabled at {:s}+{:X}.",
                     sExeName.c_str(),
                     blurEnableAddress - reinterpret_cast<uintptr_t>(baseModule));
    }

    void InstallMGS3FarFocusBlurScopeHooks()
    {
        uint8_t* farFocusBlurCallSetup = Memory::PatternScan(
            baseModule,
            "48 8D 4D 90 4C 8B C0 48 8B D6 48 89 4C 24 58 48 8D 4C 24 70 48 89 4C 24 50 48 8B CE C6 44 24 48 01 F3 0F 10 05 ?? ?? ?? ?? F3 0F 10 0D ?? ?? ?? ?? F3 0F 10 1D ?? ?? ?? ?? F3 0F 11 74 24 40 F3 0F 11 74 24 38 F3 0F 11 44 24 30 F3 0F 10 05 ?? ?? ?? ?? F3 0F 11 4C 24 28 F3 0F 11 44 24 20 E8",
            "MGS 3: Depth of Field: far focus blur scope");

        if (!farFocusBlurCallSetup)
        {
            return;
        }

        uint8_t* blurCall = farFocusBlurCallSetup + kMgs3FarFocusBlurCallOffset;
        if (*blurCall != 0xE8)
        {
            spdlog::error("MGS 3: Depth of Field: expected far-focus blur call was not found; blur UV scale disabled.");
            return;
        }

        FarFocusBlurBeginHook = safetyhook::create_mid(farFocusBlurCallSetup, [](SafetyHookContext&) {
            gInsideFarFocusBlur = true;
        });
        LOG_HOOK(FarFocusBlurBeginHook, "MGS 3: Depth of Field: far focus blur scope begin")

        FarFocusBlurEndHook = safetyhook::create_mid(blurCall + 0x05, [](SafetyHookContext&) {
            gInsideFarFocusBlur = false;
        });
        LOG_HOOK(FarFocusBlurEndHook, "MGS 3: Depth of Field: far focus blur scope end")

        if (FarFocusBlurBeginHook && FarFocusBlurEndHook)
        {
            spdlog::info("MGS 3: Depth of Field: far focus blur scope installed at {:s}+{:X}.",
                         sExeName.c_str(),
                         farFocusBlurCallSetup - reinterpret_cast<uint8_t*>(baseModule));
        }
    }

    void InstallMGS3BlurUvScaleHook()
    {
        uint8_t* blurUvRegisterUpload = Memory::PatternScan(
            baseModule,
            "48 8B 0D ?? ?? ?? ?? 4C 8D 4C 24 60 8D 57 1E 44 8D 47 01 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 4C 8D 4C 24 70 8D 57 1F 44 8D 47 01 E8",
            "MGS 3: Depth of Field: blur UV register upload");

        if (!blurUvRegisterUpload)
        {
            return;
        }

        uint8_t* setVertexRegistersCall = blurUvRegisterUpload + kMgs3BlurUvRegisterUploadCallOffset;
        if (*setVertexRegistersCall != 0xE8)
        {
            spdlog::error("MGS 3: Depth of Field: expected SetVertexRegisters call was not found; blur UV scale disabled.");
            return;
        }

        const uintptr_t setVertexRegisters = Memory::GetRelativeOffset(setVertexRegistersCall + 1);
        SetVertexRegistersHook = safetyhook::create_inline(reinterpret_cast<void*>(setVertexRegisters), reinterpret_cast<void*>(SetVertexRegisters_Hook));
        LOG_HOOK(SetVertexRegistersHook, "MGS 3: Depth of Field: blur UV register scale")

        if (SetVertexRegistersHook)
        {
            spdlog::info("MGS 3: Depth of Field: blur UV register scale installed at {:s}+{:X}.",
                         sExeName.c_str(),
                         setVertexRegisters - reinterpret_cast<uintptr_t>(baseModule));
        }
    }

}

void DepthOfFieldFixes::HandleLevelTransition() const
{
    if (!bEnabled)
    {
        return;
    }
    iNearEffectCount = 0;
    bIsD12T3 = (eGameType & MGS2) && g_GameVars.IsStage(MGS2Stages::D12T3);
}

void DepthOfFieldFixes::OnPresent()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    ++gMGS3DofFrameIndex;
    gMGS3DofFocusSourceFrameTarget.Reset();
    gMGS3DofFocusSourceFrameIndex = UINT64_MAX;
}

void DepthOfFieldFixes::Initialize()
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }

    const char* gameLabel = (eGameType & MGS3) ? "MGS 3" : "MGS 2";
    if (!bEnabled)
    {
        spdlog::info("{}: Depth of Field: disabled by config.", gameLabel);
        return;
    }

    if (IsUltrawide())
    {
        bEnabled = false;
        spdlog::info("{}: Depth of Field: disabled for ultrawide aspect ratio.", gameLabel);
        return;
    }

    fBlurUvMultiplier = std::clamp(fBlurUvMultiplier, 0.0f, 20.0f);
    spdlog::info("{}: Depth of Field: blur UV multiplier set to {}.", gameLabel, fBlurUvMultiplier);

    if (eGameType & MGS2)
    {
        ForceFarFocusBlurEnabled();
        ForceFarFocusMaxPlaneCount();
        InstallFarFocusBlurScopeHooks();
        if (ResolveRenderBufferHelpers() && InstallNearFocusDepthFuncHook())
        {
            InstallFarFocusCommandHook();
            InstallNearFocusCreationHooks();
        }
        else
        {
            spdlog::warn("MGS 2: Depth of Field: failed to resolve necessary functions for near focus fixes; near focus adjustments disabled.");
        }
        InstallBlurUvScaleHook();

#ifndef RELEASE_BUILD
        g_InputHandler.RegisterHotkey(VK_ADD, "print iNearEffectCount", []
                                      {
                                          spdlog::info("iNearEffectCount = {}, bIsD12T3 = {}", iNearEffectCount, bIsD12T3);
                                      });

        /*
        MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 41 8B F9 41 8B F0 45 33 C9 8B EA 44 8B F1 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 A4 1C B2 FF", "NewNearFocusEffect -> Focal Points", {
        spdlog::info("ctx.r8 = {}, ctx.r9 = {}", ctx.r8, ctx.r9);
        ctx.r9 = 0;
        ctx.r8 = 4000;
        //r8 = var_near
        //r9 = var_far
        })*/
#endif

        return;
    }

    DisableMGS3FarFocusBlur();

    InstallMGS3NearFocusWorkLinkHook();
    InstallMGS3FarFocusDispatchHook();
    InstallMGS3FarFocusCompositeDrawHook();
}
