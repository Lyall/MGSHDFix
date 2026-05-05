#include "stdafx.h"

#include "swap_menu_buttons.hpp"

#include "common.hpp"
#include "helper.hpp"
#include "logging.hpp"



void SwapMenuButtons::SetMenuButtonInputs()
{

    if (!(eGameType & MGS2))
    {
        spdlog::info("MGS 2: Swap Menu Buttons: Only MGS2 is currently supported. Skipping.");
        return;
    }

    if (force_menu_buttons == ConfigKeys::MenuButton_Option_Default)
    {
        spdlog::info("MGS 2: Swap Menu Buttons: Using default menu button inputs, skipping.");
        return;
    }


    uintptr_t BP_Pad_GetOkAssignment = Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 4C 8D 3D ?? ?? ?? ?? 85 C6", "MGS 2: BP_Pad_GetOkAssignment") + 1);
    uintptr_t BP_Pad_GetCancelAssignment = Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 85 C6 74", "MGS 2: BP_Pad_GetCancelAssignment") + 1);
    if (!BP_Pad_GetOkAssignment || !BP_Pad_GetCancelAssignment)
    {
        spdlog::error("MGS 2: Swap Menu Buttons: Failed to find button assignment function. Skipping.");
        return;
    }

    constexpr const char* EAST_BUTTON_OPCODE = "\x04"; // PS2 CIRCLE
    constexpr const char* SOUTH_BUTTON_OPCODE = "\x08"; // PS2 CROSS

    Memory::PatchBytes(BP_Pad_GetOkAssignment + 0x0A, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? EAST_BUTTON_OPCODE : SOUTH_BUTTON_OPCODE, 1);
    Memory::PatchBytes(BP_Pad_GetOkAssignment + 0x12, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? EAST_BUTTON_OPCODE : SOUTH_BUTTON_OPCODE, 1);

    
    Memory::PatchBytes(BP_Pad_GetCancelAssignment + 0x0A, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? SOUTH_BUTTON_OPCODE : EAST_BUTTON_OPCODE, 1);
    Memory::PatchBytes(BP_Pad_GetCancelAssignment + 0x12, (force_menu_buttons == ConfigKeys::MenuButton_Option_EastForOK) ? SOUTH_BUTTON_OPCODE : EAST_BUTTON_OPCODE, 1);
    
}
