#include "stdafx.h"
#include "mgs2_hostage_type_easter_egg.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "gamevars.hpp"

namespace
{
    constexpr uint32_t HostageJenniferStrCode = GameVars::GV_StrCode("hosf_e");
    constexpr uint32_t HostageCathyStrCode = GameVars::GV_StrCode("hosf_d");
    constexpr uint32_t HostageKatochanStrCode = GameVars::GV_StrCode("hos_b");
}

void MGS2_Hostage_Type_Easter_Egg::Force()
{

    if (!(eGameType & MGS2))
    {
        return;
    }

    if (hostageMode == RTC_NORMAL)
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "33 D2 89 86 ?? ?? ?? ?? B1 ?? E8 ?? ?? ?? ?? 89 86 ?? ?? ?? ?? 4C 8D BE", "hostage rtc override", {
            if (g_GameVars.MGS2_GetGameMode() != MGS2GameMode::Plant)
            {
                return;
            }


            auto& model_num = *reinterpret_cast<int*>(ctx.rsi + 0x1344);
            auto& status = *reinterpret_cast<int*>(ctx.rsi + 0x1368);

            if (model_num == 0) // HSTG_STATUS_RIC - skip replacing Ames
            {
                //spdlog::info("you must be ames");
                return;
            }

            const int lod = status & 0x20;

            switch (hostageMode)
            {
            case RTC_JENNIFER:
                model_num = 11;
                status = 0x00000010 | 0x00008000 | lod;
                ctx.rbp = HostageJenniferStrCode;
                break;
            case RTC_CATHY:
                model_num = 12;
                status = 0x00000008 | 0x00000020;
                ctx.rbp = HostageCathyStrCode;
                break;
            case RTC_KATOCHAN:
                model_num = 3;
                status = 0x00000400 | 0x00000020;
                ctx.rbp = HostageKatochanStrCode;
                break;
            }
            });

}
