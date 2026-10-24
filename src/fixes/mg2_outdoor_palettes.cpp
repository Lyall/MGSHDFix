#include "stdafx.h"
#include "mg2_outdoor_palettes.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    struct OutdoorColour
    {
        uint32_t nNormal;
        uint32_t nDark;
        uint32_t nBright;
    };

    // MSX bank 56 lists B96A/B9A4/B9B7; slot 197 has no MSX equivalent and is unused in R0 -> 9.
    constexpr OutdoorColour astColours[] =
    {
        { 0xDAB600, 0x916D00, 0xDAB600 },
        { 0x6D4800, 0x6D4800, 0x6D4800 },
        { 0xB66D00, 0xB66D00, 0xB66D00 },
        { 0xFFFFFF, 0xFFFFFF, 0xFFFFFF },
        { 0xDADADA, 0x6D6D6D, 0xDADADA },
        { 0xB6B6B6, 0xB6B6B6, 0xB6B6B6 },
        { 0x919191, 0x6D6D6D, 0x919191 },
        { 0x6D6D6D, 0x484848, 0x919191 },
        { 0x484848, 0x242424, 0x6D6D6D },
        { 0x242424, 0x000000, 0x484848 },
        { 0x000000, 0x000000, 0x000000 },
    };

    SafetyHookMid s_hUploadPalette{};

    void RestorePalette(SafetyHookContext& ctx)
    {
        const int nEffect = *pPaletteEffect;
        if (GM_Area != 0 || GM_Room < 0 || GM_Room > 9 || (nEffect != MG2_PALETTE_OUTDOOR_DARK && nEffect != MG2_PALETTE_OUTDOOR_BRIGHT))
        {
            return;
        }

        const auto* pnBase = reinterpret_cast<const uint32_t*>(ctx.r15) + nBackgroundPaletteStart;
        auto* pstPalette = reinterpret_cast<PaletteColour*>(ctx.rbx - 1) + nBackgroundPaletteStart;
        for (size_t nIndex = 0; nIndex < nBackgroundPaletteCount; nIndex++)
        {
            uint32_t nColour = pnBase[nIndex];
            if (nIndex < std::size(astColours) && nColour == astColours[nIndex].nNormal)
            {
                nColour = nEffect == MG2_PALETTE_OUTDOOR_DARK ? astColours[nIndex].nDark : astColours[nIndex].nBright;
            }
            pstPalette[nIndex] = { static_cast<uint8_t>(nColour >> 16), static_cast<uint8_t>(nColour >> 8), static_cast<uint8_t>(nColour) };
        }
    }
}

void MG2_OutdoorPalettes::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //UpdatePalette+2A8
    uint8_t* pUploadPalette = Memory::PatternScan(mg2Module, "33 ED 8B FD 0F 1F 40 ?? 44 0F B6 4B ?? 8B CF", "MG2: Outdoor Palettes : UpdatePalette()+0x2A8");
    if (!pUploadPalette || !pPaletteEffect || !pScriptGlobals)
    {
        spdlog::error("MG2: Outdoor Palettes : Missing palette or room addresses. Skipping.");
    }
    else
    {
        s_hUploadPalette = safetyhook::create_mid(pUploadPalette, RestorePalette);
        LOG_HOOK(s_hUploadPalette, "MG2: Outdoor Palettes : UpdatePalette()+0x2A8")
        if (s_hUploadPalette)
        {
            spdlog::info("MG2: Outdoor Palettes : Initialized.");
        }
    }
}
