#include "stdafx.h"

#include "mgs2_soft_particles.hpp"
#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"
#include "scene_depth.hpp"

#include <atomic>
#include <mutex>
#include <vector>

// Big spray puffs sitting on the water get cut along a hard line by the z-test. Soft particles:
// fade each pixel by how far the scene sits behind it, scaled to the sprite's own radius.

namespace
{
    // splash03_alp, splash05_alp (user\morita\splash\splash.c); bombgas6_alp (user\okajima\effect\bomb_gas.c)
    constexpr uint64_t kSoftTextures[] = { 0x13de2bb7654dd64eull, 0x29e8ce42c45836c5ull, 0x812baf1662a8d98cull };
    constexpr UINT kTextureSize = 64;

    // Prim.fx's sprite path, plus the sprite's true half-size in TEXCOORD0.z for the fade.
    const char* kShader = R"(
    static const float kFadeRadius = 0.5;   // fade span as a fraction of the half-size

    cbuffer Globals : register(b0) { float4 c[25]; }
    Texture2D        tex        : register(t0);
    Texture2D<float> sceneDepth : register(t1);
    SamplerState     samp       : register(s0);

    struct VSIn { float3 pos : POSITION; int4 uvdxdy : TEXCOORD0; float4 col : TEXCOORD1; };
    struct PSIn { float4 pos : SV_Position; float3 uv : TEXCOORD0; float4 col : TEXCOORD1; };

    PSIn VS(VSIn i)
    {
        PSIn o;
        float4 p = float4(i.pos, 1.0);
        float4 s = float4(dot(c[16], p), dot(c[17], p), dot(c[18], p), dot(c[19], p));
        s.xy += float2(i.uvdxdy.zw);
        o.pos = float4(dot(c[20], s), dot(c[21], s), dot(c[22], s), dot(c[23], s));
        o.uv.xy = float2(i.uvdxdy.xy) * c[24].xy / 4096.0 + c[24].zw;
        // A scaled custom world shrinks the whole view vector, so the half-size comes back out by w.
        o.uv.z = max(abs(float(i.uvdxdy.z)), abs(float(i.uvdxdy.w))) / s.w;
        o.col = i.col * 2.0;
        return o;
    }

    float LinearZ(float d) { return c[22].w / (d - c[22].z); }

    float4 PS(PSIn i) : SV_Target
    {
        float4 o = saturate(tex.Sample(samp, i.uv.xy) * i.col) * float4(1, 1, 1, 2);
        float d = sceneDepth.Load(int3(i.pos.xy, 0));
        o.a *= saturate((LinearZ(d) - LinearZ(i.pos.z)) / (i.uv.z * kFadeRadius));
        return o;
    }
    )";

    std::mutex gLock;
    std::vector<ComPtr<ID3D11Resource>> gTextures;   // held, so a freed one can never lend its address
    std::atomic<bool> gArmed{ false };

    SafetyHookInline gDrawIndexedHook{};
    SafetyHookInline gUpdateSubHook{};
    ComPtr<ID3D11VertexShader> gVS;
    ComPtr<ID3D11PixelShader> gPS;
    bool gShaderFailed = false;
    uint64_t gDepthFrame = UINT64_MAX;

    bool IsSoftTexture(const D3D11_TEXTURE2D_DESC& desc, const void* data, UINT rowPitch)
    {
        if (desc.Width != kTextureSize || desc.Height != kTextureSize || rowPitch < kTextureSize * 4
            || (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM))
        {
            return false;
        }
        const uint64_t hash = Util::HashTexels(data, rowPitch, kTextureSize, kTextureSize);
        for (const uint64_t known : kSoftTextures)
        {
            if (hash == known)
            {
                return true;
            }
        }
        return false;
    }

    void STDMETHODCALLTYPE HookedUpdateSubresource(ID3D11DeviceContext* ctx, ID3D11Resource* dst,
        UINT subresource, const D3D11_BOX* box, const void* data, UINT rowPitch, UINT depthPitch)
    {
        ComPtr<ID3D11Texture2D> tex;
        if (dst && data && !box && subresource == 0 && SUCCEEDED(dst->QueryInterface(IID_PPV_ARGS(tex.GetAddressOf()))))
        {
            D3D11_TEXTURE2D_DESC desc{};
            tex->GetDesc(&desc);
            if (IsSoftTexture(desc, data, rowPitch))
            {
                const std::lock_guard<std::mutex> guard(gLock);
                gTextures.emplace_back(dst);
                gArmed.store(true, std::memory_order_relaxed);
            }
        }
        gUpdateSubHook.stdcall<void>(ctx, dst, subresource, box, data, rowPitch, depthPitch);
    }

    bool BoundToSoftTexture(ID3D11DeviceContext* ctx)
    {
        ComPtr<ID3D11ShaderResourceView> srv;
        ctx->PSGetShaderResources(0, 1, srv.GetAddressOf());
        ComPtr<ID3D11Resource> res;
        if (srv)
        {
            srv->GetResource(res.GetAddressOf());
        }
        const std::lock_guard<std::mutex> guard(gLock);
        for (const auto& tracked : gTextures)
        {
            if (tracked.Get() == res.Get())
            {
                return true;
            }
        }
        return false;
    }

    bool EnsureShaders()
    {
        if (gVS && gPS)
        {
            return true;
        }
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        auto compile = g_D3D11Hooks.D3DCompileFunc;
        ComPtr<ID3DBlob> vs, ps, err;
        if (gShaderFailed || !dev || !compile
            || FAILED(compile(kShader, strlen(kShader), nullptr, nullptr, nullptr, "VS", "vs_5_0", 0, 0, vs.GetAddressOf(), err.GetAddressOf()))
            || FAILED(compile(kShader, strlen(kShader), nullptr, nullptr, nullptr, "PS", "ps_5_0", 0, 0, ps.GetAddressOf(), err.ReleaseAndGetAddressOf()))
            || FAILED(dev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, gVS.GetAddressOf()))
            || FAILED(dev->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, gPS.GetAddressOf())))
        {
            if (!gShaderFailed)
            {
                spdlog::error("MGS2SoftParticles: shader setup failed: {}",
                    err ? static_cast<const char*>(err->GetBufferPointer()) : "no compiler");
            }
            gShaderFailed = true;
            return false;
        }
        return true;
    }

    // One copy per frame: the puffs draw after everything that writes depth.
    ID3D11ShaderResourceView* SceneDepthForFrame(ID3D11DeviceContext* ctx)
    {
        if (gDepthFrame != g_D3D11Hooks.FrameCount)
        {
            gDepthFrame = g_D3D11Hooks.FrameCount;
            ComPtr<ID3D11DepthStencilView> dsv;
            ctx->OMGetRenderTargets(0, nullptr, dsv.GetAddressOf());
            SceneDepth::CaptureDepth(dsv.Get());
        }
        return SceneDepth::GetSRV();
    }

    void STDMETHODCALLTYPE HookedDrawIndexed(ID3D11DeviceContext* ctx, UINT indexCount, UINT startIndex,
        INT baseVertex)
    {
        // Poly prims share the textures; only the sprite layout carries dx/dy.
        ComPtr<ID3D11VertexShader> theirVS;
        if (gArmed.load(std::memory_order_relaxed) && indexCount % 6 == 0)
        {
            ctx->VSGetShader(theirVS.GetAddressOf(), nullptr, nullptr);
        }
        ID3D11ShaderResourceView* depth = nullptr;
        if (!theirVS || !D3D11Hooks::IsStockSpriteVS(theirVS.Get()) || !BoundToSoftTexture(ctx)
            || !(depth = SceneDepthForFrame(ctx)) || !EnsureShaders())
        {
            gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }

        ComPtr<ID3D11PixelShader> theirPS;
        ComPtr<ID3D11ShaderResourceView> theirSlot1;
        ctx->PSGetShader(theirPS.GetAddressOf(), nullptr, nullptr);
        ctx->PSGetShaderResources(1, 1, theirSlot1.GetAddressOf());

        ctx->VSSetShader(gVS.Get(), nullptr, 0);
        ctx->PSSetShader(gPS.Get(), nullptr, 0);
        ctx->PSSetShaderResources(1, 1, &depth);
        gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
        ID3D11ShaderResourceView* restore = theirSlot1.Get();
        ctx->PSSetShaderResources(1, 1, &restore);
        ctx->VSSetShader(theirVS.Get(), nullptr, 0);
        ctx->PSSetShader(theirPS.Get(), nullptr, 0);

        static bool logged = false;
        if (!logged)
        {
            logged = true;
            spdlog::info("MGS2SoftParticles: soft spray active.");
        }
    }
}

void MGS2SoftParticles::OnDeviceReady()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!ctx)
    {
        spdlog::error("MGS2SoftParticles: no device context.");
        return;
    }

    void** vtable = *reinterpret_cast<void***>(ctx);
    gDrawIndexedHook = safetyhook::create_inline(vtable[12], reinterpret_cast<void*>(HookedDrawIndexed));
    gUpdateSubHook = safetyhook::create_inline(vtable[48], reinterpret_cast<void*>(HookedUpdateSubresource));
    LOG_HOOK(gDrawIndexedHook, "MGS 2: Soft particles: DrawIndexed");
    LOG_HOOK(gUpdateSubHook, "MGS 2: Soft particles: UpdateSubresource");
}
