#include "stdafx.h"
#include "mg2_door_cards.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

namespace
{
    SafetyHookMid s_hDoorScriptResult {};

    void DoorScriptResult_hook(SafetyHookContext& stContext)
    {
        if (!(stContext.rflags & kRflagsZeroFlag))
        {
            return;
        }

        const auto* pDoor = reinterpret_cast<const MG2_LinkVarBuf::DoorState*>(stContext.rdi);
        const int nLevel = pDoor->nCardLevel;
        if (nLevel < 1 || nLevel > MG2_ITEM_INDEX_MAX_CARD_LEVEL)
        {
            return;
        }

        if (MG2_LinkVarBuf::HasCardForLevel(nLevel))
        {
            stContext.rflags &= ~kRflagsZeroFlag;
        }
    }
}

void MG2_DoorCards::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //TickDoorTrigger+106
    uint8_t* pDoorCheck = Memory::PatternScan(mg2Module, "74 ?? 41 B8 ?? ?? ?? ?? 48 8B D7", "MG2: Door Cards : TickDoorTrigger()+0x106 : script 154 result");

    if (!MG2_Gamefuncs::HasItem || !pDoorCheck)
    {
        spdlog::error("MG2: Door Cards : Failed to find door check. Skipping door card hook.");
		return;
    }

	s_hDoorScriptResult = safetyhook::create_mid(pDoorCheck, DoorScriptResult_hook);
	LOG_HOOK(s_hDoorScriptResult, "MG2: Door Cards : TickDoorTrigger()+0x106 : script 154 result")
	if (s_hDoorScriptResult)
	{
		spdlog::info("MG2: Door Cards : Initialized.");
	}

}
