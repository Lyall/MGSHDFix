#pragma once


namespace mg2_sprite_patches
{
    struct Patch
    {
        int nEntry;
        std::uint64_t nHash;
        const std::uint8_t* pBlob;
        std::size_t nSize;
    };

    inline constexpr std::array<Patch, 0> astPatches = {};
}
