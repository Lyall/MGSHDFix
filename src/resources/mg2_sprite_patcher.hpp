#pragma once

namespace MG2_SpritePatcher
{
    bool AddPatch(int nImageEntry, uint64_t nOriginalHash, const uint8_t* pBlob, size_t nSize);

    bool AddPalettePatch(int nPaletteEntry, int nColourIndex, uint32_t nRgb);

    void Initialize();

    bool IsPatched(int nImageEntry);

    inline constexpr bool bAllowReplacements = true;
}
