#include "stdafx.h"

#include "playtime_fixes.hpp"

#include "common.hpp"
#include "input_handler.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs2_status_flags.hpp"
#include "mgs3_linkvarbuf.hpp"
#include "mgs3_status_flags.hpp"
#include "mg1_gamevar_enums.hpp"
#include "mg2_gamevar_enums.hpp"

void FixPlaytime::Apply()
{
    if (!(eGameType & (MGS2|MGS3)))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2 | MGS 3: Playtime Fix: Disabled via config. Skipping...");
        return;
    }

    if (eGameType & MGS2)
    {
        using namespace MGS2_LinkVarBuf;
        MAKE_HOOK_MID(baseModule, "C7 05 ?? ?? ?? ?? 00 00 00 00 48 83 C4 ?? 5F C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 40 53", "gm_startloader", {
            GM_Configuration |= GM_CONFIG_PLAYTIME_STOP;
                      });

    }
    else // eGameType & MGS3
    { 
        using namespace MGS3_LinkVarBuf;
        MAKE_HOOK_MID(baseModule, "C7 05 ?? ?? ?? ?? 00 00 00 00 8B CA", "gm_startloader", {
            GM_Configuration |= GM_CONFIG_PLAYTIME_STOP;
                    });

        MAKE_HOOK_MID(baseModule, "89 1D ?? ?? ?? ?? 89 5F", "act_loading()", {
                      GM_Configuration &= ~GM_CONFIG_PLAYTIME_STOP;
                    });

    }


}

void FixPlaytime::ApplyMG1()
{
    if (!bPauseMGTimer)
    {
        return;
    }

    //GameFrame+ED
    MAKE_HOOK_MID(mg1Module, "83 F8 ?? 74 ?? E8 ?? ?? ?? ?? 83 F8 ?? 74", "MG1: Playtime Fix : GameFrame()+0xED : exclude pause screen", {
        if (static_cast<uint32_t>(ctx.rax) == MG1_SCENE_PAUSE)
        {
            ctx.rax = MG1_SCENE_TITLE_MENU; // Only changes the timer's scene test.
        }
    });
}

void FixPlaytime::ApplyMG2()
{
    if (!bPauseMGTimer)
    {
        return;
    }

    //sub_180025A90+9D3
    MAKE_HOOK_MID(mg2Module, "83 C0 ?? 48 8B 5C 24 ?? 83 F8 ?? 77 ?? E8", "MG2: Playtime Fix : sub_180025A90()+0x9D3 : exclude pause screen", {
        if (static_cast<uint32_t>(ctx.rax) == MG2_SCENE_PAUSE)
        {
            ctx.rax = MG2_SCENE_INTRO; // Only changes the timer's scene test.
        }
    });
}
