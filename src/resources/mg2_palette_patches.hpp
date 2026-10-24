#pragma once

// Override follows a palette entry; SlotOverride writes a fixed, unused base-table slot.

namespace mg2_palette_patches
{
    struct Override
    {
        int nEntry;
        int nIndex;
        std::uint32_t nRgb;
    };

    struct SlotOverride
    {
        int nSlot;
        std::uint32_t nRgb;
    };

    inline constexpr std::array<Override, 0> astOverrides = {};

    inline constexpr std::array<SlotOverride, 0> astSlotOverrides = {};
}
