#include "stdafx.h"
#include "mgs2_demo_effect_blacklist.hpp"

#include "common.hpp"
#include "logging.hpp"

// The MC blacklists demo fade/flush launches per stage (photosensitivity trim) - d12t3 loses its detonation flash and underwater green.

void MGS2DemoEffectBlacklist::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    if (uint8_t* address = Memory::PatternScan(
            baseModule,
            "E8 ?? ?? ?? ?? 33 FF 4C 8D 05 ?? ?? ?? ?? 8B D7 0F 1F 40 00 0F B6 0C 10",
            "MGS 2: Demo Effect Blacklist: mode\\demo\\demo_eft.c -> NewFadeInOutForce_Demo() launch gate"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address), "\xE9\xE9\x00\x00\x00", 5);
        spdlog::info("MGS 2: Demo Effect Blacklist: fade launch blacklist bypassed.");
    }

    if (uint8_t* address = Memory::PatternScan(
            baseModule,
            "E8 ?? ?? ?? ?? 33 C9 4C 8D 05 ?? ?? ?? ?? 66 0F 1F 84 00 00 00 00 00 0F B6 14 08",
            "MGS 2: Demo Effect Blacklist: mode\\demo\\demo_eft.c -> NewFlush() launch gate"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address), "\xE9\x53\x00\x00\x00", 5);
        spdlog::info("MGS 2: Demo Effect Blacklist: flush launch blacklist bypassed.");
    }
}
