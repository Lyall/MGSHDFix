#include "stdafx.h"

#include "mgs2_thermal_goggles.hpp"
#include "common.hpp"
#include "logging.hpp"


namespace
{

    constexpr uint32_t IRColor(const char* hex)
    {
        if (hex[0] == '#') hex++;
        auto h = [](char c) -> uint32_t {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return c - 'a' + 10;
            };
        uint8_t r = (h(hex[0]) << 4) | h(hex[1]);
        uint8_t g = (h(hex[2]) << 4) | h(hex[3]);
        uint8_t b = (h(hex[4]) << 4) | h(hex[5]);
        return 0x80000000u | (uint32_t(b) << 16) | (uint32_t(g) << 8) | r;
    }

    constexpr uint32_t irSubstance[8] = { //Substance vanilla.
        IRColor("#0000ff"),
        IRColor("#00ffff"),
        IRColor("#00ff80"),
        IRColor("#00ff00"),
        IRColor("#80ff00"),
        IRColor("#ffff00"),
        IRColor("#ff8000"),
        IRColor("#ff0000"),
    };

    constexpr uint32_t irRedHot[8] = {  //Sons of Liberty / Terminator vision
        IRColor("#000000"), //coldest
        IRColor("#240000"),
        IRColor("#490000"),
        IRColor("#6d0000"),
        IRColor("#920000"),
        IRColor("#b60000"),
        IRColor("#db0000"),
        IRColor("#ff0000"), //hot HOT HOTTTTTTT!
    };

    constexpr uint32_t irSplinterCell[8] = {
        IRColor("#000000"), 
        IRColor("#1a0030"), 
        IRColor("#6b0060"), 
        IRColor("#c70040"),
        IRColor("#ff3000"), 
        IRColor("#ff9000"), 
        IRColor("#ffff00"), 
        IRColor("#ffffff"),
    };

    constexpr uint32_t irWhiteHot[8] = {
        IRColor("#000000"),
        IRColor("#242424"),
        IRColor("#494949"),
        IRColor("#6d6d6d"),
        IRColor("#929292"),
        IRColor("#b6b6b6"),
        IRColor("#dbdbdb"),
        IRColor("#ffffff"),
    };

    constexpr uint32_t irBlackHot[8] = {
        IRColor("#ffffff"),
        IRColor("#dbdbdb"),
        IRColor("#b6b6b6"),
        IRColor("#929292"),
        IRColor("#6d6d6d"),
        IRColor("#494949"),
        IRColor("#242424"),
        IRColor("#000000"),
    };


    const uint32_t* g_irTables[] = { irSubstance, irRedHot, irSplinterCell, irWhiteHot, irBlackHot };
    int g_irTableIndex = 0;
    static int s_lastTableIndex = -1;

}


void MGS2ThermalGoggles::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    if (!bEnabled)
    {
        spdlog::info("Config disabled, skipping");
        return;
    }

    if (uint8_t* irColorTable = Memory::PatternScan(baseModule, "00 80 FF 80 00 FF FF 80 00 FF 80 80 00 FF 00 80 80 FF 00 80 FF FF 00 80 FF 80 00 80 FF 00 00 80", "IR color table"))
    {
        Memory::PatchBytes((uintptr_t)irColorTable, reinterpret_cast<const char*>(irRedHot), sizeof(irRedHot));

        /* experimental - swap tables via item menu.
        MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 57 41 56 41 57 48 81 EC ?? ?? ?? ?? 0F 29 B4 24", "ir_mode.c -> GetResources() - ir table replace", {
             if (ctx.rdx == 0 && g_irTableIndex != s_lastTableIndex)
             {
                  memcpy(reinterpret_cast<void*>(irColorTable), g_irTables[g_irTableIndex], 32);
                  s_lastTableIndex = g_irTableIndex;
             }
            });
            */
    }


}
