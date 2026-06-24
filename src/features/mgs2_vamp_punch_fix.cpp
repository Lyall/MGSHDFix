#include "stdafx.h"
#include "mgs2_vamp_punch_fix.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

namespace
{
    bool bIsW31c = false;
}

void MGS2VampFPVPunch::HandleLevelTransition()
{
	if (!bEnabled)
	{
	    return;
	}
	bIsW31c = (g_GameVars.IsStage(MGS2Stages::W31C) || g_GameVars.IsStage(MGS2Stages::A31C));
}

void MGS2VampFPVPunch::Apply()
{
	if (!(eGameType & MGS2))
	{
		return;
	}

	if (!bEnabled)
	{
		spdlog::info("MGS2: Vamp Non-lethal FPV Punches: Disabled via config, skipping.");
		return;
	}

	// Pretty simple fix for user/shibata/vamp/vamp.c, it sets a bitflag including the damage value.
	// The value is offset 9 bits for lethal, 15 for non-lethal.
	// So we change the SHL instruction value to 15.
    if (uint8_t* vmp_shl = Memory::PatternScan(baseModule, "09 44 0B C0 41 8B C0 83 C8 0C 81 E1 00 00 01 00", "Vamp FPV Punch"))
    {
        Memory::PatchBytes((uintptr_t)vmp_shl, "\x0f", 1);
    }

	///todo - speedrunner HUD should indicate that this is enabled in w31c
}
