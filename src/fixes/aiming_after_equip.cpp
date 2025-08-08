#include "common.hpp"
#include "aiming_after_equip.hpp"

#include "gamevars.hpp"
#include "logging.hpp"


void FixAimAfterEquip::Initialize()
{
    if (!(eGameType & (MGS2 | MGS3)) || !g_FixAimAfterEquip.bEnabled)
    {
        return;
    }

    if (eGameType & MGS2)
    {
        MAKE_HOOK_MID(baseModule, "89 1D ?? ?? ?? ?? 8B C3 48 83 C4", "MGS2: Aiming After Equip - Snake", {
            g_GameVars.SetAimingState(0);
        });

        MAKE_HOOK_MID(baseModule, "89 B3 ?? ?? ?? ?? 48 8B 83 ?? ?? ?? ?? 4C 8B 74 24, "MGS2: Aiming After Equip - Raiden", {
            if ((uint32_t)ctx.rsi == 0)
            {
             g_GameVars.SetAimingState(0);
           }
        });

        }
    
    else if (eGameType & MGS3)
    {
        MAKE_HOOK_MID(baseModule, "44 89 0D ?? ?? ?? ?? EB ?? 48 8D 05", "MGS3: Aiming After Equip", {
            if (ctx.r9 == 0)
            {
                g_GameVars.SetAimingState(0);
            }
            });
    }
}


