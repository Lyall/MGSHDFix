#include "stdafx.h"
#include "common.hpp"
#include "mgs2_gas_haze.hpp"

#include "logging.hpp"
#include "d3d11_api.hpp"
#include "scene_depth.hpp"
#include "mgs2_crossfade.hpp"

#include "gamevars.hpp"

namespace
{
    // NewSmokeBlurEffect (smk_blur.c) - the heli smoke. start_speed 0 -> x64 0/0 = NaN (PS2 clamped it)
    // so nothing draws; we clamp it to 1.

    // The Act sets vertex alpha = pos.vw * XMM7 (loaded once from a shared 64.0 constant); override XMM7
    // right after the load to set opacity. Centre verts get alpha, rim verts stay 0 (soft puff).
    constexpr const char* kAlphaSig         = "F3 0F 10 3D CC C6 18 00";
    constexpr ptrdiff_t   kAlphaHookOffset  = 8;
    constexpr float       kAlphaScale       = 64.0f;    // centre-vert alpha = pos.vw * this (PS2's constant;
                                                        // higher makes the fan edges visible)
    constexpr int         kAlphaFloor       = 0;        // keep 0 so rim verts stay transparent (soft edge)
    constexpr float       kTintScale        = 1.0f;     // raw PS2 colour byte; the shader does GS modulate
    constexpr float       kAlphaGain        = 1.99f;    // PS2 blends at As/128; D3D uses byte/255 -> 255/128
    constexpr float       kAlphaCap         = 1.0f;
    // Overbright per feedback iteration. The GS sampled the buffer it was drawing into, so overlapping
    // puffs compounded the cue's 0x81 modulate several times per frame; we sample one frozen capture, so
    // fold that in here. Tuned against PS2 footage.
    constexpr float       kFeedbackGain      = 1.05f;

    constexpr const char* kDieSig =
        "48 89 5C 24 08 57 48 83 EC 20 48 8B 59 60 48 8B F9 48 85 DB 74 10 48 8B CB";

    // smk_blur Act - fills the prim2 vertices each frame; we read them.
    constexpr const char* kActSig =
        "4C 8B DC 49 89 5B 18 49 89 73 20 55 57 41 54 41 55 41 56 49 8D 6B 98 48 81 EC 40 01 00 00";
    constexpr ptrdiff_t kWork_Clock    = 0x1c8;
    constexpr ptrdiff_t kWork_PrimBase = 0x60;
    constexpr ptrdiff_t kWork_NPrims   = 0x1e8;
    constexpr ptrdiff_t kPrim_PosBuf   = 0xc0;    // FVECTOR* pos[] (world-space vertex positions)
    constexpr ptrdiff_t kPrim_UvrgbBuf = 0xd0;
    constexpr ptrdiff_t kPrim_FlagBuf  = 0x268;
    constexpr ptrdiff_t kFlag_Stride   = 0x40;
    constexpr ptrdiff_t kFlag_Off      = 0x1a;
    constexpr uint16_t  kInvisible0    = 0x1000;
    constexpr int       kNVerts        = 17;

    constexpr float     kUv2Norm   = 1.0f / 4096.0f;
    constexpr float     kWarp      = 0.9751f;          // per-puff lens magnify; PS2 used 0.98, +25% warp strength
    constexpr float     kSoften    = 0.00176f;         // whisper seam-hiding blur (tiny; not the old smear)
    constexpr float     kDepthBias = 0.0002f;          // reversed-Z occlusion epsilon (tuned)

    // smk_blur is a framebuffer-sampling prim2 that MC's D3D11 backend never draws, so we draw it
    // ourselves: read the Act's puff geometry and render it in D3D11 (see DrawInto).
    constexpr float kDrawW = 512.0f;   // smk_blur's virtual screen (UV * this = pixel)
    constexpr float kDrawH = 448.0f;

    SafetyHookInline g_hook{};
    SafetyHookMid    g_alphaHook{};
    SafetyHookInline g_actHook{};
    SafetyHookInline g_dieHook{};

    int g_lastClock   = -0x7fffffff;
    int g_activeCount = 0;

    struct GasVertex { float x, y, u, v, z; uint32_t color; };   // z = projected reversed-Z (occlusion)

    std::mutex            g_vmtx;
    std::vector<GasVertex> g_verts;       // this frame's smoke triangles (filled by Act, drawn end-of-3D)
    FMATRIX g_eyePers = {}; // camera view-projection snapshot (read each frame)

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
        float  feedbackGain; // overbright ramp (>1) compounded through the previous-frame feedback
        float  soften;       // whisper 4-tap radius to hide circle-fan polygon seams (normalised UV)
        float  _pad;
    };
    struct VSIn  { float2 pos:POSITION; float2 uv:TEXCOORD0; float z:TEXCOORD1; float4 col:COLOR0; };
    struct VSOut { float4 pos:SV_Position; float2 uv:TEXCOORD0; float z:TEXCOORD1; float4 col:COLOR0; };
    VSOut VS(VSIn i){
        VSOut o;
        float2 ndc = float2(i.pos.x*invDraw.x*2.0-1.0, 1.0 - i.pos.y*invDraw.y*2.0);
        o.pos = float4(ndc, 0.0, 1.0); o.uv = i.uv; o.z = i.z; o.col = i.col; return o;
    }
    static const float2 K4[4] = { float2(1,1), float2(-1,1), float2(1,-1), float2(-1,-1) };
    float4 PS(VSOut i):SV_Target {
        // Each puff is a lens: i.uv samples the framebuffer at the projected pos scaled by ADDRESS_SCALE
        // (~2% magnify) -> refraction. Whisper 4-tap hides the fan seams.
        float3 c = sceneTex.Sample(sampLin, i.uv).rgb * 0.6;
        [unroll] for (int k = 0; k < 4; k++) c += sceneTex.Sample(sampLin, i.uv + K4[k]*soften).rgb * 0.1;
        c *= i.col.rgb * 1.9921875;   // GS modulate (vertex colour; 0x80 = neutral)
        c *= feedbackGain;            // see kFeedbackGain
        float2 screenUV = i.pos.xy / screenSize;
        float  sceneZ = depthTex.Sample(sampPt, screenUV).r;
        // reversed-Z (far=0, nearer=larger): hide where scene geometry is nearer than this puff
        if (sceneZ > i.z + depthBias) discard;
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
    ULONGLONG g_lastCapMs = 0;      // last time g_capTex was refreshed
    bool g_d3dInit = false, g_d3dFailed = false;

    // (Re)create the feedback capture texture to match the given surface. Returns true if recreated.
    bool EnsureCapTex(ID3D11Device* dev, const D3D11_TEXTURE2D_DESC& bb)
    {
        if (g_capTex && g_capW == bb.Width && g_capH == bb.Height) return false;
        g_capSRV.Reset(); g_capTex.Reset();
        D3D11_TEXTURE2D_DESC td = bb;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE; td.Usage = D3D11_USAGE_DEFAULT;
        td.CPUAccessFlags = 0; td.MiscFlags = 0; td.SampleDesc = { 1, 0 };
        td.MipLevels = 1; td.ArraySize = 1;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, g_capTex.GetAddressOf()))) return false;
        D3D11_SHADER_RESOURCE_VIEW_DESC sv = {}; sv.Format = td.Format;
        sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; sv.Texture2D.MipLevels = 1;
        dev->CreateShaderResourceView(g_capTex.Get(), &sv, g_capSRV.GetAddressOf());
        g_capW = bb.Width; g_capH = bb.Height;
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
        const float* M = g_eyePers.m[0];

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
                if (kf & 0x8000) continue;                       // vertex-kick: no triangle

                GasVertex tri[3];
                bool offscreen = false;
                for (int k = 0; k < 3; ++k)
                {
                    const uint8_t* uv = vbase + (size_t)(i - 2 + k) * 0x10;
                    int ui = *reinterpret_cast<const int16_t*>(uv + 8);
                    int vi = *reinterpret_cast<const int16_t*>(uv + 0xa);
                    if (ui < 0 || ui > 4096 || vi < 0 || vi > 4096) { offscreen = true; break; }
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
                    tri[k].x = (0.5f + (un - 0.5f) / kWarp) * kDrawW;   // polygon = un-magnified UV
                    tri[k].y = (0.5f + (vn - 0.5f) / kWarp) * kDrawH;
                    tri[k].color = uint32_t(tr) | (uint32_t(tg) << 8) | (uint32_t(tb) << 16) | (uint32_t(a) << 24);

                    // Project this puff vertex's world position to the depth buffer's reversed-Z.
                    tri[k].z = 0.0f;
                    if (posBuf)
                    {
                        const float* wp = reinterpret_cast<const float*>(
                            posBuf + (size_t)((j * kNVerts) + (i - 2 + k)) * 0x10);
                        float wx = wp[0], wy = wp[1], wz = wp[2];
                        // Column-major M*v -> GL clip Z [-1,1]; remap to the D3D reversed-Z buffer [0,1].
                        float cz = wx*M[2] + wy*M[6] + wz*M[10] + M[14];
                        float cw = wx*M[3] + wy*M[7] + wz*M[11] + M[15];
                        if (cw != 0.0f) tri[k].z = (cz / cw + 1.0f) * 0.5f;
                    }
                }
                if (offscreen) continue;

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
            memcpy(&g_eyePers, &g_GameVars.DG_Chanl(0)->eye_pers, sizeof(FMATRIX));
            std::lock_guard<std::mutex> lk(g_vmtx);
            g_verts.clear();
        }
        AppendInstance(work);
    }

    // Cutscene effects tick at 30fps on the PS2 but 60fps here, running the puffs and the feedback 2x too
    // fast. Hold the Act every other cutscene frame and only advance the feedback on frames it ran.
    bool g_actUpdated = true;

    void __fastcall Act_Detour(uintptr_t work)
    {
        static int s_tickClock = -0x7fffffff;
        static bool s_skip = false;
        if (const int clock = g_GameVars.DG_Clock(); clock != s_tickClock)   // first Act instance this frame
        {
            s_tickClock = clock;
            s_skip = g_GameVars.InCutscene() ? !s_skip : false;   // gameplay ran 60fps on PS2 too - no skip
            g_actUpdated = !s_skip;
        }
        if (g_actUpdated) g_actHook.fastcall<void>(work);
        if (!work) return;
        __try { BuildSmoke(work); }   // re-read even when held: the frozen prims still draw this frame
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    void* __fastcall NewSmokeBlur_Detour(uintptr_t world, int start_speed, int end_speed, int start_size,
                                         int end_size, int spot_size, int spot_angle, int n_prims,
                                         int interval, int color, int flag)
    {
        if (start_speed == 0) start_speed = 1;                   // avoid the 0/0 NaN
        void* work = g_hook.fastcall<void*>(world, start_speed, end_speed, start_size, end_size, spot_size,
                                            spot_angle, n_prims, interval, color, flag);
        if (work) g_activeCount++;
        return work;
    }

    void __fastcall Die_Detour(uintptr_t actor)
    {
        g_dieHook.fastcall<void>(actor);
        if (--g_activeCount <= 0)
        {
            g_activeCount = 0;
            std::lock_guard<std::mutex> lk(g_vmtx);
            g_verts.clear();
        }
    }

    bool EnsureD3D()
    {
        if (g_d3dInit) return true;
        if (g_d3dFailed) return false;

        ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev) return false;

        if (!g_D3D11Hooks.D3DCompileFunc) { g_d3dFailed = true; spdlog::error("GasHaze: no d3dcompiler_43.dll"); return false; }

        ComPtr<ID3DBlob> vsb, psb, err;
        bool ok = g_D3D11Hooks.D3DCompileFunc && SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                        "VS", "vs_5_0", 0, 0, vsb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (ok) 
            ok = SUCCEEDED(g_D3D11Hooks.D3DCompileFunc(kShader, strlen(kShader), nullptr, nullptr, nullptr,
                        "PS", "ps_5_0", 0, 0, psb.GetAddressOf(), err.ReleaseAndGetAddressOf()));
        if (!ok)
        {
            spdlog::error("GasHaze: shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
            g_d3dFailed = true; 
            return false;
        }

        dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, g_vs.GetAddressOf());
        dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, g_ps.GetAddressOf());

        D3D11_INPUT_ELEMENT_DESC il[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT,      0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        dev->CreateInputLayout(il, 4, vsb->GetBufferPointer(), vsb->GetBufferSize(), g_layout.GetAddressOf());

        D3D11_BUFFER_DESC cbd = {}; cbd.ByteWidth = 32; cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&cbd, nullptr, g_cb.GetAddressOf());

        D3D11_BLEND_DESC bd = {}; auto& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D11_BLEND_SRC_ALPHA;  rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA; rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ZERO;  rt.DestBlendAlpha = D3D11_BLEND_ONE;       rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&bd, g_blend.GetAddressOf());

        D3D11_RASTERIZER_DESC rd = {}; rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE;
        dev->CreateRasterizerState(&rd, g_rs.GetAddressOf());

        D3D11_DEPTH_STENCIL_DESC dsd = {}; dsd.DepthEnable = FALSE; dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dev->CreateDepthStencilState(&dsd, g_dss.GetAddressOf());

        D3D11_SAMPLER_DESC sd = {}; sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP; sd.MaxLOD = D3D11_FLOAT32_MAX;
        dev->CreateSamplerState(&sd, g_sampLin.GetAddressOf());
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        dev->CreateSamplerState(&sd, g_sampPt.GetAddressOf());

        g_d3dInit = true;
        spdlog::info("GasHaze: D3D11 smoke pass initialised.");
        return true;
    }
}

void MGS2GasHaze::DrawInto(ID3D11RenderTargetView* sceneColor, ID3D11ShaderResourceView* depth)
{
    if (!(eGameType & MGS2) || !bEnabled || !sceneColor)
    {
        return;
    }
    if (!EnsureD3D())
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
    if (!dev || !ctx) return;

    // The scene colour RT (3D rendered, no UI yet) is both our draw target and the warp source.
    ComPtr<ID3D11Resource> colorRes;
    sceneColor->GetResource(colorRes.GetAddressOf());
    ComPtr<ID3D11Texture2D> backbuf;
    if (!colorRes || FAILED(colorRes.As(&backbuf)) || !backbuf) return;
    D3D11_TEXTURE2D_DESC bb; backbuf->GetDesc(&bb);
    if (bb.SampleDesc.Count != 1) return;   // MSAA scene RT not supported by the plain copy/SRV

    // Warp source = previous frame's result (PS2 BP_PreviousFrameTexture); fed back for accumulation.
    const bool justCreated = EnsureCapTex(dev, bb);
    if (!g_capTex) return;
    // Reseed from the live scene when the capture is stale (a gap since the smoke last ran, or a camera
    // crossfade) so the puffs never warp an old frame in as a ghost.
    const ULONGLONG nowCapMs = GetTickCount64();
    const bool stale = (g_lastCapMs == 0) || (nowCapMs - g_lastCapMs) > 100;
    if (justCreated || stale || MGS2_Crossfade::IsFading())
    {
        ctx->CopyResource(g_capTex.Get(), backbuf.Get());
        g_lastCapMs = nowCapMs;
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
        ctx->Map(g_cb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
        float cb[8] = { 1.0f / kDrawW, 1.0f / kDrawH, (float)bb.Width, (float)bb.Height,
                        kDepthBias, kFeedbackGain, kSoften, 0.0f };
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
    ID3D11InputLayout* oIL = nullptr; ID3D11Buffer* oVB = nullptr; UINT oStr = 0, oOff = 0;
    ID3D11Buffer* oCB = nullptr; ID3D11ShaderResourceView* oSRV[2] = {}; ID3D11SamplerState* oSamp[2] = {};
    ID3D11Buffer* oVCB = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY oTopo; D3D11_VIEWPORT oVP[1] = {}; UINT oNVP = 1;
    ctx->OMGetRenderTargets(8, oRTV, &oDSV);
    ctx->VSGetConstantBuffers(0, 1, &oVCB);
    ctx->OMGetBlendState(&oBlend, oBF, &oBM);
    ctx->OMGetDepthStencilState(&oDSS, &oSR);
    ctx->RSGetState(&oRS); ctx->RSGetViewports(&oNVP, oVP);
    ctx->VSGetShader(&oVS, nullptr, nullptr); ctx->PSGetShader(&oPS, nullptr, nullptr);
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

    // Restore.
    ctx->OMSetRenderTargets(8, oRTV, oDSV);
    ctx->OMSetBlendState(oBlend, oBF, oBM);
    ctx->OMSetDepthStencilState(oDSS, oSR);
    ctx->RSSetState(oRS); ctx->RSSetViewports(oNVP, oVP);
    ctx->VSSetShader(oVS, nullptr, 0); ctx->PSSetShader(oPS, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &oVCB);
    ctx->IASetInputLayout(oIL); ctx->IASetVertexBuffers(0, 1, &oVB, &oStr, &oOff);
    ctx->IASetPrimitiveTopology(oTopo);
    ctx->PSSetConstantBuffers(0, 1, &oCB); ctx->PSSetShaderResources(0, 2, oSRV); ctx->PSSetSamplers(0, 2, oSamp);
    for (auto* r : oRTV) if (r) r->Release();
    if (oDSV) oDSV->Release(); if (oBlend) oBlend->Release(); if (oDSS) oDSS->Release();
    if (oRS) oRS->Release(); if (oVS) oVS->Release(); if (oPS) oPS->Release(); if (oIL) oIL->Release();
    if (oVB) oVB->Release(); if (oCB) oCB->Release(); if (oVCB) oVCB->Release();
    for (auto* s : oSRV) if (s) s->Release();
    for (auto* s : oSamp) if (s) s->Release();

    // Persist this frame (pre-UI, so subtitles never feed back) as the next iteration's warp source.
    if (g_actUpdated)
    {
        ctx->CopyResource(g_capTex.Get(), backbuf.Get());
        g_lastCapMs = GetTickCount64();
    }
}

void MGS2GasHaze::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Gas Haze - disabled by config; haze fix disabled.");
        return;
    }

    if (g_GameVars.DG_Chanl(0) == nullptr)
    {
        spdlog::info("MGS 2: Gas Haze - DG_Chanl gamevars patternscan failure; haze fix disabled.");
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

    // Draw the smoke at the end of the 3D pass (before UI/titles/credits) instead of at Present.
    SceneDepth::SetEndOf3DCallback(&MGS2GasHaze::DrawInto, SceneDepth::PRIORITY_HAZE);
    spdlog::info("MGS 2: Gas Haze - haze fix initialized.");
}
