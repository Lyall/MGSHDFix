#include "stdafx.h"
#include "mgs2_bandana_mass.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

namespace
{
    // Cutscenes build the bandana from the calm samples and the light tail thrashes. Konami shipped
    // heavier versions of both at 7 and 8, so use those instead.
    constexpr int kBandana1 = 2;
    constexpr int kBandana2 = 3;
    constexpr int kBandana1Heavy = 7;
    constexpr int kBandana2Heavy = 8;

    SafetyHookMid ropeSample_hook {};

    void RopeSample_hook(SafetyHookContext& ctx)
    {
        if (static_cast<int32_t>(ctx.rsi) == kBandana1)
        {
            ctx.rsi = kBandana1Heavy;
        }
        else if (static_cast<int32_t>(ctx.rsi) == kBandana2)
        {
            ctx.rsi = kBandana2Heavy;
        }
    }
}

void MGS2BandanaMass::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    if (uint8_t* sample = Memory::PatternScan(baseModule,
        "48 8D 15 ?? ?? ?? ?? 48 89 BB ?? ?? ?? ?? 48 6B CE 1C",
        "MGS 2: Bandana Mass : user\\kano\\rope\\ropemain.c -> GetResources_called() sample"))
    {
        ropeSample_hook = safetyhook::create_mid(sample, RopeSample_hook);
        LOG_HOOK(ropeSample_hook, "MGS 2: Bandana Mass : ropemain.c -> sample index")
    }
}
