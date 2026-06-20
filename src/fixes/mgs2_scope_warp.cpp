#include "stdafx.h"
#include "common.hpp"
#include "mgs2_scope_warp.hpp"

#include "helper.hpp"
#include "logging.hpp"
#include "mgs2_autopacket.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

// scr_sight builds the scope lens on the PS2 crack_pack path (dmapack->packet[]), which the D3D
// backend never draws, so the fish-eye is invisible. scr_goggles uses the autopacket path and works.
// We rebuild the lens as an autopacket like goggles, with the original WORPCOFF warp + front-lens morph.

namespace
{
    using namespace MGS2Autopacket;

    constexpr ptrdiff_t kWork_Def      = 0x70;   // psg_lenz_bd (body)
    constexpr ptrdiff_t kWork_DefK     = 0x78;   // psg_lenz_fr (front lens)
    constexpr ptrdiff_t kWork_Dmapack  = 0xb0;
    constexpr ptrdiff_t kWork_AnglePtr = 0xb8;

    constexpr ptrdiff_t kDef_NPacks    = 0x44;
    constexpr ptrdiff_t kDef_PacksPtr  = 0x70;
    constexpr ptrdiff_t kPack_Stride   = 0x60;
    constexpr ptrdiff_t kPack_NVerts   = 0x04;
    constexpr ptrdiff_t kPack_Verts    = 0x20;   // SVECTOR* (vx,vy,vz,pad)
    constexpr ptrdiff_t kPack_Norms    = 0x28;   // norm[3] bit 0x8000 = strip restart

    constexpr float    kWorpcoff   = 20000.0f;
    constexpr float    kHalfW      = 256.0f;
    constexpr float    kHalfH      = 224.0f;
    constexpr float    kMorphFromZ = -119.0f;
    constexpr int      kBunkatsu   = 2;
    constexpr uint32_t kLensColor  = 0x80808080u;

    SafetyHookInline g_getResHook{};
    InitMdlDrawFn    g_initMdlDraw = nullptr;

    // sight is a singleton, rebuilt on each scope-open; far larger than the two small lens models need.
    alignas(16) uint8_t g_autopacket[0x40000];

    uint8_t* EmitLens(uint8_t* p, uint8_t* end, uintptr_t def, bool isMorph, float angle)
    {
        const int       nPacks = Memory::ReadField<int>(def, kDef_NPacks);
        const uintptr_t packs  = Memory::ReadField<uintptr_t>(def, kDef_PacksPtr);
        if (nPacks <= 0 || !packs) return p;

        const float k = isMorph ? (60.0f - angle) : 0.0f;   // morph: 52 - (angle - 8)

        for (int kk = 0; kk < kBunkatsu; ++kk)
        {
            for (int i = 0; i < nPacks; ++i)
            {
                const uintptr_t pack   = packs + (uintptr_t)i * kPack_Stride;
                const uint32_t  nVerts = Memory::ReadField<uint32_t>(pack, kPack_NVerts);
                const int16_t*  verts  = Memory::ReadField<int16_t*>(pack, kPack_Verts);
                const int16_t*  norms  = Memory::ReadField<int16_t*>(pack, kPack_Norms);
                if (!nVerts || !verts) continue;

                uint32_t* countField = nullptr;
                p = WriteStripHeader(p, &countField);
                uint32_t count = 0;
                uint8_t* prevVert = nullptr;
                int lastKick = 0x8000;

                for (uint32_t j = 0; j < nVerts; ++j)
                {
                    if (p + 0x40 > end) { *countField = count; return p; }

                    const int vx = verts[j * 4 + 0];
                    const int vy = verts[j * 4 + 1];
                    const int vz = verts[j * 4 + 2];
                    const int kick = norms ? (norms[j * 4 + 3] & 0x8000) : 0;

                    const float svx = (kk & 1) ? (float)(-vx) : (float)vx;
                    const float svy = (float)vy;

                    float z = isMorph ? (kMorphFromZ + ((float)vz + 119.0f) * k) : (float)vz;
                    float temp = powf(2.0f, z / kWorpcoff);

                    const float sx = kHalfW + svx;
                    const float sy = kHalfH + svy;
                    const float un = (kHalfW + svx * temp) / kDrawW;
                    const float vn = (kHalfH + svy * temp) / kDrawH;

                    if (kick && !lastKick && prevVert)   // degenerate bridge to restart the strip
                    {
                        std::memcpy(p, prevVert, kVertexSize); p += kVertexSize; ++count;
                        p = WriteVertex(p, sx, sy, un, vn, kLensColor);           ++count;
                    }
                    lastKick = kick;

                    prevVert = p;
                    p = WriteVertex(p, sx, sy, un, vn, kLensColor); ++count;
                }
                *countField = count;
            }
        }
        return p;
    }

    // SEH-guarded (raw game-memory walk, no C++ unwinding here) so stale geometry leaves the scope as-is.
    void BuildScopeAutopacket(uintptr_t work)
    {
        __try
        {
            const uintptr_t dmapack = Memory::ReadField<uintptr_t>(work, kWork_Dmapack);
            const uintptr_t def     = Memory::ReadField<uintptr_t>(work, kWork_Def);
            const uintptr_t defK    = Memory::ReadField<uintptr_t>(work, kWork_DefK);
            if (!dmapack || !def || !defK) return;

            const float* anglePtr = Memory::ReadField<float*>(work, kWork_AnglePtr);
            const float  angle    = anglePtr ? *anglePtr : 0.0f;

            uint8_t* const end = g_autopacket + sizeof(g_autopacket) - 0x40;
            uint8_t* p = g_autopacket;

            p = WriteTrBuffer(p);
            p = WriteDrawOffset(p);
            p = reinterpret_cast<uint8_t*>(g_initMdlDraw(p, 0x3000eULL, 0x800000002aULL, 0x114ULL));
            p = EmitLens(p, end, def,  false, 0.0f);
            p = EmitLens(p, end, defK, true,  angle);
            WriteEnd(p);

            Commit(dmapack, g_autopacket);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    long long __fastcall GetResources_Detour(uintptr_t work, int mode, int camera)
    {
        const long long ret = g_getResHook.fastcall<long long>(work, mode, camera);
        if (ret == 0 && g_initMdlDraw && work)
        {
            BuildScopeAutopacket(work);
        }
        return ret;
    }
}

void MGS2ScopeWarp::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    spdlog::info("MGS 2: Scope Warp fix: Initializing...");

    uint8_t* mdlDraw = Memory::PatternScan(baseModule, kInitMdlDrawSig, "MGS 2: Scope Warp - InitMdlDraw");
    if (!mdlDraw)
    {
        return;
    }
    g_initMdlDraw = reinterpret_cast<InitMdlDrawFn>(mdlDraw);

    if (uint8_t* address = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 48 8B D9", "MGS 2: Scope Warp - shibata\\effect\\scr_sight.c -> NewScrSightMorph() -> GetResources()"))
    {
        g_getResHook = safetyhook::create_inline(address, reinterpret_cast<void*>(GetResources_Detour));
        LOG_HOOK(g_getResHook, "MGS 2: Scope Warp - GetResources");
    }
}
