#include "stdafx.h"
#include "common.hpp"
#include "mgs2_lens_droplets.hpp"

#include "logging.hpp"
#include "mgs2_autopacket.hpp"

#include "gamevars.hpp"

namespace
{
    using namespace MGS2Autopacket;


    constexpr ptrdiff_t kWork_DrowVerts  = 0x70;
    constexpr ptrdiff_t kWork_DrowMVerts = 0x80;
    constexpr ptrdiff_t kWork_Cv2        = 0x1090;
    constexpr ptrdiff_t kWork_Dmapack    = 0x1098;
    constexpr ptrdiff_t kWork_ActMDrops  = 0x10b0;
    constexpr ptrdiff_t kCv2_NVertsIndex = 0x14;

    constexpr int       kMainDrops  = 64;
    constexpr ptrdiff_t kVertStride = 0x30;
    constexpr ptrdiff_t kVertHeader = 0x10;
    constexpr int       kPosOffX    = 1792;
    constexpr int       kPosOffY    = 1824;

    constexpr uint64_t kMdlTest  = 0x3000eULL;
    constexpr uint64_t kMdlAlpha = 0x8000000044ULL;
    constexpr uint64_t kMdlPrim  = 0x15cULL;

    SafetyHookInline g_actHook{};
    InitMdlDrawFn    g_initMdlDraw = nullptr;

    alignas(16) uint8_t g_autopacket[0x80000];

    uint8_t* TranscodeDrops(uint8_t* p, uint8_t* end, uintptr_t buffer, int nDrops, int nVerts)
    {
        if (!buffer || nDrops <= 0 || nVerts <= 0) return p;
        const uint8_t* v = reinterpret_cast<const uint8_t*>(buffer + kVertHeader);

        for (int d = 0; d < nDrops; ++d)
        {
            if (p + 0x20 + (ptrdiff_t)nVerts * kVertexSize > end) return p;

            uint32_t* countField = nullptr;
            p = WriteStripHeader(p, &countField);

            for (int j = 0; j < nVerts; ++j, v += kVertStride)
            {
                const uint64_t rgbaq = *reinterpret_cast<const uint64_t*>(v + 0x00);
                const uint64_t uv    = *reinterpret_cast<const uint64_t*>(v + 0x10);
                const uint64_t xyz   = *reinterpret_cast<const uint64_t*>(v + 0x20);

                const int sx = (int)(uint16_t)(xyz & 0xFFFF) >> 4;
                const int sy = (int)(uint16_t)((xyz >> 16) & 0xFFFF) >> 4;
                const int up = (int)(uint16_t)(uv & 0xFFFF) >> 4;
                const int vp = (int)(uint16_t)((uv >> 16) & 0xFFFF) >> 4;
                const uint32_t color = 0x00808080u | ((uint32_t)((rgbaq >> 24) & 0xFF) << 24);

                p = WriteVertex(p, (float)(sx - kPosOffX), (float)(sy - kPosOffY),
                                (float)up / kDrawW, (float)vp / kDrawH, color);
            }
            *countField = (uint32_t)nVerts;
        }
        return p;
    }

    void BuildDropsAutopacket(uintptr_t work)
    {
        __try
        {
            const uintptr_t dmapack = Memory::ReadField<uintptr_t>(work, kWork_Dmapack);
            const uintptr_t cv2     = Memory::ReadField<uintptr_t>(work, kWork_Cv2);
            if (!dmapack || !cv2) return;

            const int clock = g_GameVars.DG_Clock() & 1;
            const int nVerts  = Memory::ReadField<int>(cv2, kCv2_NVertsIndex);
            const int nMin    = Memory::ReadField<int>(work, kWork_ActMDrops);
            const uintptr_t mainBuf = Memory::ReadField<uintptr_t>(work, kWork_DrowVerts  + (ptrdiff_t)clock * 8);
            const uintptr_t minBuf  = Memory::ReadField<uintptr_t>(work, kWork_DrowMVerts + (ptrdiff_t)clock * 8);
            if (nVerts <= 0) return;

            uint8_t* const end = g_autopacket + sizeof(g_autopacket) - 0x40;
            uint8_t* p = g_autopacket;

            p = WriteTrBuffer(p);
            p = WriteDrawOffset(p);
            p = reinterpret_cast<uint8_t*>(g_initMdlDraw(p, kMdlTest, kMdlAlpha, kMdlPrim));
            p = TranscodeDrops(p, end, mainBuf, kMainDrops, nVerts);
            p = TranscodeDrops(p, end, minBuf,  nMin,       nVerts);
            WriteEnd(p);

            Commit(dmapack, g_autopacket);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void __fastcall Act_Detour(uintptr_t work)
    {
        g_actHook.fastcall<void>(work);
        if (g_initMdlDraw && work)
        {
            BuildDropsAutopacket(work);
        }
    }
}

void MGS2LensDroplets::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    spdlog::info("MGS 2: Lens Droplets - Initializing...");

    uint8_t* mdlDraw = Memory::PatternScan(baseModule, kInitMdlDrawSig, "MGS 2: Lens Droplets - InitMdlDraw");
    if (!mdlDraw)
    {
        return;
    }
    g_initMdlDraw = reinterpret_cast<InitMdlDrawFn>(mdlDraw);

    if (uint8_t* address = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 54 41 56 41 57 48 83 EC ?? 48 8B 81", "MGS 2: Lens Droplets - shibata\\demo\\scr_drop_demo.c -> NewScrDrop_demo() -> Act()"))
    {
        g_actHook = safetyhook::create_inline(address, reinterpret_cast<void*>(Act_Detour));
        LOG_HOOK(g_actHook, "MGS 2: Lens Droplets - Act");
    }
}
