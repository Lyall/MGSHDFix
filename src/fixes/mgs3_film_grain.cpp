#include "stdafx.h"
#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"
#include "mgs3_film_grain.hpp"

#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace
{
    struct FilmGrainConstants
    {
        float strength;
        float frameIndex;
        float width;
        float height;
    };

    constexpr UINT kBlueNoiseSize = 256;
    constexpr UINT kBlueNoiseFrames = 32;
    constexpr int kAllScenesFallbackAlpha = 8;
    constexpr uintptr_t kNoiseAlphaOffset = 0x80;
    constexpr uintptr_t kNoiseActiveOffset = 0x98;

    const char* kFilmGrainShader = R"(
    cbuffer FilmGrainCB : register(b0)
    {
        float strength;
        float frameIndex;
        float width;
        float height;
    };

    Texture2D sceneColor : register(t0);
    Texture2DArray grainTexture : register(t1);
    SamplerState sceneSampler : register(s0);
    SamplerState grainSampler : register(s1);

    void VS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD0)
    {
        uv = float2((id << 1) & 2, id & 2);
        pos = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    }

    float4 PS(float4 pos : SV_Position, float2 uv : TEXCOORD0) : SV_Target
    {
        float3 color = sceneColor.Sample(sceneSampler, uv).rgb;
        float grainSize = max(1.0, height / 640.0);
        float2 grainUv = (pos.xy + 0.5) / (grainSize * 256.0);
        float grain = grainTexture.SampleLevel(grainSampler, float3(grainUv, fmod(frameIndex, 32.0)), 0).r;
        color = saturate(color + (grain - 0.5) * strength);
        return float4(color, 1.0);
    }
    )";

    std::atomic<int> gPendingAlpha = 0;
    std::atomic<int> gLatchedAlpha = 0;
    std::atomic<bool> gNativeSignalActive = false;
    std::atomic<uintptr_t> gNativeWork = 0;

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11SamplerState> sceneSampler;
    ComPtr<ID3D11SamplerState> grainSampler;
    ComPtr<ID3D11RasterizerState> rasterizer;
    ComPtr<ID3D11DepthStencilState> depthStencil;
    ComPtr<ID3D11Buffer> constantBuffer;
    ComPtr<ID3D11Texture2D> grainTexture;
    ComPtr<ID3D11ShaderResourceView> grainSRV;
    ComPtr<ID3D11Texture2D> sceneCopy;
    ComPtr<ID3D11ShaderResourceView> sceneCopySRV;
    D3D11_TEXTURE2D_DESC sceneCopyDesc = {};
    bool initialized = false;
    bool initFailed = false;
    uint32_t frameIndex = 0;
    bool drawingGrain = false;
    std::atomic<bool> drewBackbufferThisFrame = false;
    std::atomic<bool> insidePresent = false;

    uint32_t Hash32(uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7feb352dU;
        value ^= value >> 15;
        value *= 0x846ca68bU;
        value ^= value >> 16;
        return value;
    }

    float Random01(uint32_t seed)
    {
        return static_cast<float>(Hash32(seed) & 0x00ffffffU) / 16777216.0f;
    }

    bool NativeAllowsFilmGrain()
    {
        if (MGS3FilmGrain::mode == MGS3FilmGrain::Mode::AllScenes)
        {
            return true;
        }

        return gNativeSignalActive.load(std::memory_order_relaxed);
    }

    void ClearFilmGrainAlpha()
    {
        gPendingAlpha.store(0, std::memory_order_relaxed);
        gLatchedAlpha.store(0, std::memory_order_relaxed);
    }

    void ResetFilmGrainState()
    {
        ClearFilmGrainAlpha();
        gNativeSignalActive.store(false, std::memory_order_relaxed);
        gNativeWork.store(0, std::memory_order_relaxed);
    }

    bool TryReadNativeInt(uintptr_t work, uintptr_t offset, int& value)
    {
        if (!work)
        {
            return false;
        }

        const auto* address = reinterpret_cast<const int*>(work + offset);
        if (!Memory::IsReadable(address, sizeof(*address)))
        {
            return false;
        }

        value = *address;
        return true;
    }

    bool NativeWorkIsActive(uintptr_t work)
    {
        int active = 0;
        return TryReadNativeInt(work, kNoiseActiveOffset, active) && active != 0;
    }

    void QueueFilmGrainAlpha(int alpha)
    {
        if (alpha <= 0)
        {
            ClearFilmGrainAlpha();
            return;
        }

        gLatchedAlpha.store(alpha, std::memory_order_relaxed);

        int current = gPendingAlpha.load(std::memory_order_relaxed);
        while (alpha > current &&
            !gPendingAlpha.compare_exchange_weak(current, alpha, std::memory_order_relaxed))
        {
        }
    }

    void QueueFilmGrainAlphaFromNative(uintptr_t work, int alpha)
    {
        if (work)
        {
            gNativeWork.store(work, std::memory_order_relaxed);
        }

        if (MGS3FilmGrain::mode != MGS3FilmGrain::Mode::AllScenes && !NativeWorkIsActive(work))
        {
            ClearFilmGrainAlpha();
            gNativeSignalActive.store(false, std::memory_order_relaxed);
            return;
        }

        gNativeSignalActive.store(true, std::memory_order_relaxed);
        QueueFilmGrainAlpha(alpha);
    }

    void TrackFilmGrainAlpha(SafetyHookContext& ctx)
    {
        QueueFilmGrainAlphaFromNative(static_cast<uintptr_t>(ctx.rcx), static_cast<int>(ctx.rdx & 0xffffffff));
    }

    void TrackFilmGrainSignal(SafetyHookContext& ctx)
    {
        const uintptr_t work = static_cast<uintptr_t>(ctx.rcx);
        const int signal = static_cast<int>(ctx.rdx);

        if (work)
        {
            gNativeWork.store(work, std::memory_order_relaxed);
        }

        if (signal == 1)
        {
            ResetFilmGrainState();
            return;
        }

        if (signal == 2)
        {
            gNativeSignalActive.store(true, std::memory_order_relaxed);
            int alpha = 0;
            if (TryReadNativeInt(work, kNoiseAlphaOffset, alpha))
            {
                QueueFilmGrainAlpha(alpha);
            }
            return;
        }

        if (signal == 3)
        {
            QueueFilmGrainAlphaFromNative(work, static_cast<int>(ctx.r8 & 0xffffffff));
            return;
        }

        if (signal == 5 && Memory::IsReadable(reinterpret_cast<const void*>(ctx.r8), sizeof(int) * 2))
        {
            const int targetAlpha = *reinterpret_cast<const int*>(ctx.r8);
            QueueFilmGrainAlphaFromNative(work, targetAlpha);
        }
    }

    void TrackFilmGrainCleanup(SafetyHookContext& ctx)
    {
        const uintptr_t work = static_cast<uintptr_t>(ctx.rcx);
        const uintptr_t trackedWork = gNativeWork.load(std::memory_order_relaxed);
        if (!trackedWork || trackedWork == work)
        {
            ResetFilmGrainState();
        }
    }

    float StrengthFromAlpha(int alpha)
    {
        if (alpha <= 0)
        {
            return 0.0f;
        }

        const float normalized = static_cast<float>(std::clamp(alpha, 0, 128)) / 128.0f;
        return std::clamp(std::max(normalized * 1.7f, 0.1f), 0.0f, 0.25f);
    }

    bool EnsureResources(ID3D11Device* device)
    {
        if (initialized)
        {
            return true;
        }

        if (initFailed || !device || !g_D3D11Hooks.D3DCompileFunc)
        {
            return false;
        }

        ComPtr<ID3DBlob> err;
        HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kFilmGrainShader, strlen(kFilmGrainShader),
            nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0,
            vsBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            spdlog::warn("MGS 3: Film Grain: VS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
            initFailed = true;
            return false;
        }

        err.Reset();
        hr = g_D3D11Hooks.D3DCompileFunc(kFilmGrainShader, strlen(kFilmGrainShader),
            nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0,
            psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            spdlog::warn("MGS 3: Film Grain: PS compile failed: {}", err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
            initFailed = true;
            return false;
        }

        if (FAILED(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf())) ||
            FAILED(device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf())))
        {
            spdlog::warn("MGS 3: Film Grain: failed to create shaders.");
            initFailed = true;
            return false;
        }

        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        device->CreateSamplerState(&samplerDesc, sceneSampler.GetAddressOf());

        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        device->CreateSamplerState(&samplerDesc, grainSampler.GetAddressOf());

        D3D11_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D11_FILL_SOLID;
        rasterizerDesc.CullMode = D3D11_CULL_NONE;
        device->CreateRasterizerState(&rasterizerDesc, rasterizer.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC depthDesc = {};
        depthDesc.DepthEnable = FALSE;
        depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        device->CreateDepthStencilState(&depthDesc, depthStencil.GetAddressOf());

        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = sizeof(FilmGrainConstants);
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&bufferDesc, nullptr, constantBuffer.GetAddressOf());

        std::vector<uint8_t> noiseData(kBlueNoiseSize * kBlueNoiseSize * kBlueNoiseFrames);
        std::vector<float> white(kBlueNoiseSize * kBlueNoiseSize);
        std::vector<float> highpass(kBlueNoiseSize * kBlueNoiseSize);
        std::array<D3D11_SUBRESOURCE_DATA, kBlueNoiseFrames> subresources = {};

        for (UINT frame = 0; frame < kBlueNoiseFrames; ++frame)
        {
            const uint32_t frameSeed = 0x9e3779b9U * (frame + 1);
            for (UINT y = 0; y < kBlueNoiseSize; ++y)
            {
                for (UINT x = 0; x < kBlueNoiseSize; ++x)
                {
                    white[y * kBlueNoiseSize + x] = Random01(frameSeed ^ (x * 0x85ebca6bU) ^ (y * 0xc2b2ae35U));
                }
            }

            for (UINT y = 0; y < kBlueNoiseSize; ++y)
            {
                for (UINT x = 0; x < kBlueNoiseSize; ++x)
                {
                    const auto at = [&](int ox, int oy)
                    {
                        const UINT sx = static_cast<UINT>((static_cast<int>(x) + ox + kBlueNoiseSize) % kBlueNoiseSize);
                        const UINT sy = static_cast<UINT>((static_cast<int>(y) + oy + kBlueNoiseSize) % kBlueNoiseSize);
                        return white[sy * kBlueNoiseSize + sx];
                    };

                    const float blur =
                        at(-1, -1) * 0.0625f + at(0, -1) * 0.125f + at(1, -1) * 0.0625f +
                        at(-1, 0) * 0.125f + at(0, 0) * 0.25f + at(1, 0) * 0.125f +
                        at(-1, 1) * 0.0625f + at(0, 1) * 0.125f + at(1, 1) * 0.0625f;

                    highpass[y * kBlueNoiseSize + x] = at(0, 0) - blur;
                }
            }

            float mean = 0.0f;
            for (float value : highpass)
            {
                mean += value;
            }
            mean /= static_cast<float>(highpass.size());

            float variance = 0.0f;
            for (float value : highpass)
            {
                const float centered = value - mean;
                variance += centered * centered;
            }
            variance = std::max(variance / static_cast<float>(highpass.size()), 0.000001f);
            const float invStdDev = 1.0f / std::sqrt(variance);

            uint8_t* frameData = noiseData.data() + frame * kBlueNoiseSize * kBlueNoiseSize;
            for (size_t i = 0; i < highpass.size(); ++i)
            {
                const float normalized = std::clamp(0.5f + (highpass[i] - mean) * invStdDev * 0.18f, 0.0f, 1.0f);
                frameData[i] = static_cast<uint8_t>(std::lround(normalized * 255.0f));
            }

            subresources[frame].pSysMem = frameData;
            subresources[frame].SysMemPitch = kBlueNoiseSize;
        }

        D3D11_TEXTURE2D_DESC grainDesc = {};
        grainDesc.Width = kBlueNoiseSize;
        grainDesc.Height = kBlueNoiseSize;
        grainDesc.MipLevels = 1;
        grainDesc.ArraySize = kBlueNoiseFrames;
        grainDesc.Format = DXGI_FORMAT_R8_UNORM;
        grainDesc.SampleDesc.Count = 1;
        grainDesc.Usage = D3D11_USAGE_IMMUTABLE;
        grainDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(device->CreateTexture2D(&grainDesc, subresources.data(), grainTexture.GetAddressOf())))
        {
            spdlog::warn("MGS 3: Film Grain: failed to create blue-noise texture.");
            initFailed = true;
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC grainSrvDesc = {};
        grainSrvDesc.Format = grainDesc.Format;
        grainSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        grainSrvDesc.Texture2DArray.MipLevels = 1;
        grainSrvDesc.Texture2DArray.ArraySize = kBlueNoiseFrames;

        if (FAILED(device->CreateShaderResourceView(grainTexture.Get(), &grainSrvDesc, grainSRV.GetAddressOf())))
        {
            spdlog::warn("MGS 3: Film Grain: failed to create blue-noise view.");
            initFailed = true;
            return false;
        }

        initialized = static_cast<bool>(vs && ps && sceneSampler && grainSampler && rasterizer && depthStencil && constantBuffer && grainSRV);
        if (!initialized)
        {
            spdlog::warn("MGS 3: Film Grain: failed to initialize D3D resources.");
            initFailed = true;
            return false;
        }

        vsBlob.Reset();
        psBlob.Reset();
        spdlog::info("MGS 3: Film Grain: D3D grain pass initialized.");
        return true;
    }

    bool EnsureSceneCopy(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& backbufferDesc)
    {
        if (sceneCopy && sceneCopyDesc.Width == backbufferDesc.Width &&
            sceneCopyDesc.Height == backbufferDesc.Height &&
            sceneCopyDesc.Format == backbufferDesc.Format)
        {
            return true;
        }

        sceneCopy.Reset();
        sceneCopySRV.Reset();

        D3D11_TEXTURE2D_DESC desc = backbufferDesc;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        if (FAILED(device->CreateTexture2D(&desc, nullptr, sceneCopy.GetAddressOf())) ||
            FAILED(device->CreateShaderResourceView(sceneCopy.Get(), nullptr, sceneCopySRV.GetAddressOf())))
        {
            sceneCopy.Reset();
            sceneCopySRV.Reset();
            return false;
        }

        sceneCopyDesc = backbufferDesc;
        return true;
    }

    bool IsSwapchainBackbuffer(ID3D11RenderTargetView* rtv)
    {
        auto* swap = g_D3D11Hooks.swapChain.Get();
        if (!swap || !rtv)
        {
            return false;
        }

        ComPtr<ID3D11Resource> targetResource;
        rtv->GetResource(targetResource.GetAddressOf());

        ComPtr<ID3D11Texture2D> backbuffer;
        if (!targetResource ||
            FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()))) ||
            !backbuffer)
        {
            return false;
        }

        ComPtr<IUnknown> targetIdentity;
        ComPtr<IUnknown> backbufferIdentity;
        if (FAILED(targetResource.As(&targetIdentity)) || FAILED(backbuffer.As(&backbufferIdentity)))
        {
            return false;
        }

        return targetIdentity.Get() == backbufferIdentity.Get();
    }

    bool IsFullScreenBackbufferDraw(ID3D11DeviceContext* context, UINT vertexCount, ID3D11RenderTargetView** currentRTV)
    {
        if (!context || (vertexCount != 3 && vertexCount != 4))
        {
            return false;
        }

        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        context->IAGetPrimitiveTopology(&topology);
        if (topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST &&
            topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)
        {
            return false;
        }

        ID3D11DepthStencilView* dsv = nullptr;
        context->OMGetRenderTargets(1, currentRTV, &dsv);
        if (dsv)
        {
            dsv->Release();
            if (*currentRTV)
            {
                (*currentRTV)->Release();
                *currentRTV = nullptr;
            }
            return false;
        }

        if (!*currentRTV || !IsSwapchainBackbuffer(*currentRTV))
        {
            if (*currentRTV)
            {
                (*currentRTV)->Release();
                *currentRTV = nullptr;
            }
            return false;
        }

        return true;
    }
}

void MGS3FilmGrain::Initialize()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    if (mode == Mode::Off)
    {
        spdlog::info("MGS 3: Film Grain: disabled by config.");
        return;
    }

    MAKE_HOOK_MID(baseModule,
        "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 50 48 8B 41 58",
        "MGS 3: Film Grain: noise1 alpha",
        {
            TrackFilmGrainAlpha(ctx);
        });

    MAKE_HOOK_MID(baseModule,
        "48 83 EA 01 0F 84 ?? ?? ?? ?? 48 83 EA 01 0F 84 ?? ?? ?? ?? 48 83 EA 01 0F 84 ?? ?? ?? ?? 48 83 EA 01 0F 84 ?? ?? ?? ?? 48 83 FA 01 74 03 33 C0 C3",
        "MGS 3: Film Grain: noise1 signal",
        {
            TrackFilmGrainSignal(ctx);
        });

    MAKE_HOOK_MID(baseModule,
        "40 53 48 83 EC 20 48 8B D9 48 8B 49 68 48 85 C9 74 15 48 8B 83 A8 00 00 00",
        "MGS 3: Film Grain: noise1 cleanup",
        {
            TrackFilmGrainCleanup(ctx);
        });

}

void MGS3FilmGrain::OnPreMenuRender(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView*)
{
    if (!(eGameType & MGS3) || mode == Mode::Off || !sceneColor || !NativeAllowsFilmGrain())
    {
        return;
    }

    int alpha = gPendingAlpha.exchange(0, std::memory_order_relaxed);
    if (alpha <= 0)
    {
        alpha = gLatchedAlpha.load(std::memory_order_relaxed);
    }
    if (alpha <= 0 && mode == Mode::AllScenes)
    {
        alpha = kAllScenesFallbackAlpha;
    }

    const float strength = StrengthFromAlpha(alpha);
    if (strength <= 0.0f)
    {
        return;
    }

    auto* device = g_D3D11Hooks.d3dDevice.Get();
    auto* context = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!EnsureResources(device) || !context)
    {
        return;
    }

    ComPtr<ID3D11Resource> sceneResource;
    sceneColor->GetResource(sceneResource.GetAddressOf());

    ComPtr<ID3D11Texture2D> sceneTexture;
    if (!sceneResource || FAILED(sceneResource.As(&sceneTexture)) || !sceneTexture)
    {
        return;
    }

    D3D11_TEXTURE2D_DESC sceneDesc = {};
    sceneTexture->GetDesc(&sceneDesc);

    if (!EnsureSceneCopy(device, sceneDesc))
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        return;
    }

    auto* constants = static_cast<FilmGrainConstants*>(mapped.pData);
    constants->strength = strength;
    constants->frameIndex = static_cast<float>(frameIndex++);
    constants->width = static_cast<float>(sceneDesc.Width);
    constants->height = static_cast<float>(sceneDesc.Height);
    context->Unmap(constantBuffer.Get(), 0);

    context->CopyResource(sceneCopy.Get(), sceneTexture.Get());

    ID3D11RenderTargetView* oldRTV[8] = {};
    ID3D11DepthStencilView* oldDSV = nullptr;
    ID3D11BlendState* oldBlend = nullptr;
    ID3D11DepthStencilState* oldDepthStencil = nullptr;
    ID3D11RasterizerState* oldRasterizer = nullptr;
    ID3D11VertexShader* oldVS = nullptr;
    ID3D11PixelShader* oldPS = nullptr;
    ID3D11GeometryShader* oldGS = nullptr;
    ID3D11InputLayout* oldInputLayout = nullptr;
    ID3D11Buffer* oldVSCB[1] = {};
    ID3D11Buffer* oldPSCB[1] = {};
    ID3D11Buffer* oldVB[1] = {};
    ID3D11ShaderResourceView* oldSRV[2] = {};
    ID3D11SamplerState* oldSampler[2] = {};
    D3D11_PRIMITIVE_TOPOLOGY oldTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D11_VIEWPORT oldViewport[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    UINT oldBlendMask = 0;
    UINT oldStencilRef = 0;
    UINT oldStride = 0;
    UINT oldOffset = 0;
    float oldBlendFactor[4] = {};

    context->OMGetRenderTargets(8, oldRTV, &oldDSV);
    context->OMGetBlendState(&oldBlend, oldBlendFactor, &oldBlendMask);
    context->OMGetDepthStencilState(&oldDepthStencil, &oldStencilRef);
    context->RSGetState(&oldRasterizer);
    context->RSGetViewports(&numViewports, oldViewport);
    context->VSGetShader(&oldVS, nullptr, nullptr);
    context->PSGetShader(&oldPS, nullptr, nullptr);
    context->GSGetShader(&oldGS, nullptr, nullptr);
    context->VSGetConstantBuffers(0, 1, oldVSCB);
    context->PSGetConstantBuffers(0, 1, oldPSCB);
    context->PSGetShaderResources(0, 2, oldSRV);
    context->PSGetSamplers(0, 2, oldSampler);
    context->IAGetInputLayout(&oldInputLayout);
    context->IAGetPrimitiveTopology(&oldTopology);
    context->IAGetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);

    D3D11_VIEWPORT viewport = {
        0.0f,
        0.0f,
        static_cast<float>(sceneDesc.Width),
        static_cast<float>(sceneDesc.Height),
        0.0f,
        1.0f
    };

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    context->VSSetShader(vs.Get(), nullptr, 0);
    context->PSSetShader(ps.Get(), nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
    ID3D11ShaderResourceView* shaderResources[2] = { sceneCopySRV.Get(), grainSRV.Get() };
    ID3D11SamplerState* samplers[2] = { sceneSampler.Get(), grainSampler.Get() };
    context->PSSetShaderResources(0, 2, shaderResources);
    context->PSSetSamplers(0, 2, samplers);
    context->RSSetState(rasterizer.Get());
    context->RSSetViewports(1, &viewport);
    context->OMSetDepthStencilState(depthStencil.Get(), 0);
    context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    context->OMSetRenderTargets(1, &sceneColor, nullptr);
    context->Draw(3, 0);

    ID3D11ShaderResourceView* nullSRV[2] = {};
    context->PSSetShaderResources(0, 2, nullSRV);

    context->OMSetRenderTargets(8, oldRTV, oldDSV);
    context->OMSetBlendState(oldBlend, oldBlendFactor, oldBlendMask);
    context->OMSetDepthStencilState(oldDepthStencil, oldStencilRef);
    context->RSSetState(oldRasterizer);
    context->RSSetViewports(numViewports, oldViewport);
    context->VSSetShader(oldVS, nullptr, 0);
    context->PSSetShader(oldPS, nullptr, 0);
    context->GSSetShader(oldGS, nullptr, 0);
    context->VSSetConstantBuffers(0, 1, oldVSCB);
    context->PSSetConstantBuffers(0, 1, oldPSCB);
    context->PSSetShaderResources(0, 2, oldSRV);
    context->PSSetSamplers(0, 2, oldSampler);
    context->IASetInputLayout(oldInputLayout);
    context->IASetPrimitiveTopology(oldTopology);
    context->IASetVertexBuffers(0, 1, oldVB, &oldStride, &oldOffset);

    for (auto* renderTarget : oldRTV)
    {
        if (renderTarget)
        {
            renderTarget->Release();
        }
    }

    if (oldDSV) oldDSV->Release();
    if (oldBlend) oldBlend->Release();
    if (oldDepthStencil) oldDepthStencil->Release();
    if (oldRasterizer) oldRasterizer->Release();
    if (oldVS) oldVS->Release();
    if (oldPS) oldPS->Release();
    if (oldGS) oldGS->Release();
    if (oldInputLayout) oldInputLayout->Release();
    if (oldVSCB[0]) oldVSCB[0]->Release();
    if (oldPSCB[0]) oldPSCB[0]->Release();
    if (oldVB[0]) oldVB[0]->Release();
    if (oldSRV[0]) oldSRV[0]->Release();
    if (oldSRV[1]) oldSRV[1]->Release();
    if (oldSampler[0]) oldSampler[0]->Release();
    if (oldSampler[1]) oldSampler[1]->Release();
}

bool MGS3FilmGrain::HasActiveGrain()
{
    return mode != Mode::Off &&
        NativeAllowsFilmGrain() &&
        (mode == Mode::AllScenes ||
         gPendingAlpha.load(std::memory_order_relaxed) > 0 ||
         gLatchedAlpha.load(std::memory_order_relaxed) > 0);
}

void MGS3FilmGrain::BeginPresent()
{
    insidePresent.store(true, std::memory_order_relaxed);
}

void MGS3FilmGrain::EndPresent()
{
    insidePresent.store(false, std::memory_order_relaxed);
    drewBackbufferThisFrame.store(false, std::memory_order_relaxed);
}

void MGS3FilmGrain::OnAfterGameDraw(ID3D11DeviceContext* context, UINT vertexCount)
{
    if (!(eGameType & MGS3) ||
        mode == Mode::Off ||
        drawingGrain ||
        insidePresent.load(std::memory_order_relaxed) ||
        !HasActiveGrain() ||
        drewBackbufferThisFrame.load(std::memory_order_relaxed))
    {
        return;
    }

    ID3D11RenderTargetView* currentRTV = nullptr;
    if (!IsFullScreenBackbufferDraw(context, vertexCount, &currentRTV))
    {
        return;
    }

    drewBackbufferThisFrame.store(true, std::memory_order_relaxed);
    drawingGrain = true;
    OnPreMenuRender(currentRTV, nullptr);
    drawingGrain = false;

    if (currentRTV)
    {
        currentRTV->Release();
    }
}
