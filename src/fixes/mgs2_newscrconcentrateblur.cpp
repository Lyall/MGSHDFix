#include "stdafx.h"
#include "common.hpp"
#include "mgs2_newscrconcentrateblur.hpp"

#include "logging.hpp"
#include "mgs2_autopacket.hpp"
#include "gamevars.hpp"

// scr_conblur.c

namespace
{
    using namespace MGS2Autopacket;

    constexpr ptrdiff_t kWork_NMverts = 0x5c;  
    constexpr ptrdiff_t kWork_MSize   = 0x60;  
    constexpr ptrdiff_t kWork_Mverts  = 0x78;  
    constexpr ptrdiff_t kWork_Dmapack = 0x88;  
    constexpr ptrdiff_t kWork_Alpha   = 0xa0;  

    constexpr int       kNVerts         = 6;                                  
    constexpr ptrdiff_t kVertHeader     = 0x10;                               
    constexpr ptrdiff_t kVertStride     = 0x20;                               
    constexpr int       kExpectedMSize  = kVertHeader + kNVerts * kVertStride;


    constexpr int kFanTris[4][3] = { {0,1,2}, {0,2,3}, {0,3,4}, {0,4,5} };

    constexpr int kPosOffX = 1792;
    constexpr int kPosOffY = 1824;

    constexpr uint64_t kMdlTest = 0x30000ULL; 
    constexpr uint64_t kMdlPrim = 0x154ULL;   

    SafetyHookInline g_spawnHook{};
    SafetyHookInline g_actHook{};
    InitMdlDrawFn    g_initMdlDraw = nullptr;

    alignas(16) uint8_t g_autopacket[0x4000];

    struct CapturedColor { uintptr_t work = 0; uint32_t rgb = 0; };
    CapturedColor g_captured[4];

    void CaptureColor(uintptr_t work, unsigned int col)
    {
        const uint32_t rgb = (col >> 24) | (((col >> 16) & 0xFF) << 8) | (((col >> 8) & 0xFF) << 16) | (0x80u << 24);
        for (auto& slot : g_captured)
        {
            if (slot.work == work || slot.work == 0) { slot.work = work; slot.rgb = rgb; return; }
        }
        g_captured[0] = { work, rgb }; 
    }

    uint32_t LookupColor(uintptr_t work)
    {
        for (auto& slot : g_captured)
        {
            if (slot.work == work) return slot.rgb;
        }
        return 0x80808080u;
    }

    uint8_t* TranscodeFan(uint8_t* p, uint8_t* end, uintptr_t buffer, uint32_t color)
    {
        if (!buffer) return p;

        struct V { float sx, sy, un, vn; };
        V v[kNVerts];

        const uint8_t* base = reinterpret_cast<const uint8_t*>(buffer + kVertHeader);
        for (int i = 0; i < kNVerts; ++i)
        {
            const uint8_t* vp  = base + (ptrdiff_t)i * kVertStride;
            const uint64_t uv  = *reinterpret_cast<const uint64_t*>(vp + 0x00);   // MVERT_DATA_F.uv.data
            const uint64_t xyz = *reinterpret_cast<const uint64_t*>(vp + 0x10);  // MVERT_DATA_F.xyz.data

            const int sx = (int)(uint16_t)(xyz & 0xFFFF) >> 4;
            const int sy = (int)(uint16_t)((xyz >> 16) & 0xFFFF) >> 4;
            const int up = (int)(uint16_t)(uv & 0xFFFF) >> 4;
            const int vt = (int)(uint16_t)((uv >> 16) & 0xFFFF) >> 4;

            v[i] = { (float)(sx - kPosOffX), (float)(sy - kPosOffY), (float)up / kDrawW, (float)vt / kDrawH };
        }

        for (const auto& tri : kFanTris)
        {
            if (p + 0x10 + 3 * kVertexSize > end) break;

            uint32_t* countField = nullptr;
            p = WriteStripHeader(p, &countField);
            for (int idx : tri)
            {
                p = WriteVertex(p, v[idx].sx, v[idx].sy, v[idx].un, v[idx].vn, color);
            }
            *countField = 3;
        }
        return p;
    }

    void BuildBlurAutopacket(uintptr_t work)
    {
        __try
        {
            const uintptr_t dmapack = Memory::ReadField<uintptr_t>(work, kWork_Dmapack);
            if (!dmapack) return;

            const int nMverts = Memory::ReadField<int>(work, kWork_NMverts);
            const int mSize   = Memory::ReadField<int>(work, kWork_MSize);
            if (nMverts != kNVerts || mSize != kExpectedMSize) return;

            const int clock = g_GameVars.DG_Clock() & 1;
            const uintptr_t buffer = Memory::ReadField<uintptr_t>(work, kWork_Mverts + (ptrdiff_t)clock * 8);
            if (!buffer) return;

            const uint32_t rgb   = LookupColor(work);
            const float    alpha = Memory::ReadField<float>(work, kWork_Alpha);
            const uint64_t mdlAlpha = 0x64ULL | ((uint64_t)(int)alpha << 32);

            uint8_t* const end = g_autopacket + sizeof(g_autopacket) - 0x40;
            uint8_t* p = g_autopacket;

            p = WriteTrBuffer(p);
            p = WriteDrawOffset(p);
            p = reinterpret_cast<uint8_t*>(g_initMdlDraw(p, kMdlTest, mdlAlpha, kMdlPrim));
            p = TranscodeFan(p, end, buffer, rgb);
            WriteEnd(p);

            Commit(dmapack, g_autopacket);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void* __fastcall NewScrConcentrateBlur_Detour(int conName, unsigned int col, float maxPix, int name)
    {
        void* work = g_spawnHook.fastcall<void*>(conName, col, maxPix, name);
        if (work)
        {
            CaptureColor(reinterpret_cast<uintptr_t>(work), col);
        }
        return work;
    }

    void __fastcall Act_Detour(uintptr_t work)
    {
        g_actHook.fastcall<void>(work);
        if (g_initMdlDraw && work)
        {
            BuildBlurAutopacket(work);
        }
    }
}

void MGS2ConcentrateBlur::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    spdlog::info("MGS 2: Concentrate Blur - Initializing...");

    uint8_t* mdlDraw = Memory::PatternScan(baseModule, kInitMdlDrawSig, "MGS 2: Concentrate Blur - InitMdlDraw");
    if (!mdlDraw)
    {
        return;
    }
    g_initMdlDraw = reinterpret_cast<InitMdlDrawFn>(mdlDraw);

    if (uint8_t* address = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 41 8B F9 0F 29 74 24 ?? 45 33 C9 8B F2 8B E9", "MGS 2: Concentrate Blur - shibata\\demo\\scr_conblur.c -> NewScrConcentrateBlur()"))
    {
        g_spawnHook = safetyhook::create_inline(address, reinterpret_cast<void*>(NewScrConcentrateBlur_Detour));
        LOG_HOOK(g_spawnHook, "MGS 2: Concentrate Blur - NewScrConcentrateBlur");
    }

    if (uint8_t* address = Memory::PatternScan(baseModule, "48 8B C4 48 89 58 ?? 48 89 70 ?? 48 89 78 ?? 41 56 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 0F 29 78", "MGS 2: Concentrate Blur - shibata\\demo\\scr_conblur.c -> NewScrConcentrateBlur() -> Act()"))
    {
        g_actHook = safetyhook::create_inline(address, reinterpret_cast<void*>(Act_Detour));
        LOG_HOOK(g_actHook, "MGS 2: Concentrate Blur - Act");
    }
}
