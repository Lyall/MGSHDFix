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

        MAKE_HOOK_MID(baseModule, "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? 8B C3", "MGS2: Aiming After Equip - Location 1", {
            if (ctx.r8 == 0)
            {
                g_GameVars.SetAimingState(0);
            }
            });

        MAKE_HOOK_MID(baseModule, "89 1D ?? ?? ?? ?? 8B C3 48 83 C4", "MGS2: Aiming After Equip - Location 2", {
            g_GameVars.SetAimingState(0);
            });

        MAKE_HOOK_MID(baseModule, "BB ?? ?? ?? ?? 66 89 81", "MGS2: test 2", {
            spdlog::info("test 2");
            Util::DumpContext(ctx);
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
