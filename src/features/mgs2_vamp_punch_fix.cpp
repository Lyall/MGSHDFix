#include "stdafx.h"
#include "mgs2_vamp_punch_fix.hpp"
#include "common.hpp"
#include "logging.hpp"

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
	Memory::PatchBytes((uintptr_t)Memory::PatternScan(baseModule, "09 44 0B C0 41 8B C0 83 C8 0C 81 E1 00 00 01 00", "Vamp FPV Punch"), "\x0f", 1);
}
