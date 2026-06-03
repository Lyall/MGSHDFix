#include "stdafx.h"

#include "playtime_fixes.hpp"

#include "common.hpp"
#include "input_handler.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs2_status_flags.hpp"

namespace
{
    int* MGS3_GM_Configuration = nullptr;

    
}

void FixPlaytime::Apply()
{
    if (!(eGameType & (MGS2|MGS3)))
    {
        return;
    }

    if (eGameType & MGS2)
    {
        MAKE_HOOK_MID(baseModule, "C7 05 ?? ?? ?? ?? 00 00 00 00 48 83 C4 ?? 5F C3 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 40 53", "gm_startloader", {
            MGS2_LinkVarBuf::GM_Configuration |= GM_CONFIG_PLAYTIME_STOP;
                      });


#if !defined(RELEASE_BUILD)
        g_InputHandler.RegisterHotkey(VK_NUMPAD8, "report time", []()
                                      {
                                          spdlog::info("MGS 2: MGS2_LinkVarBuf::GM_PlayTime: {}", MGS2_LinkVarBuf::GM_PlayTime.get());
                                      });
#endif

    }
    else // eGameType & MGS3
    {
        MGS3_GM_Configuration = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "81 25 ?? ?? ?? ?? FF FD FF FF", "MGS3: GM_Configuration"), 2, 10));
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 89 43 ?? EB", "MGS3: gm_startloader", {
            if (MGS3_GM_Configuration)
            {
                *MGS3_GM_Configuration |= 0x200;
            }
            });

    }


}
