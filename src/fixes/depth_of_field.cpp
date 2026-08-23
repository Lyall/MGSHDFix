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
    constexpr int kFocusMaxPlaneCount = 64;
    constexpr UINT kFocusPyramidMipCount = 7;
    constexpr float kMGS3FocusSpreadBoost = 2.75f;
    constexpr uint64_t kMGS3PendingNearFocusMaxFrameAge = 2;
    constexpr float kNearFocusMinDepthSpan = 0.00025f;
    constexpr int kNearFocusMinPlaneCount = 6;
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
    // f_focus.c blends each plane at FIX 128 and shifts the frame one 448-line texel per plane, so the
    // far end is fully covered by a blur of one PS2 texel sigma.
    constexpr int kMGS2FocusAlpha = 64;
    constexpr int kMGS2FocusAlphaBoost = 32;   // past strength 20 the layers also thicken, up to this much at 30
    constexpr float kMGS2FocusSpreadPerHeight = 0.005f;

    using BpRbAllocFn = void*(__fastcall*)(int);
    using BpRbAddCommandFn = void(__fastcall*)(unsigned int, void*);
    using DmapackRenderCallbackFn = void(__fastcall*)(void*);

    bool bCutsceneNeedsSpecialHandling = false; //for per-cutscene effect skip handling.
    bool bIsD12T3 = false;
    int iNearEffectCount = 0;


    struct FocusSourcePacket
    {
        int maxPlane;
        float focusNear;
        float focusFar;
    };

    struct DofFocusPacket
    {
        int alpha;
        int maxPlane;
        float focusNear;
        float focusFar;
    };

    struct PendingMGS3FocusPacket
    {
        DofFocusPacket packet {};
        uint64_t frameIndex = 0;
        bool valid = false;
    };

    struct FocusRect
    {
        int x1;
        int y1;
        int x2;
        int y2;
    };

    enum class FocusSide
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
        DofFocusPacket packet {};
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
    ComPtr<ID3D11VertexShader> gDofFocusVS;
    ComPtr<ID3D11PixelShader> gDofDepthPS;
    ComPtr<ID3D11PixelShader> gDofDownsamplePS;
    ComPtr<ID3D11PixelShader> gDofCocPS;
    ComPtr<ID3D11PixelShader> gDofDilateHPS;
    ComPtr<ID3D11PixelShader> gDofDilateVPS;
    ComPtr<ID3D11PixelShader> gDofGatherPS;
    ComPtr<ID3D11PixelShader> gDofUpsamplePS;
    ComPtr<ID3D11Texture2D> gDofCocTexture;
    ComPtr<ID3D11RenderTargetView> gDofCocRTV;
    ComPtr<ID3D11ShaderResourceView> gDofCocSRV;
    ComPtr<ID3D11Texture2D> gDofCocScratchTexture;
    ComPtr<ID3D11RenderTargetView> gDofCocScratchRTV;
    ComPtr<ID3D11ShaderResourceView> gDofCocScratchSRV;
    UINT gDofCocWidth = 0;
    UINT gDofCocHeight = 0;
    ComPtr<ID3D11Texture2D> gDofGatherTexture;
    ComPtr<ID3D11RenderTargetView> gDofGatherRTV;
    ComPtr<ID3D11ShaderResourceView> gDofGatherSRV;
    UINT gDofGatherWidth = 0;
    UINT gDofGatherHeight = 0;
    ComPtr<ID3D11Buffer> gDofFocusConstants;
    ComPtr<ID3D11Texture2D> gDofFocusSourceTexture;
    ComPtr<ID3D11ShaderResourceView> gDofFocusSourceSRV;
    std::vector<ComPtr<ID3D11ShaderResourceView>> gDofFocusMipSRVs;
    std::vector<ComPtr<ID3D11RenderTargetView>> gDofFocusMipRTVs;
    ComPtr<ID3D11Resource> gDofFocusSourceFrameTarget;
    ComPtr<ID3D11Resource> gDofFocusDirectSource;
    ComPtr<ID3D11ShaderResourceView> gDofFocusDirectSRV;
    uint64_t gDofFocusDirectFrameIndex = UINT64_MAX;
    // A multisampled game target is resolved here first; its depth is copied as-is and read per sample.
    ComPtr<ID3D11Texture2D> gDofResolveTexture;
    D3D11_TEXTURE2D_DESC gDofResolveDesc {};
    ComPtr<ID3D11Texture2D> gDofDepthMSCopy;
    ComPtr<ID3D11ShaderResourceView> gDofDepthMSSRV;
    D3D11_TEXTURE2D_DESC gDofDepthMSDesc {};
    ComPtr<ID3D11PixelShader> gDofCocMSPS;
    bool gDofDepthMultisampled = false;
    ComPtr<ID3D11SamplerState> gDofFocusSampler;
    ComPtr<ID3D11RasterizerState> gDofFocusRasterizerState;
    ComPtr<ID3D11DepthStencilState> gDofFocusDepthDisabledState;
    ComPtr<ID3D11BlendState> gDofFocusBlendState;
    ComPtr<ID3D11BlendState> gDofFocusPremultBlendState;
    D3D11_TEXTURE2D_DESC gDofFocusSourceDesc {};
    UINT gDofFocusLogicalWidth = 0;
    UINT gDofFocusLogicalHeight = 0;
    float gDofFocusLodBias = 0.0f;
    ID3D11Device* gDofFocusDevice = nullptr;
    uint64_t gDofFrameIndex = 0;
    uint64_t gDofFocusSourceFrameIndex = UINT64_MAX;
    UINT gDofFocusSourceMipCount = 1;
    bool gDofFocusHasPyramid = false;
    bool gDofFocusReady = false;
    bool gDofFocusFailed = false;

    struct DofFocusConstants
    {
        float sourceRect[4] {};
        float sourceSizeAndSpread[4] {};
        float color[4] {};
        float planeData[2][4] {};
        float depthSize[4] {};
    };

    uintptr_t gMGS3FarFocusCompositeDrawReturn = 0;
    uint64_t gMGS3LastFarCompositeFrame = UINT64_MAX;

    // MGS3 render-command emitters, used to queue a far-focus command from the
    // nearfocus callback so near-only blur executes in the post-fx slot (before the
    // 2D layer) instead of over the finished frame.
    using MGS3RbAllocFn = void*(__fastcall*)(unsigned int size);
    using MGS3RbPushFn = void(__fastcall*)(unsigned int type, void* payload);
    constexpr unsigned int kMGS3CmdFarFocus = 0x31;
    MGS3RbAllocFn gMGS3RbAlloc = nullptr;
    MGS3RbPushFn gMGS3RbPush = nullptr;

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
    bool IsReasonableFocusRect(const FocusRect& rect);
    void DrawMGS3NearOnlyFocusPass();

    void ResetDofRenderer()
    {
        gDofFocusVS.Reset();
        gDofDepthPS.Reset();
        gDofDownsamplePS.Reset();
        gDofCocPS.Reset();
        gDofDilateHPS.Reset();
        gDofDilateVPS.Reset();
        gDofGatherPS.Reset();
        gDofUpsamplePS.Reset();
        gDofCocTexture.Reset();
        gDofCocRTV.Reset();
        gDofCocSRV.Reset();
        gDofCocScratchTexture.Reset();
        gDofCocScratchRTV.Reset();
        gDofCocScratchSRV.Reset();
        gDofCocWidth = 0;
        gDofCocHeight = 0;
        gDofGatherTexture.Reset();
        gDofGatherRTV.Reset();
        gDofGatherSRV.Reset();
        gDofGatherWidth = 0;
        gDofGatherHeight = 0;
        gDofFocusConstants.Reset();
        gDofFocusSourceTexture.Reset();
        gDofFocusSourceSRV.Reset();
        gDofFocusMipSRVs.clear();
        gDofFocusMipRTVs.clear();
        gDofFocusSourceFrameTarget.Reset();
        gDofFocusDirectSource.Reset();
        gDofFocusDirectSRV.Reset();
        gDofFocusDirectFrameIndex = UINT64_MAX;
        gDofResolveTexture.Reset();
        gDofResolveDesc = {};
        gDofDepthMSCopy.Reset();
        gDofDepthMSSRV.Reset();
        gDofDepthMSDesc = {};
        gDofCocMSPS.Reset();
        gDofFocusSampler.Reset();
        gDofFocusRasterizerState.Reset();
        gDofFocusDepthDisabledState.Reset();
        gDofFocusBlendState.Reset();
        gDofFocusPremultBlendState.Reset();
        gDofFocusSourceDesc = {};
        gDofFocusLogicalWidth = 0;
        gDofFocusLogicalHeight = 0;
        gDofFocusLodBias = 0.0f;
        gDofFocusDevice = nullptr;
        gDofFocusSourceFrameIndex = UINT64_MAX;
        gDofFocusSourceMipCount = 1;
        gDofFocusHasPyramid = false;
        gDofFocusReady = false;
    }

    bool EnsureDofRenderer()
    {
        ID3D11Device* device = g_D3D11Hooks.d3dDevice.Get();
        if (!device)
        {
            return false;
        }

        if (gDofFocusDevice != device)
        {
            ResetDofRenderer();
            gDofFocusDevice = device;
            gDofFocusFailed = false;
        }

        if (gDofFocusReady)
        {
            return true;
        }

        if (gDofFocusFailed)
        {
            return false;
        }

        if (!g_D3D11Hooks.D3DCompileFunc)
        {
            gDofFocusFailed = true;
            spdlog::warn("Depth of Field: depth focus shader unavailable; D3DCompile missing.");
            return false;
        }

        const char* shader = R"(
            cbuffer FocusConstants : register(b0)
            {
                float4 sourceRect;
                float4 sourceSizeAndSpread;
                float4 focusColor;
                float4 planeData[2];
                float4 depthSize;
            };

            Texture2D focusSource : register(t0);
            Texture2D sceneDepth : register(t1);
            Texture2D cocSource : register(t2);
            Texture2D gatherSource : register(t3);
            Texture2DMS<float> sceneDepthMS : register(t4);
            SamplerState focusSampler : register(s0);

            static const float kNearEdgeAlphaScale = 0.58;
            static const float kNearEdgeSpreadScale = 0.74;
            static const float kNearEdgeRadiusScale = 0.15;
            static const float kNearEdgeMaxRadius = 7.0;
            static const float kNearEdgeSpillScale = 0.62;

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

                // depthSize.z carries the pyramid's root level; never sample the finest mip.
                float lod = clamp(log2(max(spread * 0.30, 2.0)) - depthSize.z, 1.0 - depthSize.z, maxMip);
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

            float PlaneWidthScale(float4 packet)
            {
                return (min(packet.w, 24.0) / 8.0) * (0.6 + 0.8 * saturate(packet.z));
            }

            // Half-res CoC: far amount in x, near amount in y, dilatable near copy in z.
            float4 CocPS(VSOut input) : SV_Target
            {
                float2 depthDims = max(depthSize.xy, float2(1.0, 1.0));
                int2 depthPixel = int2(clamp(input.uv * depthDims, float2(0.0, 0.0), depthDims - 1.0));
                float depth = sceneDepth.Load(int3(depthPixel, 0)).r;
                float farAmount = FocusRangeAmount(depth, planeData[0], false);
                float nearAmount = FocusRangeAmount(depth, planeData[1], true);
                return float4(farAmount, nearAmount, nearAmount, 0.0);
            }

            float4 CocMSPS(VSOut input) : SV_Target
            {
                float2 depthDims = max(depthSize.xy, float2(1.0, 1.0));
                int2 depthPixel = int2(clamp(input.uv * depthDims, float2(0.0, 0.0), depthDims - 1.0));
                float depth = sceneDepthMS.Load(depthPixel, 0);
                float farAmount = FocusRangeAmount(depth, planeData[0], false);
                float nearAmount = FocusRangeAmount(depth, planeData[1], true);
                return float4(farAmount, nearAmount, nearAmount, 0.0);
            }

            // Widen near CoC at foreground edges so silhouettes blend into the scene.
            float NearDilateRadius()
            {
                float2 sourceSize = max(sourceSizeAndSpread.xy, float2(1.0, 1.0));
                float2 depthDims = max(depthSize.xy, float2(1.0, 1.0));
                float nearSpread = focusColor.a * PlaneWidthScale(planeData[1]) * (depthDims.x / sourceSize.x);
                return clamp(nearSpread * kNearEdgeRadiusScale, 1.0, kNearEdgeMaxRadius) * 0.5;
            }

            float3 DilateNear(float2 uv, float2 direction)
            {
                float3 coc = cocSource.SampleLevel(focusSampler, uv, 0).xyz;
                if (planeData[1].z <= 0.0 || focusColor.a <= 0.0)
                {
                    return coc;
                }

                float2 cocDims = max(depthSize.xy * 0.5, float2(1.0, 1.0));
                float2 step = direction * (NearDilateRadius() / cocDims);

                [unroll]
                for (int i = 1; i <= 4; ++i)
                {
                    float reach = float(i) / 4.0;
                    coc.z = max(coc.z, cocSource.SampleLevel(focusSampler, uv + step * reach, 0).z * kNearEdgeSpillScale);
                    coc.z = max(coc.z, cocSource.SampleLevel(focusSampler, uv - step * reach, 0).z * kNearEdgeSpillScale);
                }

                return coc;
            }

            float4 DilateHPS(VSOut input) : SV_Target
            {
                return float4(DilateNear(input.uv, float2(1.0, 0.0)), 0.0);
            }

            float4 DilateVPS(VSOut input) : SV_Target
            {
                return float4(DilateNear(input.uv, float2(0.0, 1.0)), 0.0);
            }

            // Plane count and per-plane alpha carry the scene's authored blur
            // strength; 8 planes at alpha 64 is the reference width.
            float4 ComputeFocusSample(float2 sourceUv, float2 sourceSize)
            {
                float farPlaneScale = PlaneWidthScale(planeData[0]);
                float nearPlaneScale = PlaneWidthScale(planeData[1]);
                float nearSpreadLimit = focusColor.a * nearPlaneScale;

                float3 coc = cocSource.SampleLevel(focusSampler, sourceUv, 0).xyz;
                float farAmount = coc.x;
                float nearAmount = coc.y;
                float nearEdgeAmount = coc.z;
                float farAlpha = FocusPacketAlpha(farAmount, planeData[0]);
                float nearAlpha = FocusPacketAlpha(nearAmount, planeData[1]);
                float nearEdgeAlpha = FocusPacketAlpha(nearEdgeAmount, planeData[1]) * kNearEdgeAlphaScale;

                float alpha = max(farAlpha, max(nearAlpha, nearEdgeAlpha));
                if (alpha < 0.004)
                {
                    return float4(0.0, 0.0, 0.0, 0.0);
                }

                float farSpread = sourceSizeAndSpread.z * farPlaneScale * farAmount;
                float nearSpread = nearSpreadLimit * max(nearAmount, nearEdgeAmount * kNearEdgeSpreadScale);
                float spread = max(farSpread, nearSpread);
                float maxMip = sourceSizeAndSpread.w;
                float3 color = SampleBlurredSource(sourceUv, spread, maxMip, sourceSize) * focusColor.rgb;
                return float4(color, alpha);
            }

            float4 DepthFocusPS(VSOut input) : SV_Target
            {
                float2 basePixel = sourceRect.xy + input.uv * sourceRect.zw;
                float2 sourceSize = max(sourceSizeAndSpread.xy, float2(1.0, 1.0));
                float4 focus = ComputeFocusSample(basePixel / sourceSize, sourceSize);
                if (focus.a < 0.004)
                {
                    discard; // in focus; keep the original pixel and skip the blur taps
                }

                return focus;
            }

            // Premultiplied so the full-res upsample interpolates cleanly across coverage edges.
            float4 GatherPS(VSOut input) : SV_Target
            {
                float2 sourceSize = max(sourceSizeAndSpread.xy, float2(1.0, 1.0));
                float4 focus = ComputeFocusSample(input.uv, sourceSize);
                return float4(focus.rgb * focus.a, focus.a);
            }

            float4 UpsamplePS(VSOut input) : SV_Target
            {
                float2 basePixel = sourceRect.xy + input.uv * sourceRect.zw;
                float2 sourceSize = max(sourceSizeAndSpread.xy, float2(1.0, 1.0));
                float4 blurred = gatherSource.SampleLevel(focusSampler, basePixel / sourceSize, 0);
                if (blurred.a < 0.004)
                {
                    discard;
                }

                return blurred;
            }

            // 4x4 overlapping box per mip step; the pyramid converges on a gaussian
            // instead of the blocky 2x2 box chain GenerateMips leaves.
            float4 DownsamplePS(VSOut input) : SV_Target
            {
                float2 size;
                focusSource.GetDimensions(size.x, size.y);
                float2 texel = 1.0 / max(size, float2(1.0, 1.0));

                float3 color = focusSource.SampleLevel(focusSampler, input.uv + texel * float2(-1.0, -1.0), 0).rgb;
                color += focusSource.SampleLevel(focusSampler, input.uv + texel * float2( 1.0, -1.0), 0).rgb;
                color += focusSource.SampleLevel(focusSampler, input.uv + texel * float2(-1.0,  1.0), 0).rgb;
                color += focusSource.SampleLevel(focusSampler, input.uv + texel * float2( 1.0,  1.0), 0).rgb;
                return float4(color * 0.25, 1.0);
            }
        )";

        ComPtr<ID3DBlob> vsBlob;
        ComPtr<ID3DBlob> depthPsBlob;
        ComPtr<ID3DBlob> err;
        HRESULT hr = g_D3D11Hooks.D3DCompileFunc(shader, strlen(shader), nullptr, nullptr, nullptr, "FocusVS", "vs_5_0", 0, 0, vsBlob.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr))
        {
            gDofFocusFailed = true;
            spdlog::warn("Depth of Field: depth focus VS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
            return false;
        }

        err.Reset();
        hr = g_D3D11Hooks.D3DCompileFunc(shader, strlen(shader), nullptr, nullptr, nullptr, "DepthFocusPS", "ps_5_0", 0, 0, depthPsBlob.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr))
        {
            gDofFocusFailed = true;
            spdlog::warn("Depth of Field: depth focus PS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
            return false;
        }

        const auto compilePS = [&](const char* entry, ComPtr<ID3D11PixelShader>& out) {
            ComPtr<ID3DBlob> blob;
            err.Reset();
            if (FAILED(g_D3D11Hooks.D3DCompileFunc(shader, strlen(shader), nullptr, nullptr, nullptr, entry, "ps_5_0", 0, 0, blob.GetAddressOf(), err.GetAddressOf())) ||
                FAILED(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out.GetAddressOf())))
            {
                gDofFocusFailed = true;
                spdlog::warn("Depth of Field: {} compile failed: {}", entry, err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
                return false;
            }
            return true;
        };

        if (!compilePS("DownsamplePS", gDofDownsamplePS) ||
            !compilePS("CocPS", gDofCocPS) ||
            !compilePS("CocMSPS", gDofCocMSPS) ||
            !compilePS("DilateHPS", gDofDilateHPS) ||
            !compilePS("DilateVPS", gDofDilateVPS) ||
            !compilePS("GatherPS", gDofGatherPS) ||
            !compilePS("UpsamplePS", gDofUpsamplePS))
        {
            return false;
        }

        if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, gDofFocusVS.GetAddressOf())) ||
            FAILED(device->CreatePixelShader(depthPsBlob->GetBufferPointer(), depthPsBlob->GetBufferSize(), nullptr, gDofDepthPS.GetAddressOf())))
        {
            gDofFocusFailed = true;
            spdlog::warn("Depth of Field: depth focus shader creation failed.");
            return false;
        }

        D3D11_BUFFER_DESC constantsDesc {};
        constantsDesc.ByteWidth = sizeof(DofFocusConstants);
        constantsDesc.Usage = D3D11_USAGE_DEFAULT;
        constantsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(device->CreateBuffer(&constantsDesc, nullptr, gDofFocusConstants.GetAddressOf())))
        {
            gDofFocusFailed = true;
            spdlog::warn("Depth of Field: depth focus constants creation failed.");
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(device->CreateSamplerState(&samplerDesc, gDofFocusSampler.GetAddressOf())))
        {
            gDofFocusFailed = true;
            spdlog::warn("Depth of Field: depth focus sampler creation failed.");
            return false;
        }

        D3D11_RASTERIZER_DESC rasterizerDesc {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        if (FAILED(device->CreateRasterizerState(&rasterizerDesc, gDofFocusRasterizerState.GetAddressOf())))
        {
            gDofFocusFailed = true;
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC depthDesc {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        if (FAILED(device->CreateDepthStencilState(&depthDesc, gDofFocusDepthDisabledState.GetAddressOf())))
        {
            gDofFocusFailed = true;
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
        if (FAILED(device->CreateBlendState(&blendDesc, gDofFocusBlendState.GetAddressOf())))
        {
            gDofFocusFailed = true;
            return false;
        }

        // Premultiplied variant for the half-res gather composite.
        blendTarget.SrcBlend = D3D11_BLEND_ONE;
        if (FAILED(device->CreateBlendState(&blendDesc, gDofFocusPremultBlendState.GetAddressOf())))
        {
            gDofFocusFailed = true;
            return false;
        }

        gDofFocusReady = true;
        spdlog::info("Depth of Field: depth renderer initialized.");
        return true;
    }

    bool SameTextureDesc(const D3D11_TEXTURE2D_DESC& lhs, const D3D11_TEXTURE2D_DESC& rhs)
    {
        return lhs.Width == rhs.Width &&
            lhs.Height == rhs.Height &&
            lhs.MipLevels == rhs.MipLevels &&
            lhs.ArraySize == rhs.ArraySize &&
            lhs.Format == rhs.Format &&
            lhs.SampleDesc.Count == rhs.SampleDesc.Count &&
            lhs.SampleDesc.Quality == rhs.SampleDesc.Quality;
    }

    UINT GetDofMipCount(UINT width, UINT height, UINT maxLevels)
    {
        UINT mipCount = 1;
        while (mipCount < maxLevels && (width > 1 || height > 1))
        {
            width = std::max<UINT>(width / 2, 1);
            height = std::max<UINT>(height / 2, 1);
            ++mipCount;
        }

        return mipCount;
    }

    bool CanRenderDofMips(ID3D11Device* device, DXGI_FORMAT format)
    {
        UINT support = 0;
        if (!device || FAILED(device->CheckFormatSupport(format, &support)))
        {
            return false;
        }

        constexpr UINT requiredSupport = D3D11_FORMAT_SUPPORT_RENDER_TARGET |
            D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
        return (support & requiredSupport) == requiredSupport;
    }

    // Each mip renders from the level above; non-overlapping mip views make the
    // same-texture SRV/RTV pairing legal. directSourceSRV feeds mip 0 when set.
    bool BuildDofPyramid(ID3D11DeviceContext* context, ID3D11ShaderResourceView* directSourceSRV)
    {
        if (!context ||
            !gDofDownsamplePS ||
            gDofFocusMipSRVs.size() < gDofFocusSourceMipCount ||
            gDofFocusMipRTVs.size() < gDofFocusSourceMipCount)
        {
            return false;
        }

        ID3D11SamplerState* sampler = gDofFocusSampler.Get();
        context->IASetInputLayout(nullptr);
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(gDofFocusVS.Get(), nullptr, 0);
        context->GSSetShader(nullptr, nullptr, 0);
        context->PSSetShader(gDofDownsamplePS.Get(), nullptr, 0);
        context->PSSetSamplers(0, 1, &sampler);
        context->RSSetState(gDofFocusRasterizerState.Get());
        context->OMSetDepthStencilState(gDofFocusDepthDisabledState.Get(), 0);
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

        for (UINT mip = directSourceSRV ? 0 : 1; mip < gDofFocusSourceMipCount; ++mip)
        {
            const UINT mipWidth = std::max<UINT>(gDofFocusSourceDesc.Width >> mip, 1);
            const UINT mipHeight = std::max<UINT>(gDofFocusSourceDesc.Height >> mip, 1);

            ID3D11ShaderResourceView* sourceSRV = mip == 0 ? directSourceSRV : gDofFocusMipSRVs[mip - 1].Get();
            ID3D11RenderTargetView* targetRTV = gDofFocusMipRTVs[mip].Get();
            const D3D11_VIEWPORT viewport { 0.0f, 0.0f, static_cast<float>(mipWidth), static_cast<float>(mipHeight), 0.0f, 1.0f };
            context->OMSetRenderTargets(1, &targetRTV, nullptr);
            context->RSSetViewports(1, &viewport);
            context->PSSetShaderResources(0, 1, &sourceSRV);
            context->Draw(3, 0);

            ID3D11ShaderResourceView* nullSRV = nullptr;
            context->PSSetShaderResources(0, 1, &nullSRV);
        }

        context->OMSetRenderTargets(0, nullptr, nullptr);
        return true;
    }

    // Cached SRV over the game's own target; a failed (null) result is remembered too.
    ID3D11ShaderResourceView* EnsureDofDirectSRV(ID3D11Device* device, ID3D11Resource* target, const D3D11_TEXTURE2D_DESC& targetDesc)
    {
        if (!(targetDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) ||
            targetDesc.Width < 4 ||
            targetDesc.Height < 4)
        {
            return nullptr;
        }

        if (gDofFocusDirectSource.Get() != target)
        {
            gDofFocusDirectSRV.Reset();
            gDofFocusDirectSource = target;
            device->CreateShaderResourceView(target, nullptr, gDofFocusDirectSRV.GetAddressOf());
        }

        gDofFocusDirectFrameIndex = gDofFrameIndex;
        return gDofFocusDirectSRV.Get();
    }

    bool CopyDofSourceFromCurrentTarget(ID3D11RenderTargetView* currentTarget, bool reuseCurrentFrame = false)
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
        if (targetDesc.Width == 0 || targetDesc.Height == 0)
        {
            return false;
        }

        if (targetDesc.SampleDesc.Count > 1)
        {
            D3D11_TEXTURE2D_DESC resolveDesc = targetDesc;
            resolveDesc.SampleDesc = { 1, 0 };
            resolveDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            resolveDesc.MipLevels = 1;
            resolveDesc.MiscFlags = 0;
            if (!gDofResolveTexture || !SameTextureDesc(gDofResolveDesc, resolveDesc))
            {
                gDofResolveTexture.Reset();
                if (FAILED(device->CreateTexture2D(&resolveDesc, nullptr, gDofResolveTexture.GetAddressOf())))
                {
                    return false;
                }
                gDofResolveDesc = resolveDesc;
            }
            if (!(reuseCurrentFrame && gDofFocusSourceFrameIndex == gDofFrameIndex))
            {
                context->ResolveSubresource(gDofResolveTexture.Get(), 0, targetTexture.Get(), 0, targetDesc.Format);
            }
            targetTexture = gDofResolveTexture;
            targetResource = gDofResolveTexture;
            targetDesc = resolveDesc;
        }

        const bool useMipPyramid = CanRenderDofMips(device, targetDesc.Format);
        // A sampleable game target lets the pyramid root at half res, skipping the full-res copy.
        ID3D11ShaderResourceView* directSRV = useMipPyramid
            ? EnsureDofDirectSRV(device, targetResource.Get(), targetDesc)
            : nullptr;

        D3D11_TEXTURE2D_DESC sourceDesc = targetDesc;
        if (directSRV)
        {
            sourceDesc.Width = std::max<UINT>(targetDesc.Width / 2, 1);
            sourceDesc.Height = std::max<UINT>(targetDesc.Height / 2, 1);
        }
        sourceDesc.MipLevels = useMipPyramid
            ? GetDofMipCount(sourceDesc.Width, sourceDesc.Height, directSRV ? kFocusPyramidMipCount - 1 : kFocusPyramidMipCount)
            : 1;
        sourceDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (useMipPyramid ? D3D11_BIND_RENDER_TARGET : 0);
        sourceDesc.CPUAccessFlags = 0;
        sourceDesc.Usage = D3D11_USAGE_DEFAULT;
        sourceDesc.MiscFlags = 0;

        if (reuseCurrentFrame &&
            gDofFocusSourceSRV &&
            gDofFocusSourceFrameTarget.Get() == targetResource.Get() &&
            gDofFocusSourceFrameIndex == gDofFrameIndex &&
            SameTextureDesc(gDofFocusSourceDesc, sourceDesc))
        {
            return true;
        }

        if (!gDofFocusSourceTexture || !SameTextureDesc(gDofFocusSourceDesc, sourceDesc))
        {
            gDofFocusSourceTexture.Reset();
            gDofFocusSourceSRV.Reset();
            gDofFocusMipSRVs.clear();
            gDofFocusMipRTVs.clear();
            gDofFocusSourceFrameTarget.Reset();
            gDofFocusSourceFrameIndex = UINT64_MAX;
            gDofFocusSourceMipCount = 1;
            gDofFocusHasPyramid = false;
            if (FAILED(device->CreateTexture2D(&sourceDesc, nullptr, gDofFocusSourceTexture.GetAddressOf())) ||
                FAILED(device->CreateShaderResourceView(gDofFocusSourceTexture.Get(), nullptr, gDofFocusSourceSRV.GetAddressOf())))
            {
                gDofFocusSourceTexture.Reset();
                gDofFocusSourceSRV.Reset();
                gDofFocusSourceFrameTarget.Reset();
                gDofFocusSourceDesc = {};
                gDofFocusSourceFrameIndex = UINT64_MAX;
                gDofFocusSourceMipCount = 1;
                gDofFocusHasPyramid = false;
                return false;
            }
            gDofFocusSourceDesc = sourceDesc;
            gDofFocusSourceMipCount = std::max<UINT>(sourceDesc.MipLevels, 1);
            gDofFocusHasPyramid = useMipPyramid && gDofFocusSourceMipCount > 1;

            if (gDofFocusHasPyramid)
            {
                bool pyramidReady = true;
                for (UINT mip = 0; mip < gDofFocusSourceMipCount; ++mip)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC mipSrvDesc {};
                    mipSrvDesc.Format = sourceDesc.Format;
                    mipSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    mipSrvDesc.Texture2D.MostDetailedMip = mip;
                    mipSrvDesc.Texture2D.MipLevels = 1;
                    ComPtr<ID3D11ShaderResourceView> mipSRV;
                    D3D11_RENDER_TARGET_VIEW_DESC mipRtvDesc {};
                    mipRtvDesc.Format = sourceDesc.Format;
                    mipRtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    mipRtvDesc.Texture2D.MipSlice = mip;
                    ComPtr<ID3D11RenderTargetView> mipRTV;
                    if (FAILED(device->CreateShaderResourceView(gDofFocusSourceTexture.Get(), &mipSrvDesc, mipSRV.GetAddressOf())) ||
                        FAILED(device->CreateRenderTargetView(gDofFocusSourceTexture.Get(), &mipRtvDesc, mipRTV.GetAddressOf())))
                    {
                        pyramidReady = false;
                        break;
                    }
                    gDofFocusMipSRVs.push_back(std::move(mipSRV));
                    gDofFocusMipRTVs.push_back(std::move(mipRTV));
                }

                if (!pyramidReady)
                {
                    gDofFocusMipSRVs.clear();
                    gDofFocusMipRTVs.clear();
                    gDofFocusSourceMipCount = 1;
                    gDofFocusHasPyramid = false;
                    // A half-rooted texture can't fall back to the full-res copy path.
                    if (directSRV)
                    {
                        gDofFocusSourceTexture.Reset();
                        gDofFocusSourceSRV.Reset();
                        gDofFocusSourceDesc = {};
                        return false;
                    }
                }
            }
        }

        gDofFocusLogicalWidth = targetDesc.Width;
        gDofFocusLogicalHeight = targetDesc.Height;
        gDofFocusLodBias = directSRV ? 1.0f : 0.0f;

        if (directSRV)
        {
            if (!BuildDofPyramid(context, directSRV))
            {
                return false;
            }
        }
        else
        {
            context->CopySubresourceRegion(gDofFocusSourceTexture.Get(), 0, 0, 0, 0, targetTexture.Get(), 0, nullptr);
            if (gDofFocusHasPyramid)
            {
                BuildDofPyramid(context, nullptr);
            }
        }
        gDofFocusSourceFrameTarget = targetResource;
        gDofFocusSourceFrameIndex = gDofFrameIndex;
        return true;
    }

    // The scene depth as a shader resource. SceneDepth's copy serves a single-sample depth; a
    // multisampled one is copied whole and read per sample by CocMSPS.
    ID3D11ShaderResourceView* CaptureFocusDepth(ID3D11DepthStencilView* dsv)
    {
        gDofDepthMultisampled = false;
        if (!dsv)
        {
            return SceneDepth::CaptureSceneDepth();
        }

        ComPtr<ID3D11Resource> resource;
        dsv->GetResource(resource.GetAddressOf());
        ComPtr<ID3D11Texture2D> texture;
        if (!resource || FAILED(resource.As(&texture)) || !texture)
        {
            return nullptr;
        }
        D3D11_TEXTURE2D_DESC desc {};
        texture->GetDesc(&desc);
        if (desc.SampleDesc.Count <= 1)
        {
            return SceneDepth::CaptureDepth(dsv);
        }

        DXGI_FORMAT typeless = DXGI_FORMAT_UNKNOWN;
        DXGI_FORMAT view = DXGI_FORMAT_UNKNOWN;
        switch (desc.Format)
        {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
            typeless = DXGI_FORMAT_R24G8_TYPELESS; view = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_TYPELESS:
            typeless = DXGI_FORMAT_R32_TYPELESS; view = DXGI_FORMAT_R32_FLOAT; break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
            typeless = DXGI_FORMAT_R32G8X24_TYPELESS; view = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
        default:
            return nullptr;
        }

        ID3D11Device* device = g_D3D11Hooks.d3dDevice.Get();
        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!device || !context)
        {
            return nullptr;
        }
        if (!gDofDepthMSCopy || !SameTextureDesc(gDofDepthMSDesc, desc))
        {
            gDofDepthMSSRV.Reset();
            gDofDepthMSCopy.Reset();
            D3D11_TEXTURE2D_DESC copyDesc = desc;
            copyDesc.Format = typeless;
            copyDesc.Usage = D3D11_USAGE_DEFAULT;
            copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            copyDesc.CPUAccessFlags = 0;
            copyDesc.MiscFlags = 0;
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
            srvDesc.Format = view;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
            if (FAILED(device->CreateTexture2D(&copyDesc, nullptr, gDofDepthMSCopy.GetAddressOf())) ||
                FAILED(device->CreateShaderResourceView(gDofDepthMSCopy.Get(), &srvDesc, gDofDepthMSSRV.GetAddressOf())))
            {
                gDofDepthMSSRV.Reset();
                gDofDepthMSCopy.Reset();
                return nullptr;
            }
            gDofDepthMSDesc = desc;
        }

        context->CopyResource(gDofDepthMSCopy.Get(), texture.Get());
        gDofDepthMultisampled = true;
        return gDofDepthMSSRV.Get();
    }

    struct DofPassState
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
        // Our passes bind color, depth, CoC, the gather and the multisampled depth at t0-t4.
        ID3D11ShaderResourceView* oldSRV[5] = {};
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

    void ReleaseDofPassState(DofPassState& state)
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

    bool SaveDofPassState(ID3D11DeviceContext* context, DofPassState& state)
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

    void RestoreDofPass(ID3D11DeviceContext* context, DofPassState& state)
    {
        if (!context || !state.saved)
        {
            ReleaseDofPassState(state);
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

        ReleaseDofPassState(state);
    }

    bool BeginDofPass(DofPassState& state, bool requireDepth = true)
    {
        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!context || !EnsureDofRenderer())
        {
            return false;
        }

        if (!SaveDofPassState(context, state))
        {
            return false;
        }

        if (!state.oldRTV[0] ||
            (requireDepth && !state.oldDSV) ||
            state.oldViewportCount == 0 ||
            !CopyDofSourceFromCurrentTarget(state.oldRTV[0], true) ||
            !gDofFocusSourceSRV)
        {
            ReleaseDofPassState(state);
            return false;
        }

        ID3D11RenderTargetView* currentRTV = state.oldRTV[0];
        ID3D11ShaderResourceView* sourceSRVs[2] = { gDofFocusSourceSRV.Get(), nullptr };
        ID3D11SamplerState* sampler = gDofFocusSampler.Get();
        ID3D11Buffer* constantBuffer = gDofFocusConstants.Get();

        context->IASetInputLayout(nullptr);
        context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(gDofFocusVS.Get(), nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &constantBuffer);
        context->GSSetShader(nullptr, nullptr, 0);
        context->PSSetShader(gDofDepthPS.Get(), nullptr, 0);
        context->PSSetConstantBuffers(0, 1, &constantBuffer);
        context->PSSetShaderResources(0, static_cast<UINT>(std::size(sourceSRVs)), sourceSRVs);
        context->PSSetSamplers(0, 1, &sampler);
        context->RSSetState(gDofFocusRasterizerState.Get());
        context->RSSetViewports(1, state.oldViewports);
        context->OMSetRenderTargets(1, &currentRTV, state.oldDSV);
        context->OMSetDepthStencilState(gDofFocusDepthDisabledState.Get(), 0);
        context->OMSetBlendState(gDofFocusBlendState.Get(), nullptr, 0xFFFFFFFF);
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

    bool IsReasonableDofFocusPacket(const DofFocusPacket* packet)
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

    bool ShouldApplyDepthFocus(const DofFocusPacket& packet)
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

        const int resolvedAlpha = FindTrackedMGS3NearFocusWorkAlpha(work, fallbackAlpha);
        const bool stored = StoreMGS3NearFocusPacketFromWork(work, resolvedAlpha);

        // The far composite (which normally carries the near packet) runs earlier in
        // the frame; if it didn't run at all this is a near-only shot. Queue an inert
        // far-focus command so the draw lands in the post-fx slot with the rest of the
        // frame still to come - drawing here directly would blur the 2D layer too.
        if (stored && gMGS3LastFarCompositeFrame != gDofFrameIndex)
        {
            if (gMGS3RbAlloc && gMGS3RbPush)
            {
                if (auto* packet = static_cast<DofFocusPacket*>(gMGS3RbAlloc(sizeof(DofFocusPacket))))
                {
                    packet->alpha = 1;
                    packet->maxPlane = 2;
                    packet->focusNear = 0.5f;
                    packet->focusFar = 0.4f;
                    gMGS3RbPush(kMGS3CmdFarFocus, packet);
                }
            }
            else
            {
                DrawMGS3NearOnlyFocusPass();
            }
        }

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

    void TrackMGS3FocusPacket(const DofFocusPacket* packet, MGS3FocusSourceKind sourceKind = MGS3FocusSourceKind::Unknown)
    {
        if (!Memory::IsReadable(packet, sizeof(DofFocusPacket)) ||
            !IsReasonableDofFocusPacket(packet))
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
        tracked.applyFix = ShouldApplyDepthFocus(tracked.packet);

        if (tracked.sourceKind == MGS3FocusSourceKind::Unknown && previousTracked)
        {
            tracked.sourceKind = previousTracked->sourceKind;
        }
    }

    bool BuildMGS3NearFocusPacketFromWork(uintptr_t work, int alpha, DofFocusPacket& packet)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(work), 0x9C))
        {
            return false;
        }

        packet = DofFocusPacket {
            std::clamp(alpha, 0, 128),
            Memory::ReadField<int>(work, 0x54, 0),
            Memory::ReadField<float>(work, 0x58, 0.0f),
            Memory::ReadField<float>(work, 0x5C, 0.0f),
        };

        return ShouldApplyDepthFocus(packet);
    }

    bool StoreMGS3NearFocusPacketFromWork(uintptr_t work, int alpha)
    {
        DofFocusPacket packet {};
        if (!BuildMGS3NearFocusPacketFromWork(work, alpha, packet))
        {
            return false;
        }

        gMGS3PendingNearFocusPacket.packet = packet;
        gMGS3PendingNearFocusPacket.frameIndex = gDofFrameIndex;
        gMGS3PendingNearFocusPacket.valid = true;
        return true;
    }

    bool ConsumeMGS3PendingNearFocusPacket(DofFocusPacket& packet)
    {
        if (!gMGS3PendingNearFocusPacket.valid ||
            !ShouldApplyDepthFocus(gMGS3PendingNearFocusPacket.packet))
        {
            return false;
        }

        if (gDofFrameIndex < gMGS3PendingNearFocusPacket.frameIndex ||
            gDofFrameIndex - gMGS3PendingNearFocusPacket.frameIndex > kMGS3PendingNearFocusMaxFrameAge)
        {
            gMGS3PendingNearFocusPacket.valid = false;
            return false;
        }

        packet = gMGS3PendingNearFocusPacket.packet;
        gMGS3PendingNearFocusPacket.valid = false;
        return true;
    }

    bool ShouldApplyTrackedMGS3FocusFix(const DofFocusPacket* packet)
    {
        const uintptr_t packetAddress = reinterpret_cast<uintptr_t>(packet);
        if (const TrackedMGS3FocusPacket* tracked = FindTrackedMGS3FocusPacketConst(packetAddress))
        {
            return tracked->applyFix;
        }

        if (!Memory::IsReadable(packet, sizeof(DofFocusPacket)) ||
            !IsReasonableDofFocusPacket(packet))
        {
            return false;
        }

        return ShouldApplyDepthFocus(*packet);
    }

    bool IsReasonableFocusRect(const FocusRect& rect)
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

    struct PreparedFocusDraw
    {
        const DofFocusPacket* packet = nullptr;
        FocusRect fullRect {};
        int focusPixelScale = 0;
    };

    bool PrepareFocusDrawRect(
        const FocusRect& fullRect,
        const DofFocusPacket* packet,
        FocusSide focusSide,
        PreparedFocusDraw& prepared)
    {
        if (!packet || !IsReasonableFocusRect(fullRect))
        {
            return false;
        }

        const int height = fullRect.y2 - fullRect.y1;
        const bool nearSide = focusSide == FocusSide::Near;
        int focusPixelScale = 0;
        if (eGameType & MGS2)
        {
            // Strength 10 is the PS2 blur; the slider scales from there.
            focusPixelScale = std::clamp(static_cast<int>(height * kMGS2FocusSpreadPerHeight * g_DepthOfFieldFixes.fBlurUvMultiplier / 10.0f + 0.5f), 2, 80);
        }
        else
        {
            const float configuredSpread = std::clamp(g_DepthOfFieldFixes.fBlurUvMultiplier / 5.0f, 1.0f, 4.0f);
            // ~0.6% of screen height per configured spread unit.
            const int baseFocusPixelScale = std::clamp(static_cast<int>(height * 0.0029f * configuredSpread + 0.5f), 2, 30);
            const float sideSpreadScale = nearSide ? 1.12f : 1.0f;
            focusPixelScale = std::clamp(static_cast<int>(baseFocusPixelScale * sideSpreadScale * kMGS3FocusSpreadBoost + 0.5f), 2, 80);
        }

        prepared = {};
        prepared.packet = packet;
        prepared.fullRect = fullRect;
        prepared.focusPixelScale = focusPixelScale;
        return true;
    }

    bool PrepareMGS3FocusDraw(
        uintptr_t stackBase,
        const DofFocusPacket* packet,
        FocusSide focusSide,
        PreparedFocusDraw& prepared)
    {
        return PrepareFocusDrawRect(Memory::ReadField<FocusRect>(stackBase, 0x60), packet, focusSide, prepared);
    }

    bool EnsureDofCocTargets(ID3D11Device* device, UINT width, UINT height)
    {
        if (gDofCocTexture && gDofCocWidth == width && gDofCocHeight == height)
        {
            return true;
        }

        gDofCocTexture.Reset();
        gDofCocRTV.Reset();
        gDofCocSRV.Reset();
        gDofCocScratchTexture.Reset();
        gDofCocScratchRTV.Reset();
        gDofCocScratchSRV.Reset();
        gDofCocWidth = 0;
        gDofCocHeight = 0;

        D3D11_TEXTURE2D_DESC desc {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

        if (!device ||
            FAILED(device->CreateTexture2D(&desc, nullptr, gDofCocTexture.GetAddressOf())) ||
            FAILED(device->CreateRenderTargetView(gDofCocTexture.Get(), nullptr, gDofCocRTV.GetAddressOf())) ||
            FAILED(device->CreateShaderResourceView(gDofCocTexture.Get(), nullptr, gDofCocSRV.GetAddressOf())) ||
            FAILED(device->CreateTexture2D(&desc, nullptr, gDofCocScratchTexture.GetAddressOf())) ||
            FAILED(device->CreateRenderTargetView(gDofCocScratchTexture.Get(), nullptr, gDofCocScratchRTV.GetAddressOf())) ||
            FAILED(device->CreateShaderResourceView(gDofCocScratchTexture.Get(), nullptr, gDofCocScratchSRV.GetAddressOf())))
        {
            gDofCocTexture.Reset();
            gDofCocRTV.Reset();
            gDofCocSRV.Reset();
            gDofCocScratchTexture.Reset();
            gDofCocScratchRTV.Reset();
            gDofCocScratchSRV.Reset();
            return false;
        }

        gDofCocWidth = width;
        gDofCocHeight = height;
        return true;
    }

    bool EnsureDofGatherTarget(ID3D11Device* device, UINT width, UINT height)
    {
        if (gDofGatherTexture && gDofGatherWidth == width && gDofGatherHeight == height)
        {
            return true;
        }

        gDofGatherTexture.Reset();
        gDofGatherRTV.Reset();
        gDofGatherSRV.Reset();
        gDofGatherWidth = 0;
        gDofGatherHeight = 0;

        D3D11_TEXTURE2D_DESC desc {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        // FP16 keeps the premultiplied color from banding at low coverage.
        desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

        if (!device ||
            FAILED(device->CreateTexture2D(&desc, nullptr, gDofGatherTexture.GetAddressOf())) ||
            FAILED(device->CreateRenderTargetView(gDofGatherTexture.Get(), nullptr, gDofGatherRTV.GetAddressOf())) ||
            FAILED(device->CreateShaderResourceView(gDofGatherTexture.Get(), nullptr, gDofGatherSRV.GetAddressOf())))
        {
            gDofGatherTexture.Reset();
            gDofGatherRTV.Reset();
            gDofGatherSRV.Reset();
            return false;
        }

        gDofGatherWidth = width;
        gDofGatherHeight = height;
        return true;
    }

    bool DrawDepthWeightedFocus(
        const DofPassState& passState,
        const PreparedFocusDraw& farDraw,
        const PreparedFocusDraw* nearDraw,
        ID3D11ShaderResourceView* depthSRV)
    {
        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!context ||
            !gDofDepthPS ||
            !gDofCocPS ||
            !gDofFocusConstants ||
            !gDofFocusDepthDisabledState ||
            !gDofFocusSourceSRV ||
            !depthSRV ||
            !farDraw.packet ||
            !IsReasonableFocusRect(farDraw.fullRect))
        {
            return false;
        }

        const PreparedFocusDraw* activeNear = (nearDraw && nearDraw->packet) ? nearDraw : nullptr;
        const int width = farDraw.fullRect.x2 - farDraw.fullRect.x1;
        const int height = farDraw.fullRect.y2 - farDraw.fullRect.y1;

        DofFocusConstants constants {};
        constants.sourceRect[0] = static_cast<float>(farDraw.fullRect.x1);
        constants.sourceRect[1] = static_cast<float>(farDraw.fullRect.y1);
        constants.sourceRect[2] = static_cast<float>(width);
        constants.sourceRect[3] = static_cast<float>(height);
        // UVs and spread are in game-target pixels, independent of the pyramid root.
        constants.sourceSizeAndSpread[0] = static_cast<float>(gDofFocusLogicalWidth);
        constants.sourceSizeAndSpread[1] = static_cast<float>(gDofFocusLogicalHeight);
        constants.sourceSizeAndSpread[2] = static_cast<float>(farDraw.focusPixelScale);
        constants.sourceSizeAndSpread[3] = static_cast<float>(std::max<UINT>(gDofFocusSourceMipCount, 1) - 1);
        constants.color[0] = 1.0f;
        constants.color[1] = 1.0f;
        constants.color[2] = 1.0f;
        constants.color[3] = static_cast<float>(activeNear ? activeNear->focusPixelScale : 0);

        const auto writePacket = [&](int index, const PreparedFocusDraw* draw) {
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
            constants.planeData[index][3] = static_cast<float>(std::clamp(draw->packet->maxPlane, 1, kFocusMaxPlaneCount));
        };

        writePacket(0, &farDraw);
        writePacket(1, activeNear);

        constants.depthSize[0] = constants.sourceSizeAndSpread[0];
        constants.depthSize[1] = constants.sourceSizeAndSpread[1];
        {
            ComPtr<ID3D11Resource> depthResource;
            depthSRV->GetResource(depthResource.GetAddressOf());
            ComPtr<ID3D11Texture2D> depthTexture;
            if (depthResource && SUCCEEDED(depthResource.As(&depthTexture)) && depthTexture)
            {
                D3D11_TEXTURE2D_DESC depthDesc {};
                depthTexture->GetDesc(&depthDesc);
                constants.depthSize[0] = static_cast<float>(depthDesc.Width);
                constants.depthSize[1] = static_cast<float>(depthDesc.Height);
            }
        }
        constants.depthSize[2] = gDofFocusLodBias;

        context->UpdateSubresource(gDofFocusConstants.Get(), 0, nullptr, &constants, 0, 0);

        const UINT cocWidth = std::max<UINT>(static_cast<UINT>(constants.depthSize[0]) / 2, 1);
        const UINT cocHeight = std::max<UINT>(static_cast<UINT>(constants.depthSize[1]) / 2, 1);
        if (!EnsureDofCocTargets(g_D3D11Hooks.d3dDevice.Get(), cocWidth, cocHeight))
        {
            return false;
        }

        ID3D11Buffer* constantBuffer = gDofFocusConstants.Get();
        context->PSSetConstantBuffers(0, 1, &constantBuffer);
        context->OMSetDepthStencilState(gDofFocusDepthDisabledState.Get(), 0);
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

        // CoC at half depth res, then a separable dilate of the near channel; the
        // composite reads one filtered CoC sample instead of nine depth loads per pixel.
        const D3D11_VIEWPORT cocViewport { 0.0f, 0.0f, static_cast<float>(cocWidth), static_cast<float>(cocHeight), 0.0f, 1.0f };
        ID3D11ShaderResourceView* nullSRVs[5] = {};
        ID3D11ShaderResourceView* depthSingle = gDofDepthMultisampled ? nullptr : depthSRV;
        ID3D11ShaderResourceView* depthMulti = gDofDepthMultisampled ? depthSRV : nullptr;
        const auto runCocPass = [&](ID3D11PixelShader* shader, ID3D11RenderTargetView* target, ID3D11ShaderResourceView* cocInput) {
            context->PSSetShaderResources(0, 5, nullSRVs);
            context->OMSetRenderTargets(1, &target, nullptr);
            context->RSSetViewports(1, &cocViewport);
            ID3D11ShaderResourceView* srvs[5] = { nullptr, depthSingle, cocInput, nullptr, depthMulti };
            context->PSSetShader(shader, nullptr, 0);
            context->PSSetShaderResources(0, 5, srvs);
            context->DrawInstanced(3, 1, 0, 0);
        };

        runCocPass(gDofDepthMultisampled ? gDofCocMSPS.Get() : gDofCocPS.Get(), gDofCocRTV.Get(), nullptr);
        if (activeNear)
        {
            runCocPass(gDofDilateHPS.Get(), gDofCocScratchRTV.Get(), gDofCocSRV.Get());
            runCocPass(gDofDilateVPS.Get(), gDofCocRTV.Get(), gDofCocScratchSRV.Get());
        }

        // Blur taps at half res; the full-res composite then only upsamples and blends.
        const UINT gatherWidth = std::max<UINT>(gDofFocusLogicalWidth / 2, 1);
        const UINT gatherHeight = std::max<UINT>(gDofFocusLogicalHeight / 2, 1);
        const bool halfResGather = gDofGatherPS &&
            gDofUpsamplePS &&
            gDofFocusPremultBlendState &&
            EnsureDofGatherTarget(g_D3D11Hooks.d3dDevice.Get(), gatherWidth, gatherHeight);

        if (halfResGather)
        {
            const D3D11_VIEWPORT gatherViewport { 0.0f, 0.0f, static_cast<float>(gatherWidth), static_cast<float>(gatherHeight), 0.0f, 1.0f };
            ID3D11RenderTargetView* gatherRTV = gDofGatherRTV.Get();
            context->PSSetShaderResources(0, 4, nullSRVs);
            context->OMSetRenderTargets(1, &gatherRTV, nullptr);
            context->RSSetViewports(1, &gatherViewport);
            context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            ID3D11ShaderResourceView* gatherSrvs[4] = { gDofFocusSourceSRV.Get(), nullptr, gDofCocSRV.Get(), nullptr };
            context->PSSetShader(gDofGatherPS.Get(), nullptr, 0);
            context->PSSetShaderResources(0, 4, gatherSrvs);
            context->DrawInstanced(3, 1, 0, 0);
        }

        // Back to the caller's target for the composite.
        ID3D11RenderTargetView* targetRTV = passState.oldRTV[0];
        context->PSSetShaderResources(0, 4, nullSRVs);
        context->OMSetRenderTargets(1, &targetRTV, passState.oldDSV);
        context->RSSetViewports(1, passState.oldViewports);
        context->OMSetBlendState(halfResGather ? gDofFocusPremultBlendState.Get() : gDofFocusBlendState.Get(), nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(gDofFocusDepthDisabledState.Get(), 0);

        ID3D11ShaderResourceView* srvs[4] = { gDofFocusSourceSRV.Get(), depthSingle, gDofCocSRV.Get(), halfResGather ? gDofGatherSRV.Get() : nullptr };
        context->PSSetShader(halfResGather ? gDofUpsamplePS.Get() : gDofDepthPS.Get(), nullptr, 0);
        context->PSSetShaderResources(0, 4, srvs);

        context->DrawInstanced(3, 1, 0, 0);
        return true;
    }

    // Some shots run near focus with no far-focus composite to piggyback on; draw the
    // near side standalone at the game's own near-focus render position.
    void DrawMGS3NearOnlyFocusPass()
    {
        if (!gMGS3PendingNearFocusPacket.valid)
        {
            return;
        }

        DofPassState passState {};
        if (!BeginDofPass(passState, false))
        {
            return;
        }

        // The near callback runs after the game's 3D pass; the depth buffer may already
        // be unbound, so take a fresh copy of the last scene depth the game rendered.
        ID3D11ShaderResourceView* depthSRV = CaptureFocusDepth(passState.oldDSV);
        if (!depthSRV)
        {
            RestoreDofPass(g_D3D11Hooks.d3dDeviceContext.Get(), passState);
            return;
        }

        DofFocusPacket nearPacket {};
        if (!ConsumeMGS3PendingNearFocusPacket(nearPacket))
        {
            RestoreDofPass(g_D3D11Hooks.d3dDeviceContext.Get(), passState);
            return;
        }

        const FocusRect fullRect { 0, 0,
            static_cast<int>(gDofFocusLogicalWidth),
            static_cast<int>(gDofFocusLogicalHeight) };
        const int height = fullRect.y2;
        const float configuredSpread = std::clamp(g_DepthOfFieldFixes.fBlurUvMultiplier / 5.0f, 1.0f, 4.0f);
        const int baseFocusPixelScale = std::clamp(static_cast<int>(height * 0.0029f * configuredSpread + 0.5f), 2, 30);
        const int focusPixelScale = std::clamp(static_cast<int>(baseFocusPixelScale * 1.12f * kMGS3FocusSpreadBoost + 0.5f), 2, 80);

        static const DofFocusPacket inertFar { 0, 1, 0.5f, 0.4f };
        PreparedFocusDraw farDraw {};
        farDraw.packet = &inertFar;
        farDraw.fullRect = fullRect;
        farDraw.focusPixelScale = 2;

        PreparedFocusDraw nearDraw {};
        nearDraw.packet = &nearPacket;
        nearDraw.fullRect = fullRect;
        nearDraw.focusPixelScale = focusPixelScale;

        DrawDepthWeightedFocus(passState, farDraw, &nearDraw, depthSRV);

        RestoreDofPass(g_D3D11Hooks.d3dDeviceContext.Get(), passState);
    }

    void DrawMGS3CombinedFocusSamples(
        uintptr_t stackBase,
        const DofFocusPacket* farPacket,
        const DofFocusPacket* nearPacket)
    {
        PreparedFocusDraw farDraw {};
        if (!PrepareMGS3FocusDraw(stackBase, farPacket, FocusSide::Far, farDraw))
        {
            return;
        }

        PreparedFocusDraw nearDraw {};
        const bool hasNear = nearPacket && PrepareMGS3FocusDraw(stackBase, nearPacket, FocusSide::Near, nearDraw);

        DofPassState passState {};
        if (!BeginDofPass(passState))
        {
            return;
        }

        ID3D11ShaderResourceView* depthSRV = CaptureFocusDepth(passState.oldDSV);
        DrawDepthWeightedFocus(passState, farDraw, hasNear ? &nearDraw : nullptr, depthSRV);

        RestoreDofPass(g_D3D11Hooks.d3dDeviceContext.Get(), passState);
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

    // The game asks for the far blur and the near blur separately. Each only touches its own depth
    // range, so whichever comes first is simply drawn on its own.
    bool DrawMGS2DepthFocus(const FocusSourcePacket* packet, bool nearSide)
    {
        if (!Memory::IsReadable(packet, sizeof(FocusSourcePacket)))
        {
            return false;
        }
        FocusSourcePacket source = *packet;
        if (!PrepareFocusSource(source))
        {
            return false;
        }

        const float boost = std::clamp((g_DepthOfFieldFixes.fBlurUvMultiplier - 20.0f) / 10.0f, 0.0f, 1.0f);
        const int alpha = kMGS2FocusAlpha + static_cast<int>(boost * kMGS2FocusAlphaBoost + 0.5f);
        // The scene's own plane count sets the blur width; PrepareFocusSource pads it.
        const DofFocusPacket focus { alpha, std::clamp(packet->maxPlane, 2, kFocusMaxPlaneCount), source.focusNear, source.focusFar };
        if (!ShouldApplyDepthFocus(focus))
        {
            return false;
        }

        DofPassState passState {};
        if (!BeginDofPass(passState))
        {
            return false;
        }

        ID3D11DeviceContext* context = g_D3D11Hooks.d3dDeviceContext.Get();
        ID3D11ShaderResourceView* depthSRV = CaptureFocusDepth(passState.oldDSV);
        const FocusRect fullRect { 0, 0,
            static_cast<int>(gDofFocusLogicalWidth),
            static_cast<int>(gDofFocusLogicalHeight) };

        static const DofFocusPacket inertFar { 0, 1, 0.5f, 0.4f };
        PreparedFocusDraw farDraw {};
        PreparedFocusDraw nearDraw {};
        bool prepared = false;
        if (nearSide)
        {
            prepared = PrepareFocusDrawRect(fullRect, &inertFar, FocusSide::Far, farDraw) &&
                PrepareFocusDrawRect(fullRect, &focus, FocusSide::Near, nearDraw);
        }
        else
        {
            prepared = PrepareFocusDrawRect(fullRect, &focus, FocusSide::Far, farDraw);
        }

        const bool drawn = prepared && depthSRV &&
            DrawDepthWeightedFocus(passState, farDraw, nearSide ? &nearDraw : nullptr, depthSRV);
        RestoreDofPass(context, passState);

        static bool logged = false;
        if (drawn && !logged)
        {
            logged = true;
            spdlog::info("MGS 2: Depth of Field: depth renderer active ({}).", gDofDepthMultisampled ? "multisampled" : "single sample");
        }
        return drawn;
    }

    void __fastcall FarFocusCommand_Hook(void* packet)
    {
        auto* focusPacket = static_cast<FocusSourcePacket*>(packet);
        const bool isNearFocusPacket = IsTrackedNearFocusPacket(focusPacket);
        const bool drawn = DrawMGS2DepthFocus(focusPacket, isNearFocusPacket);
        if (isNearFocusPacket)
        {
            // The game's planes only know the far side; a near packet it can't draw is dropped.
            ConsumeNearFocusPacket(focusPacket);
            return;
        }
        if (!drawn)
        {
            FarFocusCommandHook.fastcall<void>(packet);
        }
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

    void ResolveMGS3RenderBufferEmitters()
    {
        uint8_t* alloc = Memory::PatternScan(
            baseModule,
            "48 63 05 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 48 63 C9 48 8D 14 80 49 8B 04",
            "MGS 3: Depth of Field: render command alloc");
        uint8_t* push = Memory::PatternScan(
            baseModule,
            "48 63 05 ?? ?? ?? ?? 4C 8D 15 ?? ?? ?? ?? 4C 8D 04 80 4E 8D 0C C5",
            "MGS 3: Depth of Field: render command push");

        if (alloc && push)
        {
            gMGS3RbAlloc = reinterpret_cast<MGS3RbAllocFn>(alloc);
            gMGS3RbPush = reinterpret_cast<MGS3RbPushFn>(push);
        }
        else
        {
            spdlog::warn("MGS 3: Depth of Field: render command emitters not found; near-only focus draws late.");
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
            auto* packet = Memory::ReadField<DofFocusPacket*>(commandNode, 0x08, nullptr);
            if (!Memory::IsReadable(packet, sizeof(DofFocusPacket)) ||
                !IsReasonableDofFocusPacket(packet))
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
            "48 8B 44 35",
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
            auto* packet = reinterpret_cast<DofFocusPacket*>(ctx.r15);
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
                // Our queued near-only command carries this fingerprint; it must not
                // count as a real far composite (the near callback would then skip
                // queueing every other frame), and it's redundant if a real one
                // already consumed the near packet this frame.
                const bool synthetic = packet->alpha == 1 && packet->maxPlane == 2 &&
                    packet->focusNear == 0.5f && packet->focusFar == 0.4f;
                const bool redundant = synthetic && gMGS3LastFarCompositeFrame == gDofFrameIndex;
                if (!synthetic)
                {
                    gMGS3LastFarCompositeFrame = gDofFrameIndex;
                }

                if (!redundant)
                {
                    DofFocusPacket nearPacket {};
                    const bool hasNearPacket = ConsumeMGS3PendingNearFocusPacket(nearPacket);
                    if (!synthetic || hasNearPacket)
                    {
                        DrawMGS3CombinedFocusSamples(ctx.rsp, packet, hasNearPacket ? &nearPacket : nullptr);
                    }
                }
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
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }

    ++gDofFrameIndex;
    gDofFocusSourceFrameTarget.Reset();
    gDofFocusSourceFrameIndex = UINT64_MAX;

    // Drop the game-target SRV once DOF goes idle.
    if (gDofFocusDirectFrameIndex + 1 < gDofFrameIndex)
    {
        gDofFocusDirectSource.Reset();
        gDofFocusDirectSRV.Reset();
    }
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

    fBlurUvMultiplier = std::clamp(fBlurUvMultiplier, 0.0f, 30.0f);
    spdlog::info("{}: Depth of Field: blur strength set to {}.", gameLabel, fBlurUvMultiplier);



    if (eGameType & MGS2)
    {
        if (InstallFarFocusCommandHook() && ResolveRenderBufferHelpers())
        {
            InstallNearFocusCreationHooks();
        }
        else
        {
            spdlog::warn("MGS 2: Depth of Field: failed to resolve necessary functions for near focus fixes; near focus adjustments disabled.");
        }

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
    ResolveMGS3RenderBufferEmitters();
}
