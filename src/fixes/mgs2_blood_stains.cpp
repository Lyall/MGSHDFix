#include "stdafx.h"
#include "common.hpp"
#include "mgs2_blood_stains.hpp"

#include "logging.hpp"
#include "d3d11_api.hpp"


namespace
{
    constexpr ptrdiff_t kWork_Objs       = 0x60;
    constexpr ptrdiff_t kObjs_NModels    = 0x64;
    constexpr ptrdiff_t kDgObj_Stride    = 0x180;
    constexpr ptrdiff_t kDgObj_NPacks    = 0x1dc;
    constexpr ptrdiff_t kDgObj_Rgbs      = 0x220;
    constexpr ptrdiff_t kDgObj_Packets   = 0x228;
    constexpr ptrdiff_t kDgObj_BPModel   = 0x280;
    constexpr ptrdiff_t kPacket_Stride   = 0x38;
    constexpr ptrdiff_t kPacket_NVerts   = 0x14;
    constexpr ptrdiff_t kPacket_RgbsOff  = 0x1b;
    constexpr ptrdiff_t kCMesh_Buffers   = 0x08;
    constexpr ptrdiff_t kCMB_VertexCount = 0x1e0;
    constexpr ptrdiff_t kCMB_OrigIndices = 0x1e8;
    constexpr ptrdiff_t kParam_Mesh      = 0x188;
    constexpr ptrdiff_t kLP_DgObjs       = 0x08;

    SafetyHookMid    g_oozeAddHook{};
    SafetyHookMid    g_localParamHook{};
    SafetyHookMid    g_changeObjsHook{};
    SafetyHookInline g_dispatchHook{};
    SafetyHookInline g_dispatch2Hook{};
    SafetyHookInline g_drawIndexedHook{};

    std::mutex                     g_regMutex;
    std::unordered_set<uintptr_t>  g_bloodObjs;

    thread_local bool      g_active   = false;
    thread_local bool      g_inMyDraw = false;
    thread_local uintptr_t g_curObjs  = 0;
    thread_local uintptr_t g_curMesh  = 0;
    thread_local uintptr_t g_lpObjs   = 0;

    std::once_flag           g_initOnce;
    bool                     g_ready    = false;
    ID3D11VertexShader*      g_bloodVS  = nullptr;   // rigid: float4 @ slot0
    ID3D11VertexShader*      g_bloodVS_S = nullptr;  // skinned: int16 @ slot2
    ID3D11PixelShader*       g_bloodPS  = nullptr;
    ID3D11InputLayout*       g_layoutF  = nullptr;
    ID3D11InputLayout*       g_layoutS  = nullptr;
    ID3D11BlendState*        g_blendMul = nullptr;
    ID3D11DepthStencilState* g_depthState = nullptr;
    SafetyHookInline         g_createILHook{};

    constexpr UINT kBloodSlot = 7;                   // free slot; game uses 0-2

    // layout -> 0 = float4 @ slot0, 1 = int16 @ slot2
    std::mutex                       g_ilMutex;
    std::unordered_map<void*, int>   g_ilClass;

    struct MeshBlood { ID3D11Buffer* vb = nullptr; UINT verts = 0; uint64_t lastTick = 0; };
    std::unordered_map<uintptr_t, MeshBlood> g_meshBlood;
    std::vector<float>                       g_scratch;
    std::vector<float>                       g_packed;

    inline bool HeapPtr(uintptr_t v)
    {
        // accept any aligned canonical user pointer (Proton/Wine allocates below the old 1 TB bound)
        return v >= 0x10000ull && v < 0x800000000000ull && (v & 7) == 0;
    }

    void __stdcall DrawIndexed_Detour(ID3D11DeviceContext* ctx, UINT indexCount, UINT startIndex, INT baseVertex);
    HRESULT __stdcall CreateInputLayout_Detour(ID3D11Device* dev, const D3D11_INPUT_ELEMENT_DESC* descs, UINT num,
                                               const void* bc, SIZE_T bcLen, ID3D11InputLayout** out);

    // rigid: POSITION float4 @ slot0
    const char* kVS =
        "cbuffer Globals : register(b0) {\n"
        "  row_major float4x4 gVS_Mat0 : packoffset(c16);\n"
        "  row_major float4x4 gVS_Pers : packoffset(c20);\n"
        "  uint gVS_IsShort : packoffset(c33);\n"
        "};\n"
        "struct VIn  { float4 pos:POSITION; float4 blood:TEXCOORD7; };\n"
        "struct VOut { float4 pos:SV_Position; float4 blood:TEXCOORD0; };\n"
        "VOut main(VIn i){\n"
        "  VOut o;\n"
        "  float3 p = (gVS_IsShort == 1u) ? (float3)asint(i.pos.xyz) : i.pos.xyz;\n"
        "  float4 wp = mul(gVS_Mat0, float4(p, 1.0));\n"
        "  o.pos = mul(gVS_Pers, wp);\n"
        "  o.blood = i.blood;\n"
        "  return o;\n"
        "}\n";
    // skinned: POSITION int16 @ slot2, xyz fixed-point, w = bone weight (/4096)
    const char* kVS_S =
        "cbuffer Globals : register(b0) {\n"
        "  row_major float4x4 gVS_Mat0 : packoffset(c16);\n"
        "  row_major float4x4 gVS_Pers : packoffset(c20);\n"
        "  row_major float4x4 gVS_Corr : packoffset(c24);\n"
        "};\n"
        "struct VInS { int4 pos:POSITION; float4 blood:TEXCOORD7; };\n"
        "struct VOut { float4 pos:SV_Position; float4 blood:TEXCOORD0; };\n"
        "VOut main(VInS i){\n"
        "  VOut o;\n"
        "  float4 p = float4((float3)i.pos.xyz, 1.0);\n"
        "  float  w = (float)i.pos.w * (1.0/4096.0);\n"
        "  float4 wp = lerp(mul(gVS_Corr, p), mul(gVS_Mat0, p), w);\n"
        "  o.pos = mul(gVS_Pers, wp);\n"
        "  o.blood = i.blood;\n"
        "  return o;\n"
        "}\n";
    const char* kPS =
        "struct VOut { float4 pos:SV_Position; float4 blood:TEXCOORD0; };\n"
        "float4 main(VOut i):SV_Target {\n"
        "  const float kStrength = 0.6;\n"
        "  float3 c = lerp(float3(1,1,1), saturate(i.blood.rgb), kStrength);\n"
        "  return float4(c, 1.0);\n"
        "}\n";

    void EnsureD3D()
    {
        std::call_once(g_initOnce, []
        {
            ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
            ID3D11DeviceContext* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
            if (!dev || !ctx) { spdlog::error("MGS 2: Blood Stains: D3D device not ready."); return; }

            if (!g_D3D11Hooks.D3DCompileFunc) { spdlog::error("MGS 2: Blood Stains: D3DCompile not found."); return; }

            ID3DBlob *vsb = nullptr, *vsbS = nullptr, *psb = nullptr, *err = nullptr;
            if (FAILED(g_D3D11Hooks.D3DCompileFunc(kVS, strlen(kVS), "blood_vs", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsb, &err)))
            {
                spdlog::error("MGS 2: Blood Stains: vertex shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
                if (err) err->Release();
                return;
            }
            if (err) { err->Release(); err = nullptr; }
            if (FAILED(g_D3D11Hooks.D3DCompileFunc(kVS_S, strlen(kVS_S), "blood_vs_s", nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsbS, &err)))
            {
                spdlog::error("MGS 2: Blood Stains: skinned vertex shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
                if (err) err->Release();
                vsb->Release();
                return;
            }
            if (err) { err->Release(); err = nullptr; }
            if (FAILED(g_D3D11Hooks.D3DCompileFunc(kPS, strlen(kPS), "blood_ps", nullptr, nullptr, "main", "ps_4_0", 0, 0, &psb, &err)))
            {
                spdlog::error("MGS 2: Blood Stains: pixel shader compile failed: {}", err ? (const char*)err->GetBufferPointer() : "?");
                if (err) err->Release();
                vsb->Release(); vsbS->Release();
                return;
            }
            if (err) { err->Release(); err = nullptr; }

            dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_bloodVS);
            dev->CreateVertexShader(vsbS->GetBufferPointer(), vsbS->GetBufferSize(), nullptr, &g_bloodVS_S);
            dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_bloodPS);

            const D3D11_INPUT_ELEMENT_DESC ilF[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,         0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 7, DXGI_FORMAT_R32G32B32A32_FLOAT, kBloodSlot, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            dev->CreateInputLayout(ilF, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_layoutF);

            const D3D11_INPUT_ELEMENT_DESC ilS[] = {
                { "POSITION", 0, DXGI_FORMAT_R16G16B16A16_SINT,  2,         0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 7, DXGI_FORMAT_R32G32B32A32_FLOAT, kBloodSlot, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            dev->CreateInputLayout(ilS, 2, vsbS->GetBufferPointer(), vsbS->GetBufferSize(), &g_layoutS);
            vsb->Release();
            vsbS->Release();
            psb->Release();

            D3D11_BLEND_DESC bd = {};
            bd.RenderTarget[0].BlendEnable = TRUE;
            bd.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;
            bd.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
            bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            dev->CreateBlendState(&bd, &g_blendMul);

            D3D11_DEPTH_STENCIL_DESC dd = {};
            dd.DepthEnable = TRUE;
            dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            dd.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
            dev->CreateDepthStencilState(&dd, &g_depthState);

            void** vtable = *reinterpret_cast<void***>(ctx);
            g_drawIndexedHook = safetyhook::create_inline(vtable[12], reinterpret_cast<void*>(DrawIndexed_Detour));

            void** dvt = *reinterpret_cast<void***>(dev);
            g_createILHook = safetyhook::create_inline(dvt[11], reinterpret_cast<void*>(CreateInputLayout_Detour));

            g_ready = g_bloodVS && g_bloodVS_S && g_bloodPS && g_layoutF && g_layoutS && g_blendMul && g_depthState && g_drawIndexedHook;
            if (g_ready)
                spdlog::info("MGS 2: Blood Stains: render resources initialised.");
            else
                spdlog::error("MGS 2: Blood Stains: failed to initialise render resources.");
        });
    }

    constexpr uint32_t kMaxVerts = 65536;

    int BuildBloodColours(uintptr_t objs, uintptr_t mesh) noexcept
    {
        __try
        {
            const uintptr_t cmb = *reinterpret_cast<uintptr_t*>(mesh + kCMesh_Buffers);
            if (!HeapPtr(cmb)) return 0;
            const int vc = *reinterpret_cast<int*>(cmb + kCMB_VertexCount);
            const uint32_t* origIdx = *reinterpret_cast<uint32_t**>(cmb + kCMB_OrigIndices);
            const int nm = *reinterpret_cast<int16_t*>(objs + kObjs_NModels);
            if (vc <= 0 || (uint32_t)vc > kMaxVerts || !HeapPtr((uintptr_t)origIdx) || nm <= 0 || nm > 256) return 0;

            uint32_t packedVerts = 0;
            for (int u = 0; u < nm; ++u)
            {
                const uintptr_t packets = *reinterpret_cast<uintptr_t*>(objs + kDgObj_Packets + (ptrdiff_t)u * kDgObj_Stride);
                const int nPacks = *reinterpret_cast<int16_t*>(objs + kDgObj_NPacks + (ptrdiff_t)u * kDgObj_Stride);
                if (!HeapPtr(packets) || nPacks <= 0 || nPacks > 4096) continue;
                for (int p = 0; p < nPacks; ++p)
                    packedVerts += *reinterpret_cast<uint8_t*>(packets + (ptrdiff_t)p * kPacket_Stride + kPacket_NVerts);
            }
            uint32_t maxIdx = 0;
            for (int i = 0; i < vc; ++i) { const uint32_t v = origIdx[i]; if (v > maxIdx) maxIdx = v; }
            const uint32_t packedCount = (packedVerts > maxIdx + 1) ? packedVerts : (maxIdx + 1);
            if (packedCount == 0 || packedCount > kMaxVerts) return 0;

            float* packed = g_packed.data();
            for (uint32_t i = 0; i < packedCount * 4; ++i) packed[i] = 1.0f;

            uint32_t off = 0;
            for (int u = 0; u < nm; ++u)
            {
                const int16_t* rgbs   = *reinterpret_cast<int16_t**>(objs + kDgObj_Rgbs + (ptrdiff_t)u * kDgObj_Stride);
                const uintptr_t packets = *reinterpret_cast<uintptr_t*>(objs + kDgObj_Packets + (ptrdiff_t)u * kDgObj_Stride);
                const int nPacks = *reinterpret_cast<int16_t*>(objs + kDgObj_NPacks + (ptrdiff_t)u * kDgObj_Stride);
                if (!HeapPtr(packets) || nPacks <= 0 || nPacks > 4096) continue;
                const int16_t* pc = HeapPtr((uintptr_t)rgbs) ? rgbs : nullptr;
                for (int q = 0; q < nPacks; ++q)
                {
                    const uintptr_t packet = packets + (ptrdiff_t)q * kPacket_Stride;
                    const uint8_t nv = *reinterpret_cast<uint8_t*>(packet + kPacket_NVerts);
                    const uint8_t ro = *reinterpret_cast<uint8_t*>(packet + kPacket_RgbsOff);
                    if (pc)
                    {
                        for (uint32_t i = 0; i < nv && off + i < packedCount; ++i)
                        {
                            const int16_t* sv = pc + (size_t)i * 4;
                            float* d = &packed[(off + i) * 4];
                            d[0] = sv[0] / 4096.0f; d[1] = sv[1] / 4096.0f; d[2] = sv[2] / 4096.0f; d[3] = sv[3] / 4096.0f;
                        }
                        pc += (size_t)ro * 2 * 4;
                    }
                    off += nv;
                }
            }

            float* sc = g_scratch.data();
            for (int i = 0; i < vc; ++i)
            {
                const uint32_t o = origIdx[i];
                float* d = &sc[(size_t)i * 4];
                if (o < packedCount) { const float* s = &packed[o * 4]; d[0]=s[0]; d[1]=s[1]; d[2]=s[2]; d[3]=s[3]; }
                else { d[0]=d[1]=d[2]=d[3]=1.0f; }
            }
            return vc;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    bool BuildBlood(ID3D11DeviceContext* ctx, uintptr_t objs, uintptr_t mesh, MeshBlood& out)
    {
        if (g_packed.size()  < (size_t)kMaxVerts * 4) g_packed.resize((size_t)kMaxVerts * 4);
        if (g_scratch.size() < (size_t)kMaxVerts * 4) g_scratch.resize((size_t)kMaxVerts * 4);

        const int vertexCount = BuildBloodColours(objs, mesh);
        if (vertexCount <= 0) return false;

        if (!out.vb || out.verts < (UINT)vertexCount)
        {
            if (out.vb) { out.vb->Release(); out.vb = nullptr; }
            D3D11_BUFFER_DESC bd = {};
            bd.ByteWidth = (UINT)vertexCount * 16;
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(g_D3D11Hooks.d3dDevice->CreateBuffer(&bd, nullptr, &out.vb))) return false;
            out.verts = (UINT)vertexCount;
        }
        D3D11_MAPPED_SUBRESOURCE ms = {};
        if (FAILED(ctx->Map(out.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return false;
        memcpy(ms.pData, g_scratch.data(), (size_t)vertexCount * 16);
        ctx->Unmap(out.vb, 0);
        return true;
    }

    void SecondPassSEH(ID3D11DeviceContext* ctx, ID3D11Buffer* bloodVB, UINT ic, UINT si, INT bv, bool isSint) noexcept
    {
        __try
        {
            ID3D11VertexShader*      oVS = nullptr; ID3D11PixelShader* oPS = nullptr;
            ID3D11InputLayout*       oIL = nullptr;
            ID3D11BlendState*        oBlend = nullptr; ID3D11DepthStencilState* oDepth = nullptr;
            ID3D11Buffer*            oVB = nullptr; UINT oStr = 0, oOf = 0;
            float oBF[4] = {}; UINT oMask = 0, oStencil = 0;
            ctx->VSGetShader(&oVS, nullptr, nullptr);
            ctx->PSGetShader(&oPS, nullptr, nullptr);
            ctx->IAGetInputLayout(&oIL);
            ctx->OMGetBlendState(&oBlend, oBF, &oMask);
            ctx->OMGetDepthStencilState(&oDepth, &oStencil);
            ctx->IAGetVertexBuffers(kBloodSlot, 1, &oVB, &oStr, &oOf);

            UINT strideB = 16, offB = 0;
            ctx->IASetInputLayout(isSint ? g_layoutS : g_layoutF);
            ctx->IASetVertexBuffers(kBloodSlot, 1, &bloodVB, &strideB, &offB);
            ctx->VSSetShader(isSint ? g_bloodVS_S : g_bloodVS, nullptr, 0);
            ctx->PSSetShader(g_bloodPS, nullptr, 0);
            const float bf[4] = { 0, 0, 0, 0 };
            ctx->OMSetBlendState(g_blendMul, bf, 0xFFFFFFFF);
            ctx->OMSetDepthStencilState(g_depthState, 0);
            g_drawIndexedHook.stdcall<void>(ctx, ic, si, bv);

            ctx->VSSetShader(oVS, nullptr, 0);
            ctx->PSSetShader(oPS, nullptr, 0);
            ctx->IASetInputLayout(oIL);
            ctx->OMSetBlendState(oBlend, oBF, oMask);
            ctx->OMSetDepthStencilState(oDepth, oStencil);
            ctx->IASetVertexBuffers(kBloodSlot, 1, &oVB, &oStr, &oOf);
            if (oVB) oVB->Release();
            if (oVS) oVS->Release(); if (oPS) oPS->Release(); if (oIL) oIL->Release();
            if (oBlend) oBlend->Release(); if (oDepth) oDepth->Release();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // 0 = float4 @ slot0, 1 = int16 @ slot2, -1 = unknown
    int ClassifyPosLayout(const D3D11_INPUT_ELEMENT_DESC* descs, UINT num) noexcept
    {
        __try
        {
            for (UINT i = 0; i < num; ++i)
            {
                if (descs[i].SemanticName && descs[i].SemanticIndex == 0 &&
                    strcmp(descs[i].SemanticName, "POSITION") == 0)
                    return (descs[i].Format == DXGI_FORMAT_R16G16B16A16_SINT) ? 1 : 0;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return -1;
    }

    HRESULT __stdcall CreateInputLayout_Detour(ID3D11Device* dev, const D3D11_INPUT_ELEMENT_DESC* descs, UINT num,
                                               const void* bc, SIZE_T bcLen, ID3D11InputLayout** out)
    {
        HRESULT hr = g_createILHook.stdcall<HRESULT>(dev, descs, num, bc, bcLen, out);
        if (SUCCEEDED(hr) && out && *out && descs)
        {
            const int cls = ClassifyPosLayout(descs, num);
            if (cls >= 0)
            {
                std::lock_guard<std::mutex> lk(g_ilMutex);
                g_ilClass[*out] = cls;
            }
        }
        return hr;
    }

    void __stdcall DrawIndexed_Detour(ID3D11DeviceContext* ctx, UINT indexCount, UINT startIndex, INT baseVertex)
    {
        g_drawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);

        if (g_inMyDraw || !g_active || !g_ready || !ctx || !g_curObjs || !g_curMesh) return;

        MeshBlood& mb = g_meshBlood[g_curObjs];
        const uint64_t now = GetTickCount64();
        if (!mb.vb || now - mb.lastTick > 8)
        {
            BuildBlood(ctx, g_curObjs, g_curMesh, mb);
            mb.lastTick = now;
        }
        if (!mb.vb) return;

        bool isSint = false;
        ID3D11InputLayout* il = nullptr;
        ctx->IAGetInputLayout(&il);
        if (il)
        {
            std::lock_guard<std::mutex> lk(g_ilMutex);
            auto it = g_ilClass.find(il);
            if (it != g_ilClass.end()) isSint = (it->second == 1);
        }
        if (il) il->Release();

        g_inMyDraw = true;
        SecondPassSEH(ctx, mb.vb, indexCount, startIndex, baseVertex, isSint);
        g_inMyDraw = false;
    }

    bool IsBloodObjs(uintptr_t objs)
    {
        if (!objs) return false;
        std::lock_guard<std::mutex> lk(g_regMutex);
        return g_bloodObjs.find(objs) != g_bloodObjs.end();
    }

    void LocalParam_Hook(SafetyHookContext& ctx)
    {
        g_lpObjs = Memory::ReadField<uintptr_t>(ctx.rcx, kLP_DgObjs, 0);
    }

    void EnterDispatch(uintptr_t param1)
    {
        EnsureD3D();
        g_active = false; g_curObjs = 0; g_curMesh = 0;
        if (!g_ready) return;
        const uintptr_t objs = g_lpObjs;
        if (!IsBloodObjs(objs)) return;
        const uintptr_t mesh = Memory::ReadField<uintptr_t>(param1, kParam_Mesh, 0);
        if (!HeapPtr(mesh)) return;
        g_active = true; g_curObjs = objs; g_curMesh = mesh;
    }

    void __fastcall Dispatch_Detour(uintptr_t param1)
    {
        EnterDispatch(param1);
        g_dispatchHook.fastcall<void>(param1);
        g_active = false; g_curObjs = 0; g_curMesh = 0;
    }
    void __fastcall Dispatch2_Detour(uintptr_t param1)
    {
        EnterDispatch(param1);
        g_dispatch2Hook.fastcall<void>(param1);
        g_active = false; g_curObjs = 0; g_curMesh = 0;
    }

    void OozeAdd_Hook(SafetyHookContext& ctx)
    {
        const uintptr_t objs = Memory::ReadField<uintptr_t>(ctx.rcx, kWork_Objs, 0);
        if (!HeapPtr(objs)) return;
        std::lock_guard<std::mutex> lk(g_regMutex);
        g_bloodObjs.insert(objs);
    }

    // follow the blood onto the new objs when a body ragdolls
    void ChangeObjs_Hook(SafetyHookContext& ctx)
    {
        const uintptr_t oldObjs = Memory::ReadField<uintptr_t>(ctx.rcx, kWork_Objs, 0);
        const uintptr_t newObjs = ctx.rdx;
        if (!HeapPtr(newObjs)) return;
        std::lock_guard<std::mutex> lk(g_regMutex);
        g_bloodObjs.erase(oldObjs);
        g_bloodObjs.insert(newObjs);
    }
}

void MGS2BloodStains::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    spdlog::info("MGS 2 - Blood stain fixes: Initializing...");

    if (uint8_t* address = Memory::PatternScan(baseModule, "4C 8B DC 55 57 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? 00 00 45 0F 29 BB ?? ?? FF FF", "MGS 2: Blood Stains - OozeAdd"))
    {
        g_oozeAddHook = safetyhook::create_mid(address, OozeAdd_Hook);
        LOG_HOOK(g_oozeAddHook, "MGS 2: Blood Stains - OozeAdd");
    }
    if (uint8_t* address = Memory::PatternScan(baseModule, "40 53 48 83 EC ?? 33 D2 4C 8D 0D ?? ?? ?? ?? 48 8B D9 48 8B 0D ?? ?? ?? ?? 44 8D 42 01 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 4C 8D 4B 10 BA 01 00 00 00 44 8B C2 E8 ?? ?? ?? ?? 48 C7 05", "MGS 2: Blood Stains - LocalParam"))
    {
        g_localParamHook = safetyhook::create_mid(address, LocalParam_Hook);
        LOG_HOOK(g_localParamHook, "MGS 2: Blood Stains - LocalParam");
    }
    if (uint8_t* address = Memory::PatternScan(baseModule, "48 83 EC ?? 44 0F BF 52 64 4C 8B D9 48 8B 41 60 45 85 D2 7E ?? 48 89 1C 24 4C 8D 80 28 02 00 00 33 DB 4C 8D 8A 28 02 00", "MGS 2: Blood Stains - ChangeObjs"))
    {
        g_changeObjsHook = safetyhook::create_mid(address, ChangeObjs_Hook);
        LOG_HOOK(g_changeObjsHook, "MGS 2: Blood Stains - ChangeObjs");
    }
    if (uint8_t* address = Memory::PatternScan(baseModule, "4C 8B DC 55 41 57 49 8D AB ?? ?? FF FF 48 81 EC ?? ?? 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? 00 4C 8B F9", "MGS 2: Blood Stains - Dispatch"))
    {
        g_dispatchHook = safetyhook::create_inline(address, reinterpret_cast<void*>(Dispatch_Detour));
        LOG_HOOK(g_dispatchHook, "MGS 2: Blood Stains - Dispatch");
    }
    if (uint8_t* address = Memory::PatternScan(baseModule, "4C 8B DC 55 53 49 8D AB ?? ?? FF FF 48 81 EC ?? ?? 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 ?? ?? ?? ?? 83 3D ?? ?? ?? ?? 00 48 8B D9", "MGS 2: Blood Stains - Dispatch2"))
    {
        g_dispatch2Hook = safetyhook::create_inline(address, reinterpret_cast<void*>(Dispatch2_Detour));
        LOG_HOOK(g_dispatch2Hook, "MGS 2: Blood Stains - Dispatch2");
    }
}
