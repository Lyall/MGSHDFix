#include "stdafx.h"
#include "common.hpp"
#include "mgs2_gas_haze.hpp"

#include "logging.hpp"
#include "d3d11_api.hpp"
#include "effect_speeds.hpp"
#include "scene_depth.hpp"

#include "gamevars.hpp"
#include "mgs2_status_flags.hpp"

using namespace MGS2_StatusFlags;

namespace
{
    // NewSmokeBlurEffect (smk_blur.c).
    constexpr const char* kAlphaSig         = "F3 0F 10 3D CC C6 18 00";
    constexpr ptrdiff_t   kAlphaHookOffset  = 8;
    constexpr float       kAlphaScale       = 64.0f;
    constexpr int         kAlphaFloor       = 0;
    constexpr float       kTintScale        = 1.0f;
    constexpr float       kAlphaGain        = 255.0f / 128.0f;
    constexpr float       kAlphaCap         = 1.0f;
    constexpr float       kFeedbackGain     = 1.0f;

    constexpr const char* kDieSig =
        "48 89 5C 24 08 57 48 83 EC 20 48 8B 59 60 48 8B F9 48 85 DB 74 10 48 8B CB";

    // smk_blur Act - fills the prim2 vertices each frame; we read them.
    constexpr const char* kActSig =
        "4C 8B DC 49 89 5B 18 49 89 73 20 55 57 41 54 41 55 41 56 49 8D 6B 98 48 81 EC 40 01 00 00";
    constexpr ptrdiff_t kWork_Clock    = 0x1c8;
    constexpr ptrdiff_t kWork_StartSpeed = 0x1d0;
    constexpr ptrdiff_t kWork_PrimBase = 0x60;
    constexpr ptrdiff_t kWork_NPrims   = 0x1e8;
    constexpr ptrdiff_t kPrim_PosBuf   = 0xc0;    // FVECTOR* pos[] (world-space vertex positions)
    constexpr ptrdiff_t kPrim_UvrgbBuf = 0xd0;
    constexpr ptrdiff_t kPrim_FlagBuf  = 0x268;
    constexpr ptrdiff_t kFlag_Stride   = 0x40;
    constexpr ptrdiff_t kFlag_Off      = 0x1a;
    constexpr uint16_t  kInvisible0    = 0x1000;
    constexpr int       kNVerts        = 17;
    constexpr ptrdiff_t kRenderPacketFname = 0x18;

    constexpr float     kUv2Norm   = 1.0f / 4096.0f;
    constexpr float     kWarp      = 0.98f;
    constexpr float     kDepthBias = 0.0f;

    // smk_blur uses a 512x448 framebuffer.
    constexpr float kDrawW = 512.0f;
    constexpr float kDrawH = 448.0f;

    SafetyHookInline g_hook{};
    SafetyHookMid    g_alphaHook{};
    SafetyHookInline g_actHook{};
    SafetyHookInline g_dieHook{};
    SafetyHookInline g_primRenderHook{};

    int g_lastClock   = -0x7fffffff;
    bool g_actUpdated = true;
    std::atomic<int> g_activeCount { 0 };

    // Keep clip-space W for near-plane clipping.
    struct GasVertex { float clipX, clipY, clipZ, clipW, u, v; uint32_t color; };

    std::mutex            g_vmtx;
    std::vector<GasVertex> g_verts;
    FMATRIX g_raiseEyePers = {};

    // ---- D3D11 resources -------------------------------------------------------------------------
    const char* kShader = R"(
    Texture2D    sceneTex : register(t0);
    Texture2D    depthTex : register(t1);
    SamplerState sampLin  : register(s0);
    SamplerState sampPt   : register(s1);
    cbuffer CB : register(b0) {
        float2 invDraw;      // 1/512, 1/448
        float2 screenSize;   // backbuffer w,h
        float  depthBias;
        float  feedbackGain;
        float2 _pad;
    };
    struct VSIn  { float4 clip:POSITION; float2 uv:TEXCOORD0; float4 col:COLOR0; };
    struct VSOut { float4 pos:SV_Position; float2 uv:TEXCOORD0; float4 col:COLOR0; };
    VSOut VS(VSIn i){
        VSOut o;
        // Convert KP clip space to D3D clip space.
        o.pos = float4(i.clip.x, -i.clip.y, 0.5 * (i.clip.z + i.clip.w), i.clip.w);
        o.uv = i.uv;
        o.col = i.col;
        return o;
    }
    float4 PS(VSOut i):SV_Target {
        // Each puff refracts the previous framebuffer.
        float3 c = sceneTex.Sample(sampLin, i.uv).rgb;
        c *= i.col.rgb * 1.9921875;   // GS modulate (vertex colour; 0x80 = neutral)
        c *= feedbackGain;
        c = saturate(c);
        float2 screenUV = i.pos.xy / screenSize;
        float  sceneZ = depthTex.Sample(sampPt, screenUV).r;
        // reversed-Z (far=0, nearer=larger): hide where scene geometry is nearer than this puff
        if (sceneZ > i.pos.z + depthBias) discard;
        return float4(c, i.col.a);
    }
    )";

    ComPtr<ID3D11VertexShader>      g_vs;
    ComPtr<ID3D11PixelShader>       g_ps;
    ComPtr<ID3D11InputLayout>       g_layout;
    ComPtr<ID3D11Buffer>            g_cb;
    ComPtr<ID3D11Buffer>            g_vb;
    UINT                            g_vbCap = 0;
    ComPtr<ID3D11BlendState>        g_blend;
    ComPtr<ID3D11RasterizerState>   g_rs;
    ComPtr<ID3D11DepthStencilState> g_dss;
    ComPtr<ID3D11SamplerState>      g_sampLin, g_sampPt;
    ComPtr<ID3D11Texture2D>         g_capTex;
    ComPtr<ID3D11ShaderResourceView> g_capSRV;
    UINT g_capW = 0, g_capH = 0;
    DXGI_FORMAT g_capFormat = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT g_capSrvFormat = DXGI_FORMAT_UNKNOWN;
    uint64_t g_lastCapFrame = UINT64_MAX;
    uint64_t g_drawnFrame = UINT64_MAX;
    uint64_t g_attemptedFrame = UINT64_MAX;
    bool g_d3dInit = false, g_d3dFailed = false;
    ID3D11Device* g_boundDevice = nullptr;

    bool IsSmokeRenderPacket(const void* packet)
    {
        if (!packet) return false;

        __try
        {
            const char* fname = *reinterpret_cast<const char* const*>(
                static_cast<const uint8_t*>(packet) + kRenderPacketFname);
            if (!fname) return false;

            const char* base = fname;
            for (size_t i = 0; i < 512; ++i)
            {
                const char c = fname[i];
                if (c == '\\' || c == '/') base = fname + i + 1;
                if (c == '\0') return _stricmp(base, "smk_blur.c") == 0;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return false;
    }

    void __fastcall PrimRenderPacket_Detour(void* packet)
    {
        if (!IsSmokeRenderPacket(packet))
        {
            g_primRenderHook.fastcall<void>(packet);
            return;
        }

        const uint64_t frame = g_D3D11Hooks.FrameCount;
        if (g_attemptedFrame != frame)
        {
            g_attemptedFrame = frame;
            static std::atomic<bool> logged = false;
            if (!logged.exchange(true))
            {
                spdlog::info("MGS 2: Gas Haze: native draw order active.");
            }
            MGS2GasHaze::DrawIntoCurrentTarget();
        }
    }

    // (Re)create the feedback capture texture to match the given surface. Returns true if recreated.
    bool EnsureCapTex(ID3D11Device* dev, const D3D11_TEXTURE2D_DESC& bb, DXGI_FORMAT srvFormat)
    {
        if (g_capTex && g_capSRV && g_capW == bb.Width && g_capH == bb.Height &&
            g_capFormat == bb.Format && g_capSrvFormat == srvFormat)
        {
            return false;
        }
        g_capSRV.Reset(); g_capTex.Reset();
        g_capW = g_capH = 0;
        g_capFormat = g_capSrvFormat = DXGI_FORMAT_UNKNOWN;
        D3D11_TEXTURE2D_DESC td = bb;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE; td.Usage = D3D11_USAGE_DEFAULT;
        td.CPUAccessFlags = 0; td.MiscFlags = 0; td.SampleDesc = { 1, 0 };
        td.MipLevels = 1; td.ArraySize = 1;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, g_capTex.GetAddressOf()))) return false;
        D3D11_SHADER_RESOURCE_VIEW_DESC sv = {}; sv.Format = srvFormat;
        sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; sv.Texture2D.MipLevels = 1;
        if (FAILED(dev->CreateShaderResourceView(g_capTex.Get(), &sv, g_capSRV.GetAddressOf())))
        {
            g_capTex.Reset();
            return false;
        }
        g_capW = bb.Width; g_capH = bb.Height;
        g_capFormat = bb.Format; g_capSrvFormat = srvFormat;
        spdlog::info("MGS 2: Gas Haze: feedback target {}x{} (resource {}, view {}).",
                     bb.Width, bb.Height, static_cast<int>(bb.Format), static_cast<int>(srvFormat));
        return true;
    }

    // ---- read the prim and build CPU triangles ----------------------------------------------------
    void AppendInstance(uintptr_t work)
    {
        int clock  = *reinterpret_cast<int*>(work + kWork_Clock);
        int nprims = *reinterpret_cast<int*>(work + kWork_NPrims);
        uintptr_t prim = *reinterpret_cast<uintptr_t*>(work + kWork_PrimBase + (ptrdiff_t)clock * 8);
        if (!prim || nprims <= 0) return;
        uintptr_t uvBuf   = *reinterpret_cast<uintptr_t*>(prim + kPrim_UvrgbBuf);
        uintptr_t posBuf  = *reinterpret_cast<uintptr_t*>(prim + kPrim_PosBuf);
        uintptr_t flagBuf = *reinterpret_cast<uintptr_t*>(prim + kPrim_FlagBuf);
        if (!uvBuf) return;
        const float* M = g_raiseEyePers.m[0];

        for (int j = 0; j < nprims; ++j)
        {
            if (flagBuf)
            {
                uint16_t f = *reinterpret_cast<uint16_t*>(flagBuf + kFlag_Off + (ptrdiff_t)j * kFlag_Stride);
                if (f & kInvisible0) continue;
            }
            const uint8_t* vbase = reinterpret_cast<const uint8_t*>(uvBuf + (size_t)j * kNVerts * 0x10);

            for (int i = 2; i < kNVerts; ++i)
            {
                uint16_t kf = *reinterpret_cast<const uint16_t*>(vbase + (size_t)i * 0x10 + 0xe);
                if (kf & 0x8000) continue;

                GasVertex tri[3];
                for (int k = 0; k < 3; ++k)
                {
                    const uint8_t* uv = vbase + (size_t)(i - 2 + k) * 0x10;
                    // Clamp during sampling to preserve the UV gradient.
                    int ui = *reinterpret_cast<const int16_t*>(uv + 8);
                    int vi = *reinterpret_cast<const int16_t*>(uv + 0xa);
                    float un = ui * kUv2Norm;
                    float vn = vi * kUv2Norm;
                    int a = (int)(uv[6] * kAlphaGain) + kAlphaFloor;
                    int cap = (int)(kAlphaCap * 255.0f);
                    if (a > cap) a = cap;
                    int tr = (int)(uv[0] * kTintScale); if (tr > 255) tr = 255;
                    int tg = (int)(uv[2] * kTintScale); if (tg > 255) tg = 255;
                    int tb = (int)(uv[4] * kTintScale); if (tb > 255) tb = 255;
                    tri[k].u = un;
                    tri[k].v = vn;
                    tri[k].color = uint32_t(tr) | (uint32_t(tg) << 8) | (uint32_t(tb) << 16) | (uint32_t(a) << 24);

                    // Fallback for a missing position buffer.
                    const float screenX = (0.5f + (un - 0.5f) / kWarp) * kDrawW;
                    const float screenY = (0.5f + (vn - 0.5f) / kWarp) * kDrawH;
                    tri[k].clipX = screenX * (2.0f / kDrawW) - 1.0f;
                    tri[k].clipY = screenY * (2.0f / kDrawH) - 1.0f;
                    tri[k].clipZ = 0.0f;
                    tri[k].clipW = 1.0f;

                    // Match the native Prim2 projection.
                    if (posBuf)
                    {
                        const float* wp = reinterpret_cast<const float*>(
                            posBuf + (size_t)((j * kNVerts) + (i - 2 + k)) * 0x10);
                        float wx = wp[0], wy = wp[1], wz = wp[2];
                        tri[k].clipX = wx*M[0] + wy*M[4] + wz*M[8]  + M[12];
                        tri[k].clipY = wx*M[1] + wy*M[5] + wz*M[9]  + M[13];
                        tri[k].clipZ = wx*M[2] + wy*M[6] + wz*M[10] + M[14];
                        tri[k].clipW = wx*M[3] + wy*M[7] + wz*M[11] + M[15];
                    }
                }

                std::lock_guard<std::mutex> lk(g_vmtx);
                for (int k = 0; k < 3; ++k) g_verts.push_back(tri[k]);
            }
        }
    }

    void BuildSmoke(uintptr_t work)
    {
        if (const int clock = g_GameVars.DG_Clock(); clock != g_lastClock)
        {
            g_lastClock = clock;
            if (g_actUpdated) memcpy(&g_raiseEyePers, &g_GameVars.DG_Chanl(0)->raise_eye_pers, sizeof(FMATRIX));
            std::lock_guard<std::mutex> lk(g_vmtx);
            g_verts.clear();
        }
        AppendInstance(work);
    }


    void __fastcall Act_Detour(uintptr_t work)
    {
        // Hold actor state and feedback on the same tick.
        g_actUpdated = !EffectSpeedFix::IsFeedbackHoldTick();
        if (g_actUpdated) g_actHook.fastcall<void>(work);
        if (!work) return;
        __try { BuildSmoke(work); }   // re-read even when held: the frozen prims still draw this frame
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void* __fastcall NewSmokeBlur_Detour(uintptr_t world, int start_speed, int end_speed, int start_size,
                                         int end_size, int spot_size, int spot_angle, int n_prims,
                                         int interval, int color, int flag)
    {
        void* work = g_hook.fastcall<void*>(world, start_speed, end_speed, start_size, end_size, spot_size,
                                            spot_angle, n_prims, interval, color, flag);
        if (work)
        {
            // Avoid the port's divide by zero without moving the puff.
            if (start_speed == 0)
            {
                *reinterpret_cast<float*>(static_cast<uint8_t*>(work) + kWork_StartSpeed) = 1.0e-7f;
            }
            g_activeCount.fetch_add(1, std::memory_order_relaxed);
        }
        return work;
    }

    void __fastcall Die_Detour(uintptr_t actor)
    {
        g_dieHook.fastcall<void>(actor);
        if (g_activeCount.fetch_sub(1, std::memory_order_relaxed) <= 1)
        {
            g_activeCount.store(0, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lk(g_vmtx);
            g_verts.clear();
        }
    }

    bool EnsureD3D(ID3D11Device* dev)
    {
        if (dev != g_boundDevice)
        {
            g_vs.Reset(); g_ps.Reset(); g_layout.Reset(); g_cb.Reset(); g_vb.Reset();
            g_blend.Reset(); g_rs.Reset(); g_dss.Reset(); g_sampLin.Reset(); g_sampPt.Reset();
            g_capSRV.Reset(); g_capTex.Reset();
            g_vbCap = g_capW = g_capH = 0;
            g_capFormat = g_capSrvFormat = DXGI_FORMAT_UNKNOWN;
            g_lastCapFrame = g_drawnFrame = UINT64_MAX;
            g_d3dInit = g_d3dFailed = false;
            g_boundDevice = dev;
        }
        if (g_d3dInit) return true;
        if (g_d3dFailed) return false;
        if (!dev) return false;

        if (!g_D3D11Hooks.D3DCompileFunc)
        {
            g_d3dFailed = true;
            spdlog::error("MGS 2: Gas Haze: d3dcompiler_43.dll is unavailable.");
            return false;
        }

        ComPtr<ID3DBlob> vsb, psb, err;
        bool ok = g_D3D11Hooks.D3DCompileFunc && SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                        "VS", "vs_5_0", 0, 0, vsb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (ok) 
            ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                        "PS", "ps_5_0", 0, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (!ok)
        {
            spdlog::error("MGS 2: Gas Haze: shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
            g_d3dFailed = true; 
            return false;
        }

        if (FAILED(dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, g_vs.GetAddressOf())) ||
            FAILED(dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, g_ps.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_INPUT_ELEMENT_DESC il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        if (FAILED(dev->CreateInputLayout(il, 3, vsb->GetBufferPointer(), vsb->GetBufferSize(), g_layout.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_BUFFER_DESC cbd = {}; cbd.ByteWidth = 32; cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(dev->CreateBuffer(&cbd, nullptr, g_cb.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_BLEND_DESC bd = {}; auto& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;  rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA; rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ZERO;  rt.DestBlendAlpha = D3D11_BLEND_ONE;       rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(dev->CreateBlendState(&bd, g_blend.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_RASTERIZER_DESC rd = {}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE;
        rd.DepthClipEnable = TRUE;
        if (FAILED(dev->CreateRasterizerState(&rd, g_rs.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_DEPTH_STENCIL_DESC dsd = {}; dsd.DepthEnable = FALSE; dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        if (FAILED(dev->CreateDepthStencilState(&dsd, g_dss.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        D3D11_SAMPLER_DESC sd = {}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(dev->CreateSamplerState(&sd, g_sampLin.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        if (FAILED(dev->CreateSamplerState(&sd, g_sampPt.GetAddressOf())))
        {
            g_d3dFailed = true;
            return false;
        }

        g_d3dInit = true;
        spdlog::info("MGS 2: Gas Haze: D3D11 pass initialized.");
        return true;
    }
}

void MGS2GasHaze::DrawIntoCurrentTarget()
{
    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!ctx) return;

    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);
    if (rtv)
    {
        ID3D11ShaderResourceView* depth = dsv ? SceneDepth::CaptureDepth(dsv) : nullptr;
        DrawInto(rtv, depth);
        rtv->Release();
    }
    if (dsv) dsv->Release();
}

void MGS2GasHaze::DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth)
{
    if (!(eGameType & MGS2) || !bEnabled || !sceneColor)
    {
        return;
    }
    if (g_GameVars.GM_MenuStatus() & MENU_RADIO_ON)
    {
        return;
    }
    // Snapshot this frame's geometry.
    std::vector<GasVertex> verts;
    {
        std::lock_guard<std::mutex> lk(g_vmtx);
        if (g_verts.empty()) return;
        verts = g_verts;
    }

    auto* dev = g_D3D11Hooks.d3dDevice.Get();
    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!dev || !ctx || !EnsureD3D(dev)) return;

    // The scene colour RT (3D rendered, no UI yet) is both our draw target and the warp source.
    ComPtr<ID3D11Resource> colorRes;
    sceneColor->GetResource(colorRes.GetAddressOf());
    ComPtr<ID3D11Texture2D> backbuf;
    if (!colorRes || FAILED(colorRes.As(&backbuf)) || !backbuf) return;
    D3D11_TEXTURE2D_DESC bb; backbuf->GetDesc(&bb);
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc {}; sceneColor->GetDesc(&rtvDesc);
    const DXGI_FORMAT srvFormat = rtvDesc.Format != DXGI_FORMAT_UNKNOWN ? rtvDesc.Format : bb.Format;

    if (bb.SampleDesc.Count != 1) return;   // MSAA scene RT not supported by the plain copy/SRV

    // Warp source = previous frame's result (PS2 BP_PreviousFrameTexture); fed back for accumulation.
    const bool justCreated = EnsureCapTex(dev, bb, srvFormat);
    if (!g_capTex || !g_capSRV) return;
    const uint64_t frame = g_D3D11Hooks.FrameCount;
    const bool stale = g_lastCapFrame == UINT64_MAX;
    const bool reseed = justCreated || stale;
    if (reseed)
    {
        ctx->CopyResource(g_capTex.Get(), backbuf.Get());
        g_lastCapFrame = frame;
    }

    // Grow the dynamic vertex buffer if needed.
    UINT need = (UINT)verts.size();
    if (!g_vb || g_vbCap < need)
    {
        g_vb.Reset();
        g_vbCap = need + 512;
        D3D11_BUFFER_DESC vd = {}; vd.ByteWidth = g_vbCap * sizeof(GasVertex);
        vd.Usage = D3D11_USAGE_DYNAMIC; vd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(dev->CreateBuffer(&vd, nullptr, g_vb.GetAddressOf()))) { g_vbCap = 0; return; }
    }
    {
        D3D11_MAPPED_SUBRESOURCE m;
        if (FAILED(ctx->Map(g_vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return;
        memcpy(m.pData, verts.data(), need * sizeof(GasVertex));
        ctx->Unmap(g_vb.Get(), 0);
    }
    {
        D3D11_MAPPED_SUBRESOURCE m;
        if (FAILED(ctx->Map(g_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return;
        float cb[8] = { 1.0f / kDrawW, 1.0f / kDrawH, (float)bb.Width, (float)bb.Height,
                        kDepthBias, kFeedbackGain, 0.0f, 0.0f };
        memcpy(m.pData, cb, sizeof(cb));
        ctx->Unmap(g_cb.Get(), 0);
    }

    D3D11_VIEWPORT vp = { 0, 0, (float)bb.Width, (float)bb.Height, 0.f, 1.f };
    ID3D11ShaderResourceView* depthSRV = depth;   // null -> shader sees 0 -> no occlusion

    // Save state.
    ID3D11RenderTargetView* oRTV[8] = {}; ID3D11DepthStencilView* oDSV = nullptr;
    ID3D11BlendState* oBlend = nullptr; FLOAT oBF[4]; UINT oBM = 0;
    ID3D11DepthStencilState* oDSS = nullptr; UINT oSR = 0;
    ID3D11RasterizerState* oRS = nullptr; ID3D11VertexShader* oVS = nullptr; ID3D11PixelShader* oPS = nullptr;
    ID3D11GeometryShader* oGS = nullptr; ID3D11HullShader* oHS = nullptr; ID3D11DomainShader* oDS = nullptr;
    ID3D11InputLayout* oIL = nullptr; ID3D11Buffer* oVB = nullptr; UINT oStr = 0, oOff = 0;
    ID3D11Buffer* oCB = nullptr; ID3D11ShaderResourceView* oSRV[2] = {}; ID3D11SamplerState* oSamp[2] = {};
    ID3D11Buffer* oVCB = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY oTopo;
    D3D11_VIEWPORT oVP[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT oNVP = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->OMGetRenderTargets(8, oRTV, &oDSV);
    ctx->VSGetConstantBuffers(0, 1, &oVCB);
    ctx->OMGetBlendState(&oBlend, oBF, &oBM);
    ctx->OMGetDepthStencilState(&oDSS, &oSR);
    ctx->RSGetState(&oRS); ctx->RSGetViewports(&oNVP, oVP);
    ctx->VSGetShader(&oVS, nullptr, nullptr); ctx->PSGetShader(&oPS, nullptr, nullptr);
    ctx->GSGetShader(&oGS, nullptr, nullptr); ctx->HSGetShader(&oHS, nullptr, nullptr);
    ctx->DSGetShader(&oDS, nullptr, nullptr);
    ctx->IAGetInputLayout(&oIL); ctx->IAGetVertexBuffers(0, 1, &oVB, &oStr, &oOff);
    ctx->IAGetPrimitiveTopology(&oTopo);
    ctx->PSGetConstantBuffers(0, 1, &oCB); ctx->PSGetShaderResources(0, 2, oSRV); ctx->PSGetSamplers(0, 2, oSamp);

    // Draw.
    UINT stride = sizeof(GasVertex), offset = 0;
    ID3D11ShaderResourceView* srvs[2] = { g_capSRV.Get(), depthSRV };
    ID3D11SamplerState* samps[2] = { g_sampLin.Get(), g_sampPt.Get() };
    ctx->IASetInputLayout(g_layout.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, g_vb.GetAddressOf(), &stride, &offset);
    ctx->VSSetShader(g_vs.Get(), nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, g_cb.GetAddressOf());
    ctx->PSSetShader(g_ps.Get(), nullptr, 0);
    ctx->PSSetConstantBuffers(0, 1, g_cb.GetAddressOf());
    ctx->PSSetShaderResources(0, 2, srvs);
    ctx->PSSetSamplers(0, 2, samps);
    ctx->OMSetBlendState(g_blend.Get(), nullptr, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_dss.Get(), 0);
    ctx->RSSetState(g_rs.Get());
    ctx->RSSetViewports(1, &vp);
    ctx->OMSetRenderTargets(1, &sceneColor, nullptr);
    ctx->Draw(need, 0);
    g_drawnFrame = frame;

    // Restore.
    ctx->OMSetRenderTargets(8, oRTV, oDSV);
    ctx->OMSetBlendState(oBlend, oBF, oBM);
    ctx->OMSetDepthStencilState(oDSS, oSR);
    ctx->RSSetState(oRS); ctx->RSSetViewports(oNVP, oVP);
    ctx->VSSetShader(oVS, nullptr, 0); ctx->PSSetShader(oPS, nullptr, 0);
    ctx->GSSetShader(oGS, nullptr, 0); ctx->HSSetShader(oHS, nullptr, 0);
    ctx->DSSetShader(oDS, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &oVCB);
    ctx->IASetInputLayout(oIL); ctx->IASetVertexBuffers(0, 1, &oVB, &oStr, &oOff);
    ctx->IASetPrimitiveTopology(oTopo);
    ctx->PSSetConstantBuffers(0, 1, &oCB); ctx->PSSetShaderResources(0, 2, oSRV); ctx->PSSetSamplers(0, 2, oSamp);
    for (auto* r : oRTV) if (r) r->Release();
    if (oDSV) oDSV->Release(); if (oBlend) oBlend->Release(); if (oDSS) oDSS->Release();
    if (oRS) oRS->Release(); if (oVS) oVS->Release(); if (oPS) oPS->Release();
    if (oGS) oGS->Release(); if (oHS) oHS->Release(); if (oDS) oDS->Release();
    if (oIL) oIL->Release();
    if (oVB) oVB->Release(); if (oCB) oCB->Release(); if (oVCB) oVCB->Release();
    for (auto* s : oSRV) if (s) s->Release();
    for (auto* s : oSamp) if (s) s->Release();

}

void MGS2GasHaze::OnPreMenuRender(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth)
{
    if (!(eGameType & MGS2) || !bEnabled || !sceneColor) return;

    const uint64_t frame = g_D3D11Hooks.FrameCount;
    if (g_drawnFrame != frame)
    {
        DrawInto(sceneColor, depth);
    }
}

bool MGS2GasHaze::IsActive()
{
    return bEnabled && g_activeCount.load(std::memory_order_relaxed) > 0;
}

void MGS2GasHaze::OnFeedbackCapture(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView*)
{
    if (!(eGameType & MGS2) || !bEnabled || !sceneColor) return;

    auto* dev = g_D3D11Hooks.d3dDevice.Get();
    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!dev || !ctx || !EnsureD3D(dev)) return;

    ComPtr<ID3D11Resource> colorRes;
    sceneColor->GetResource(colorRes.GetAddressOf());
    ComPtr<ID3D11Texture2D> sceneTex;
    if (!colorRes || FAILED(colorRes.As(&sceneTex)) || !sceneTex) return;

    D3D11_TEXTURE2D_DESC desc {}; sceneTex->GetDesc(&desc);
    if (desc.SampleDesc.Count != 1) return;
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc {}; sceneColor->GetDesc(&rtvDesc);
    const DXGI_FORMAT srvFormat = rtvDesc.Format != DXGI_FORMAT_UNKNOWN ? rtvDesc.Format : desc.Format;
    EnsureCapTex(dev, desc, srvFormat);
    if (!g_capTex || !g_capSRV) return;

    // Capture after scene effects and before subtitles.
    if (EffectSpeedFix::IsFeedbackHoldTick()) return;
    ctx->CopyResource(g_capTex.Get(), sceneTex.Get());
    g_lastCapFrame = g_D3D11Hooks.FrameCount;
}

void MGS2GasHaze::InvalidateCapture()
{
    g_lastCapFrame = UINT64_MAX;
}

void MGS2GasHaze::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Gas Haze: disabled by config.");
        return;
    }

    if (g_GameVars.DG_Chanl(0) == nullptr)
    {
        spdlog::info("MGS 2: Gas Haze: DG_Chanl scan failed; fix disabled.");
        return;
    }

    if (uint8_t* address = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC 20 8B 7C 24 ?? 41 8B E8 41 8B F1 44 8B F2 45 33 C9 4C 8B F9 BA 00 20 00 00", "MGS 2: Gas Haze - NewSmokeBlurEffect"))
    {
        g_hook = safetyhook::create_inline(address, reinterpret_cast<void*>(NewSmokeBlur_Detour));
        LOG_HOOK(g_hook, "MGS 2: Gas Haze - NewSmokeBlurEffect");
    }

    if (uint8_t* alpha = Memory::PatternScan(baseModule, kAlphaSig, "MGS 2: Gas Haze - Alpha"))
    {
        g_alphaHook = safetyhook::create_mid(alpha + kAlphaHookOffset,
            [](SafetyHookContext& ctx) { ctx.xmm7.f32[0] = kAlphaScale; });
        LOG_HOOK(g_alphaHook, "MGS 2: Gas Haze - Alpha");
    }

    if (uint8_t* act = Memory::PatternScan(baseModule, kActSig, "MGS 2: Gas Haze - Act"))
    {
        g_actHook = safetyhook::create_inline(act, reinterpret_cast<void*>(Act_Detour));
        LOG_HOOK(g_actHook, "MGS 2: Gas Haze - Act");
    }

    if (uint8_t* die = Memory::PatternScan(baseModule, kDieSig, "MGS 2: Gas Haze - Die"))
    {
        g_dieHook = safetyhook::create_inline(die, reinterpret_cast<void*>(Die_Detour));
        LOG_HOOK(g_dieHook, "MGS 2: Gas Haze - Die");
    }

    if (uint8_t* primRender = Memory::PatternScan(
            baseModule,
            "4C 8B DC 53 48 83 EC ?? 83 79 ?? 00",
            "MGS 2: Gas Haze - BP_Prim_RenderPacket"))
    {
        g_primRenderHook = safetyhook::create_inline(
            primRender, reinterpret_cast<void*>(PrimRenderPacket_Detour));
        LOG_HOOK(g_primRenderHook, "MGS 2: Gas Haze - BP_Prim_RenderPacket");
    }

    SceneDepth::SetEndOf3DCallback(&MGS2GasHaze::OnPreMenuRender, SceneDepth::PRIORITY_HAZE_CAPTURE);
    SceneDepth::SetEndOf3DCallback(&MGS2GasHaze::OnFeedbackCapture, SceneDepth::PRIORITY_FEEDBACK_CAPTURE);
    spdlog::info("MGS 2: Gas Haze: initialized.");
}
