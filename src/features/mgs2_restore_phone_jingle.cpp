#include "stdafx.h"
#include "mgs2_restore_phone_jingle.hpp"
#include "common.hpp"
#include "logging.hpp"

void MGS2_RestorePhoneJingle::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS2: Restore Phone Jingle: Disabled via config, skipping.");
        return;
    }


    if (uint8_t* phoneJingle = Memory::PatternScan(baseModule, "B9 9B 00 00 00 EB", "MGS2: Japanese Phone Jingle"))
    {
        Memory::PatchBytes((uintptr_t)phoneJingle, "\xB9\xA0", 2);
    }

    if (uint8_t* phoneJingle = Memory::PatternScan(baseModule, "B9 9B 00 00 00 89 BB", "MGS2: Japanese Phone Jingle"))
    {
        Memory::PatchBytes((uintptr_t)phoneJingle, "\xB9\xA0", 2);
    }


}
