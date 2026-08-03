#include "stdafx.h"
#include "mgs2_glass_dmapack_overflow.hpp"

#include "common.hpp"
#include "logging.hpp"

// MC added a fourth setter to brk_gls_ini.c's packet chain without growing the buffer, so the
// terminator lands one byte past it, on the next heap block's prev link.
namespace
{
    constexpr uint8_t kOriginalSize = 0x40;
    constexpr uint8_t kPatchedSize = 0x50;      // 0x48 would do; keep the block 16-aligned
}

void MGS2_GlassDmapackOverflow::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    // The store to +0x2a0 is work->packet, which is what tells this malloc from any other.
    uint8_t* address = Memory::PatternScan(baseModule,
        "B9 40 00 00 00 E8 ?? ?? ?? ?? 48 89 83 A0 02 00 00",
        "MGS 2: Glass Packet Overflow | brk_gls_ini.c -> InitDmapack()");
    if (address == nullptr)
    {
        return;
    }

    if (address[1] != kOriginalSize)
    {
        spdlog::warn("MGS 2: Glass Packet Overflow: buffer is already {:#x}, leaving it alone.",
            address[1]);
        return;
    }

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 1, "\x50", 1);
    spdlog::info("MGS 2: Glass Packet Overflow: buffer grown to {:#x} bytes.", kPatchedSize);
}
