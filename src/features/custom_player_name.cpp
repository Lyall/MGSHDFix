#include "stdafx.h"
#include "custom_player_name.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "game_stages.hpp"
#include "logging.hpp"
#include "mgs2_status_flags.hpp"

using namespace MGS2_Characters;

void CustomPlayerName::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bUseCustomName && !bUseCharacterNames)
    {
        spdlog::info("MGS 2: Custom Player Name: Config disabled. Skipping");
        return;
    }

    if (bUseCustomName)
    {
        if (sCustomName.empty())
        {
            spdlog::warn("MGS 2: Custom Player Name: Custom name is empty. Skipping");
            return;
        }
        if (sCustomName == "LIFE")
        {
            spdlog::warn("MGS 2: Custom Player Name: Custom name is default (LIFE). Skipping");
            return;
        }
        spdlog::info("MGS 2: Custom Player Name: Applying custom name '{}'", sCustomName);
        
        MAKE_HOOK_MID(baseModule, "48 8B F2 48 8B E9 FF 15 ?? ?? ?? ?? 8D 43", "GM_InitGageSet", {
            std::string nameString = reinterpret_cast<const char*>(ctx.rdx);
            if (nameString != "LIFE")
            {
                return;
            }
            ctx.rdx = reinterpret_cast<uintptr_t>(sCustomName.c_str());
        })
        return;
    }

    spdlog::info("MGS 2: Custom Player Name: Applying story name.");

    MAKE_HOOK_MID(baseModule, "48 8B F2 48 8B E9 FF 15 ?? ?? ?? ?? 8D 43", "GM_InitGageSet", {
        std::string nameString = reinterpret_cast<const char*>(ctx.rdx);
        if (nameString != "LIFE")
        {
            return;
        }
        //spdlog::info("GM_SaveResidentDir: {}", MGS2_LinkVarBuf::GM_SaveResidentDir.get());

        if (IsCurrentResident("r_plt0") && g_GameVars.IsStage(MGS2Stages::W61A))
        {
            ctx.rdx = reinterpret_cast<uintptr_t>("JACK");
            return;
        }
        ctx.rdx = reinterpret_cast<uintptr_t>(GetCurrentCharacterName().data());
        });
}
