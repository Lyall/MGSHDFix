#include "stdafx.h"

#include "common.hpp"
#include "mgs2_restore_elevator_glitch.hpp"

#include "logging.hpp"

// elv_switch = 0xB62582, pane = 0x399225
void MGS2_RestoreElevatorGlitch::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    MAKE_HOOK_MID(baseModule,
        "48 89 74 24 ?? 41 56 4C 63 1D",
        "MGS2: Restore SoL Elevator Glitch (HZX_SetBind)", {
            const uintptr_t bnd  = ctx.rcx;
            const uint32_t  name = *reinterpret_cast<uint32_t*>(bnd);
            if (name == 0xB62582 || name == 0x399225)
            {
                int8_t*   st  = reinterpret_cast<int8_t*>(bnd + 0x38);
                const int cnt = st[0];
                bool      has = false;
                for (int k = 1; k <= cnt && k < 4; ++k)
                    if (st[k] == 2) has = true;
                if (!has && cnt >= 0 && cnt < 3)
                {
                    st[cnt + 1] = 2;
                    st[0] = static_cast<int8_t>(cnt + 1);
                }
            }
        });
}
