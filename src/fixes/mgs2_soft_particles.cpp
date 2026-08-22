#include "stdafx.h"

#include "mgs2_soft_particles.hpp"
#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"
#include "scene_depth.hpp"

#include <atomic>
#include <mutex>
#include <vector>

// Spray puffs and lamp glows are flat sprites, so the z-test cuts them along a hard line wherever
// they pass through water, ground or the wall they sit on. Soft particles: fade each pixel by how
// far the scene sits behind it, as a fraction of the sprite's own radius.

namespace
{
    struct SoftTexture
    {
        uint64_t hash;
        UINT size;
        float fade;      // fade span as a fraction of the half-size
        float feather;   // and never narrower than this many pixels
        float bias;      // glows are volumes: start the fade this many half-sizes behind the surface, no z-test
    };

    constexpr SoftTexture kSoftTextures[] = {
        { 0x13de2bb7654dd64eull, 64, 0.5f, 8.0f, 0.0f },   // splash03_alp   user\morita\splash\splash.c
        { 0x29e8ce42c45836c5ull, 64, 0.5f, 8.0f, 0.0f },   // splash05_alp   user\morita\splash\splash.c
        { 0x812baf1662a8d98cull, 64, 0.5f, 8.0f, 0.0f },   // bombgas6_alp   user\okajima\effect\bomb_gas.c
        { 0x2da04743caa928e5ull, 64, 2.0f, 0.0f, 1.0f },   // xlit04a_alp    user\skoba\test\c4_eff.c (the C4 lamp)
        { 0xac9aef320ed7b850ull, 32, 2.0f, 0.0f, 1.0f },   // drop01_msk     user\shibata\effect\cam_lamp.c
        { 0xd1d6b062c0903383ull, 64, 2.0f, 0.0f, 1.0f },   // ray_eye_bonbori_alp  user\kunibe\effect\cypher_light.c (camera lamp)
        { 0xa661471843db65d7ull, 64, 2.0f, 0.0f, 1.0f },   // xlit01b_msk    user\okajima\t_irs\trap_c4.c, irs.c (bomb panel and IR sensor lamps)
    };
    constexpr int kSoftTextureCount = static_cast<int>(std::size(kSoftTextures));

    // Prim.fx's textured sprite and poly paths (FOG=1 when the game's fog build is bound), plus the
    // sprite's true half-size in TEXCOORD0.z for the fade; a poly has no radius and feathers by pixels.
    const char* kShader = R"(
    cbuffer Globals : register(b0) { float4 c[486]; }
    cbuffer Soft    : register(b1) { float fadeRadius; float featherPixels; float biasRadius; }
    Texture2D        tex        : register(t0);
    Texture2D<float> sceneDepth : register(t1);
    SamplerState     samp       : register(s0);

    struct VSIn { float3 pos : POSITION; int4 uvdxdy : TEXCOORD0; float4 col : TEXCOORD1; };
    struct PSIn { float4 pos : SV_Position; float4 uv : TEXCOORD0; float4 col : TEXCOORD1; };

    PSIn VS(VSIn i)
    {
        PSIn o;
        float4 p = float4(i.pos, 1.0);
        float4 s = float4(dot(c[16], p), dot(c[17], p), dot(c[18], p), dot(c[19], p));
#ifdef POLY
        o.uv.xy = float2(i.uvdxdy.xy) / float(i.uvdxdy.z) * c[24].xy + c[24].zw;
        o.uv.z = 0.0;
#else
        s.xy += float2(i.uvdxdy.zw);
        o.uv.xy = float2(i.uvdxdy.xy) * c[24].xy / 4096.0 + c[24].zw;
        // A scaled custom world shrinks the whole view vector, so the half-size comes back out by w.
        o.uv.z = max(abs(float(i.uvdxdy.z)), abs(float(i.uvdxdy.w))) / s.w;
#endif
        o.pos = float4(dot(c[20], s), dot(c[21], s), dot(c[22], s), dot(c[23], s));
        o.uv.w = 0.0;
#ifdef FOG
        o.uv.w = clamp(saturate(o.pos.w * c[1].x + c[1].y), c[1].z, c[1].w);
#endif
        o.col = i.col * 2.0;
        return o;
    }

    float LinearZ(float d) { return c[22].w / (d - c[22].z); }

    float4 PS(PSIn i) : SV_Target
    {
        float4 o = saturate(tex.Sample(samp, i.uv.xy) * i.col);
        o.rgb = lerp(o.rgb, c[485].rgb, i.uv.w);
        o.a *= 2.0;
        // The pixel floor keeps a grazing puff from cutting hard. The slope is the smaller one-sided
        // difference per axis, so a silhouette jump never counts as slope.
        uint w, h;
        sceneDepth.GetDimensions(w, h);
        int2 px = int2(i.pos.xy);
        float zp = LinearZ(i.pos.z);
        float dc = LinearZ(sceneDepth.Load(int3(px, 0))) - zp;
        float dl = LinearZ(sceneDepth.Load(int3(max(px.x - 1, 0), px.y, 0))) - zp;
        float dr = LinearZ(sceneDepth.Load(int3(min(px.x + 1, int(w) - 1), px.y, 0))) - zp;
        float du = LinearZ(sceneDepth.Load(int3(px.x, max(px.y - 1, 0), 0))) - zp;
        float dd = LinearZ(sceneDepth.Load(int3(px.x, min(px.y + 1, int(h) - 1), 0))) - zp;
        float slope = min(abs(dc - dl), abs(dr - dc)) + min(abs(dc - du), abs(dd - dc));
        float feather = max(max(i.uv.z * fadeRadius, slope * featherPixels), 1e-3);
        o.a *= saturate((dc + i.uv.z * biasRadius) / feather);
        return o;
    }
    )";

    struct Tracked
    {
        ComPtr<ID3D11Resource> texture;   // held, so a freed one can never lend its address
        int entry;
    };

    std::mutex gLock;
    std::vector<Tracked> gTextures;
    std::atomic<bool> gArmed{ false };

    SafetyHookInline gDrawIndexedHook{};
    SafetyHookInline gUpdateSubHook{};
    ComPtr<ID3D11VertexShader> gVS[5];   // indexed by D3D11Hooks::StockVS
    ComPtr<ID3D11PixelShader> gPS;
    ComPtr<ID3D11Buffer> gFade[kSoftTextureCount];
    ComPtr<ID3D11DepthStencilState> gNoDepthTest;
    bool gShaderFailed = false;
    uint64_t gDepthFrame = UINT64_MAX;

    int SoftTextureEntry(const D3D11_TEXTURE2D_DESC& desc, const void* data, UINT rowPitch)
    {
        if (desc.Width != desc.Height || rowPitch < desc.Width * 4
            || (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM))
        {
            return -1;
        }
        uint64_t hash = 0;
        UINT hashed = 0;
        for (int i = 0; i < kSoftTextureCount; i++)
        {
            if (kSoftTextures[i].size != desc.Width)
            {
                continue;
            }
            if (hashed != desc.Width)
            {
                hash = Util::HashTexels(data, rowPitch, desc.Width, desc.Height);
                hashed = desc.Width;
            }
            if (hash == kSoftTextures[i].hash)
            {
                return i;
            }
        }
        return -1;
    }

    void STDMETHODCALLTYPE HookedUpdateSubresource(ID3D11DeviceContext* ctx, ID3D11Resource* dst,
        UINT subresource, const D3D11_BOX* box, const void* data, UINT rowPitch, UINT depthPitch)
    {
        ComPtr<ID3D11Texture2D> tex;
        if (dst && data && !box && subresource == 0 && SUCCEEDED(dst->QueryInterface(IID_PPV_ARGS(tex.GetAddressOf()))))
        {
            D3D11_TEXTURE2D_DESC desc{};
            tex->GetDesc(&desc);
            if (const int entry = SoftTextureEntry(desc, data, rowPitch); entry >= 0)
            {
                const std::lock_guard<std::mutex> guard(gLock);
                gTextures.push_back({ dst, entry });
                gArmed.store(true, std::memory_order_relaxed);
            }
        }
        gUpdateSubHook.stdcall<void>(ctx, dst, subresource, box, data, rowPitch, depthPitch);
    }

    int BoundSoftTexture(ID3D11DeviceContext* ctx)
    {
        ComPtr<ID3D11ShaderResourceView> srv;
        ctx->PSGetShaderResources(0, 1, srv.GetAddressOf());
        ComPtr<ID3D11Resource> res;
        if (srv)
        {
            srv->GetResource(res.GetAddressOf());
        }
        const std::lock_guard<std::mutex> guard(gLock);
        for (const Tracked& tracked : gTextures)
        {
            if (tracked.texture.Get() == res.Get())
            {
                return tracked.entry;
            }
        }
        return -1;
    }

    bool Compile(const char* entry, const char* target, const D3D_SHADER_MACRO* defines, ComPtr<ID3DBlob>& blob)
    {
        ComPtr<ID3DBlob> err;
        if (FAILED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, defines, nullptr, entry, target,
            0, 0, blob.GetAddressOf(), err.GetAddressOf())))
        {
            spdlog::error("MGS2SoftParticles: {} compile failed: {}", entry,
                err ? static_cast<const char*>(err->GetBufferPointer()) : "?");
            return false;
        }
        return true;
    }

    bool EnsureShaders()
    {
        if (gPS)
        {
            return true;
        }
        if (gShaderFailed)
        {
            return false;
        }
        gShaderFailed = true;

        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev || !g_D3D11Hooks.D3DCompileFunc)
        {
            return false;
        }
        using StockVS = D3D11Hooks::StockVS;
        const struct { StockVS kind; D3D_SHADER_MACRO defines[3]; } builds[] = {
            { StockVS::Sprite,    { { nullptr, nullptr } } },
            { StockVS::SpriteFog, { { "FOG", "1" }, { nullptr, nullptr } } },
            { StockVS::Poly,      { { "POLY", "1" }, { nullptr, nullptr } } },
            { StockVS::PolyFog,   { { "POLY", "1" }, { "FOG", "1" }, { nullptr, nullptr } } },
        };
        for (const auto& build : builds)
        {
            ComPtr<ID3DBlob> vs;
            if (!Compile("VS", "vs_5_0", build.defines, vs)
                || FAILED(dev->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr,
                    gVS[static_cast<int>(build.kind)].GetAddressOf())))
            {
                return false;
            }
        }
        ComPtr<ID3DBlob> ps;
        D3D11_DEPTH_STENCIL_DESC noTest{};
        if (!Compile("PS", "ps_5_0", nullptr, ps) || FAILED(dev->CreateDepthStencilState(&noTest, gNoDepthTest.GetAddressOf())))
        {
            return false;
        }
        for (int i = 0; i < kSoftTextureCount; i++)
        {
            const float fade[4] = { kSoftTextures[i].fade, kSoftTextures[i].feather, kSoftTextures[i].bias, 0.0f };
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(fade);
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            const D3D11_SUBRESOURCE_DATA init{ fade };
            if (FAILED(dev->CreateBuffer(&desc, &init, gFade[i].GetAddressOf())))
            {
                return false;
            }
        }
        if (FAILED(dev->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, gPS.GetAddressOf())))
        {
            return false;
        }
        gShaderFailed = false;
        return true;
    }

    // One copy per frame: the sprites draw after everything that writes depth.
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
        ComPtr<ID3D11VertexShader> theirVS;
        if (gArmed.load(std::memory_order_relaxed) && indexCount % 6 == 0)
        {
            ctx->VSGetShader(theirVS.GetAddressOf(), nullptr, nullptr);
        }
        const D3D11Hooks::StockVS kind = D3D11Hooks::GetStockVS(theirVS.Get());
        const int entry = kind != D3D11Hooks::StockVS::None ? BoundSoftTexture(ctx) : -1;
        ID3D11ShaderResourceView* depth = entry >= 0 ? SceneDepthForFrame(ctx) : nullptr;
        if (!depth || !EnsureShaders())
        {
            gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }

        ComPtr<ID3D11PixelShader> theirPS;
        ComPtr<ID3D11ShaderResourceView> theirSlot1;
        ComPtr<ID3D11Buffer> theirCB1;
        ctx->PSGetShader(theirPS.GetAddressOf(), nullptr, nullptr);
        ctx->PSGetShaderResources(1, 1, theirSlot1.GetAddressOf());
        ctx->PSGetConstantBuffers(1, 1, theirCB1.GetAddressOf());

        // A glow occludes by its own sphere in the shader, so the z-test would only cut it.
        ComPtr<ID3D11DepthStencilState> theirDSS;
        UINT theirStencilRef = 0;
        const bool volume = kSoftTextures[entry].bias > 0.0f;
        if (volume)
        {
            ctx->OMGetDepthStencilState(theirDSS.GetAddressOf(), &theirStencilRef);
            ctx->OMSetDepthStencilState(gNoDepthTest.Get(), 0);
        }

        ID3D11Buffer* fade = gFade[entry].Get();
        ctx->VSSetShader(gVS[static_cast<int>(kind)].Get(), nullptr, 0);
        ctx->PSSetShader(gPS.Get(), nullptr, 0);
        ctx->PSSetShaderResources(1, 1, &depth);
        ctx->PSSetConstantBuffers(1, 1, &fade);
        gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
        if (volume)
        {
            ctx->OMSetDepthStencilState(theirDSS.Get(), theirStencilRef);
        }
        ID3D11ShaderResourceView* restoreSRV = theirSlot1.Get();
        ID3D11Buffer* restoreCB = theirCB1.Get();
        ctx->PSSetShaderResources(1, 1, &restoreSRV);
        ctx->PSSetConstantBuffers(1, 1, &restoreCB);
        ctx->VSSetShader(theirVS.Get(), nullptr, 0);
        ctx->PSSetShader(theirPS.Get(), nullptr, 0);

        static bool logged = false;
        if (!logged)
        {
            logged = true;
            spdlog::info("MGS2SoftParticles: soft sprites active.");
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
