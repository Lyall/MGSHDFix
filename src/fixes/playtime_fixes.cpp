#include "stdafx.h"

#include "playtime_fixes.hpp"

#include "common.hpp"
#include "input_handler.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs2_status_flags.hpp"
#include "mgs3_linkvarbuf.hpp"
#include "mgs3_status_flags.hpp"

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
