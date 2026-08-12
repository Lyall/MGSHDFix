#include "stdafx.h"
#include "mgs2_codec_band.hpp"

#include "common.hpp"
#include "logging.hpp"

// The port computes the codec CRT band's off-screen wait as 2*height ticks (~3s); the PS2 divided
// by 8 sixteenths (~0.2s), so the band should re-enter almost immediately.

namespace
{
    SafetyHookMid TrBufferHeight_hooks[2] {};
}

void MGS2CodecBand::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    // The draw passes the band's box bottom as the TrBuffer height, so the renderer's vertical scale
    // shrinks as the band descends and the sweep eases out - feed it the window height instead.
    auto sites = Memory::FindMultiplePatternMatches(baseModule,
        "F3 0F 11 4C 24 20 0F 57 C9 E8 ?? ?? ?? ?? 48 BA A9 00 00 00 64 00 00 00");
    if (sites.size() == 2)
    {
        for (int i = 0; i < 2; i++)
        {
            TrBufferHeight_hooks[i] = safetyhook::create_mid(sites[i], [](SafetyHookContext& ctx) {
                ctx.xmm1.f32[0] = static_cast<float>(*reinterpret_cast<const int32_t*>(ctx.rbx + 0x74));
            });
            LOG_HOOK(TrBufferHeight_hooks[i], "MGS 2: Codec Band: user\\mode\\codec\\codeccrt.c -> proc_black_zone() | TrBuffer height")
        }
    }
    else
    {
        spdlog::error("MGS 2: Codec Band: expected 2 TrBuffer sites, found {}.", sites.size());
    }

    MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? 75 ?? F3 0F 58 0D", "MGS 2: Codec Band: user\\mode\\codec\\codeccrt.c -> init_black_zone() | wait store", {
        ctx.rax >>= 4;
    });

    MAKE_HOOK_MID(baseModule, "89 81 ?? ?? ?? ?? F6 05 ?? ?? ?? ?? ?? F3 0F 58 CA", "MGS 2: Codec Band: user\\mode\\codec\\codeccrt.c -> proc_black_zone() | wait store", {
        ctx.rax >>= 4;
    });
}
