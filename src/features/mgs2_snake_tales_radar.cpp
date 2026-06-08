#include "stdafx.h"
#include "mgs2_snake_tales_radar.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

void MGS2_SnakeTalesRadar::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        return;
    }


    MAKE_HOOK_MID(baseModule, "3D ?? ?? ?? ?? 75 ?? 81 0D ?? ?? ?? ?? ?? ?? ?? ?? 33 C0", "game\\gamed.c -> NewSetMenuStatus() @ l2029", {
        if (g_GameVars.MGS2_GetGameMode() == MGS2GameMode::Alternate)
        {
            ctx.rax = 3662; //if(MENU_NODE_ACCESSED == GM_STRCODE_ON) = true
        }
                  });

}

