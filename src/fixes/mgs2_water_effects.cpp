#include "stdafx.h"
#include "common.hpp"
#include "mgs2_water_effects.hpp"

#include "helper.hpp"
#include "logging.hpp"

void MGS2WaterEffects::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    if (uint8_t* address = Memory::PatternScan(baseModule,
            "C6 44 ?? 06 00 ?? C6 44 ?? 16 00 ?? C6 44 ?? 26 00 ?? C6 44 ?? 36 00",
            "MGS 2: Water Effects - Surface Splash Alpha"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 4,  "\xFF", 1);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 10, "\xFF", 1);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 16, "\xFF", 1);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(address) + 22, "\xFF", 1);
        spdlog::info("MGS 2: Water Effects - Surface splash alpha restored.");
    }
}
