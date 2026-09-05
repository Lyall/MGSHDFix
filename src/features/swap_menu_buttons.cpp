#include "stdafx.h"

#include "swap_menu_buttons.hpp"

#include "common.hpp"
#include "helper.hpp"
#include "logging.hpp"



void SwapMenuButtons::SetMenuButtonInputs()
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }

    if (force_menu_buttons == ConfigKeys::MenuButton_Option_Default)
    {
        spdlog::info("MGS 2 | MGS 3: Swap Menu Buttons: Using default menu button inputs, skipping.");
        return;
    }

    if (eGameType & MGS3)
    {
        uint8_t* BP_Pad_GetOkAssignment = Memory::PatternScan(baseModule, "48 83 EC 28 48 8B 05 ?? ?? ?? ?? ?? ?? ?? 83 79 ?? 00 75 ?? E8 ?? ?? ?? ?? 85 C0 74 ?? 8B 0D ?? ?? ?? ?? 0F BA E1 15", "MGS 3: Swap Menu Buttons: bp\\shared\\BP_Misc.cpp | BP_Pad_GetOkAssignment()");
        uint8_t* BP_Pad_GetCancelAssignment = Memory::PatternScan(baseModule, "48 83 EC 28 48 8B 05 ?? ?? ?? ?? ?? ?? ?? 83 79 ?? 00 75 ?? E8 ?? ?? ?? ?? 85 C0 74 ?? 8B 0D ?? ?? ?? ?? 0F BA E1 14", "MGS 3: Swap Menu Buttons: bp\\shared\\BP_Misc.cpp | BP_Pad_GetCancelAssignment()");
        if (!BP_Pad_GetOkAssignment || !BP_Pad_GetCancelAssignment)
        {
            spdlog::error("MGS 3: Swap Menu Buttons: Failed to find button assignment function. Skipping.");
            return;
        }

        constexpr const char* FORCE_JAPAN_REGION_BRANCH = "\xEB"; //East
        constexpr const char* FORCE_USA_REGION_BRANCH = "\x90\x90"; //South
        const bool eastForOK = force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK;

        Memory::PatchBytes(reinterpret_cast<uintptr_t>(BP_Pad_GetOkAssignment + 0x12), eastForOK ? FORCE_JAPAN_REGION_BRANCH : FORCE_USA_REGION_BRANCH, eastForOK ? 1 : 2);
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(BP_Pad_GetCancelAssignment + 0x12), eastForOK ? FORCE_JAPAN_REGION_BRANCH : FORCE_USA_REGION_BRANCH, eastForOK ? 1 : 2);
        return;
    }

    uint8_t* BP_Pad_GetOkAssignment_Scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 4C 8D 3D ?? ?? ?? ?? 85 C6", "MGS 2: Swap Menu Buttons: bp\\shared\\BP_Misc.cpp | BP_Pad_GetOkAssignment()");
    uint8_t* BP_Pad_GetCancelAssignment_Scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 85 C6 74", "MGS 2: Swap Menu Buttons: bp\\shared\\BP_Misc.cpp | BP_Pad_GetCancelAssignment()");
    if (!BP_Pad_GetOkAssignment_Scan || !BP_Pad_GetCancelAssignment_Scan)
    {
        spdlog::error("MGS 2: Swap Menu Buttons: Failed to find button assignment function. Skipping.");
        return;
    }

    uintptr_t BP_Pad_GetOkAssignment = Memory::GetRelativeOffset(BP_Pad_GetOkAssignment_Scan + 1);
    uintptr_t BP_Pad_GetCancelAssignment = Memory::GetRelativeOffset(BP_Pad_GetCancelAssignment_Scan + 1);

    constexpr const char* EAST_BUTTON_OPCODE = "\x04"; // PS2 CIRCLE
    constexpr const char* SOUTH_BUTTON_OPCODE = "\x08"; // PS2 CROSS

    Memory::PatchBytes(BP_Pad_GetOkAssignment + 0x0A, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? EAST_BUTTON_OPCODE : SOUTH_BUTTON_OPCODE, 1);
    Memory::PatchBytes(BP_Pad_GetOkAssignment + 0x12, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? EAST_BUTTON_OPCODE : SOUTH_BUTTON_OPCODE, 1);

    
    Memory::PatchBytes(BP_Pad_GetCancelAssignment + 0x0A, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? SOUTH_BUTTON_OPCODE : EAST_BUTTON_OPCODE, 1);
    Memory::PatchBytes(BP_Pad_GetCancelAssignment + 0x12, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? SOUTH_BUTTON_OPCODE : EAST_BUTTON_OPCODE, 1);
    
}
