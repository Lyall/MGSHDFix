#include "stdafx.h"
#include "mgs2_parrot_radar_fix.hpp"
#include "common.hpp"
#include "logging.hpp"

void MGS2_ParrotRadarFix::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    spdlog::info("MGS2 Parrot: Caw! Fixing parrot radar rotation...");

    MAKE_HOOK_MID(baseModule, "8B 83 ?? ?? ?? ?? 85 C0 75 ?? 8B 83", "MGS2: Parrot Fix", {
        *reinterpret_cast<int16_t*>(ctx.rbx + 0x3B4)    =   *reinterpret_cast<int16_t*>(*reinterpret_cast<uintptr_t*>(ctx.rbx + 0x358) + 0x22)  +   *reinterpret_cast<int16_t*>(ctx.rbx + 0xA02); // npc->action.face_dir = ctrl->rot.vy + svecAdjust.vy
                  });

}
