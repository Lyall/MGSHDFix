#include "stdafx.h"
#include "mgs2_demo_effect_blacklist.hpp"

#include "common.hpp"
#include "logging.hpp"

// The MC blacklists demo fade/flush launches per stage (photosensitivity trim) - d12t3 loses its detonation flash and underwater green.

namespace
{
    uint8_t* NewFadeInOutForce_Demo_0008Launch_Call_scan = nullptr;
    uint8_t* NewFlush_0005Launch_Call_scan = nullptr;
}

void MGS2DemoEffectBlacklist::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    NewFadeInOutForce_Demo_0008Launch_Call_scan = Memory::PatternScan(baseModule, "8B 84 24 ?? ?? ?? ?? 45 8B CC 89 74 24",
        "MGS 2: Demo Effect Blacklist: user\\mode\\demo\\effect\\0008_NewFadeInOutForce_Demo.c-> NewFadeInOutForce_Demo_0008Launch() | NewFadeInOutForce_Demo() call @l58");
    if (NewFadeInOutForce_Demo_0008Launch_Call_scan)
    {
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 33 FF 4C 8D 05", "MGS 2: Demo Effect Blacklist: user\\mode\\demo\\effect\\0008_NewFadeInOutForce_Demo.c-> NewFadeInOutForce_Demo_0008Launch() | mc area check", {
            ctx.rip = reinterpret_cast<uintptr_t>(NewFadeInOutForce_Demo_0008Launch_Call_scan);
        });
    }

    NewFlush_0005Launch_Call_scan = Memory::PatternScan(baseModule, "8B D6 8B CB 48 8B 5C 24",
        "MGS 2: Demo Effect Blacklist: user\\mode\\demo\\effect\\0005_NewFlush.c-> NewFlush_0005Launch() | NewFlush() call @l49");
    if (NewFlush_0005Launch_Call_scan)
    {
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 33 C9 4C 8D 05 ?? ?? ?? ?? 66 0F 1F 84 00", "MGS 2: Demo Effect Blacklist: user\\mode\\demo\\effect\\0005_NewFlush.c-> NewFlush_0005Launch() | mc area check", {
            ctx.rip = reinterpret_cast<uintptr_t>(NewFlush_0005Launch_Call_scan);
        });
    }
}
