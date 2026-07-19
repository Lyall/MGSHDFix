#include "stdafx.h"

#include "mgs2_thermal_goggles.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "input_handler.hpp"
#include "logging.hpp"
#include "mgs2_equipment_enums.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs2_status_flags.hpp"

using namespace MGS2_StatusFlags;

#include "game_funcs.hpp"
#include "game_defines.hpp"
using namespace MGS2_GameFuncs;
using namespace MGS2_Defines;
using namespace MGS2_LinkVarBuf;

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


    const uint32_t* g_irTables[] = { irSubstance, irSplinterCell, irRedHot, irWhiteHot, irBlackHot };
    static_assert(std::size(g_irTables) == (size_t)MGS2ThermalGoggles::IRMode::Count);

    uint8_t* g_irColorTable = nullptr;

    bool deequipped_this_frame = false;
    bool deequipped_last_frame = false;
    bool processing_reequip = false;
}

void MGS2ThermalGoggles::Tick()
{
    if (!processing_reequip)
    {
        return;
    }
    if (g_GameVars.Get_GM_GameStatus() & (STATE_GAMEOVER | STATE_PLAY_DEMO))
    {
        deequipped_this_frame = false;
        deequipped_last_frame = false;
        processing_reequip = false;
        return;
    }
    if (deequipped_this_frame)
    {
        deequipped_this_frame = false;
        deequipped_last_frame = true;
        return;
    }

    if (deequipped_last_frame)
    {
        GM_Item = MGS2_ITEM_INDEX_THERMAL_GOGGLES;
        deequipped_last_frame = false;
        return;
    }
    processing_reequip = false;

}

void MGS2ThermalGoggles::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        return;
    }
    
    spdlog::info("MGS2ThermalGoggles: Setting up thermal goggle color cycling.");
    if (uint8_t* irColorTable = Memory::PatternScan(baseModule, "00 80 FF 80 00 FF FF 80 00 FF 80 80 00 FF 00 80 80 FF 00 80 FF FF 00 80 FF 80 00 80 FF 00 00 80", "IR color table"))
    {
        spdlog::info("MGS2ThermalGoggles: IR color table found at {:s}+{:X}", sExeName.c_str(), (uintptr_t)irColorTable - (uintptr_t)baseModule);
        g_irColorTable = irColorTable;
        if (g_irMode != IRMode::Substance)
        {
            Memory::PatchBytes((uintptr_t)irColorTable, reinterpret_cast<const char*>(g_irTables[(int)g_irMode]), 32);
            spdlog::info("MGS2ThermalGoggles: Applying IR mode {}.", (int)g_irMode);
        }
    }
    else
    {
        return;
    }


    g_InputHandler.RegisterHotkey(vk_ToggleThermalGoggleColor, "MGS2 - Thermal Goggle Cycle Mode", []() {
        if (processing_reequip)
        {
            return;
        }
        if (GM_Item != MGS2_ITEM_INDEX_THERMAL_GOGGLES)
        {
            return;
        }
        if (g_GameVars.Get_GM_GameStatus() & (STATE_GAMEOVER | STATE_PLAY_DEMO))
        {
            return;
        }

        processing_reequip = deequipped_this_frame = true;
        g_irMode = (IRMode)(((int)g_irMode + 1) % (int)IRMode::Count);
        memcpy(g_irColorTable, g_irTables[(int)g_irMode], 32);
        GM_Item = 0;
        GM_SeSet(GM_PAN_CENTER, GM_MAX_VOL, SD_S_TYPING03);
        });



}
