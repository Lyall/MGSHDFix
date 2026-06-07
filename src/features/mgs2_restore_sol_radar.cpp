#include "stdafx.h"

#include "mgs2_restore_sol_radar.hpp"
#include "common.hpp"
#include "logging.hpp"

void MGS2_RestoreSoLRadar::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    if (!bEnabled)
    {
        return;
    }



    MAKE_HOOK_MID(baseModule, "B9 ?? ?? ?? ?? 66 23 C1 B9 ?? ?? ?? ?? 66 89 87", "MGS2: Restore Sons of Liberty radar rotation", {
        ctx.rax = 0;
                  })



}