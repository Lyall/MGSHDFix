#include "stdafx.h"

#include "mgs2_coolant_mirror.hpp"

#include "common.hpp"

#include "logging.hpp"

namespace {
    short** GM_Item;
}

void CoolantMirrorFix::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    // The function for creating the mist-receiving object uses incorrect offsets for breakable glass.
    // This seems to be an issue with calculating an average of two vectors, so dividing each component works.

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 89 83 68 02 00 00 33 C0", "MGS2: Coolant Mirror", {
        ((float*)ctx.rcx)[0] /= 2.0f;
        ((float*)ctx.rcx)[1] /= 2.0f;
        ((float*)ctx.rcx)[2] /= 2.0f;
    });

    // Grab GM_Item out of some Olga code, why not
    if (uint8_t* OlgaMouthDisp = Memory::PatternScan(baseModule, "F7 81 E0 13 00 00 00 00 00 08 75 3A 48 8B 05", "MGS2: Coolant Mirror")) {
        //uintptr_t globsBase = *(uintptr_t*)Memory::GetRelativeOffset(OlgaMouthDisp + 15);
        //GM_Item = (short*)(globsBase + 0x106);
        GM_Item = (short**)Memory::GetRelativeOffset(OlgaMouthDisp + 15);
        // Xbox ifdefed feature: Different alpha settings in thermal vision
        MAKE_HOOK_MID(baseModule, "44 8B 43 ?? B8", "MGS2: Coolant Mirror", {
            const auto prim = *reinterpret_cast<uintptr_t*>(ctx.rbx + 88);
            *reinterpret_cast<uint64_t*>(prim + 0x200) = 
                (GM_Item[0][0x83] == 13) // GM_Item == IT_Thermal
                    ? 0x42ULL    // SCE_GS_SET_ALPHA(2, 0, 0, 1, 0)
                    : 0x44ULL;   // SCE_GS_SET_ALPHA(0, 1, 0, 1, 0)
        });

    }


}
