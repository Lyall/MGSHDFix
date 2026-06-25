#include "stdafx.h"

#include "mgs2_restore_dogtag_viewer.hpp"
#include "common.hpp"
#include "logging.hpp"

namespace
{
    
    int month {}, day {}, bloodtype {};


}


void MGS2_RestoreDogtagViewer::Restore()
{
    if (!(eGameType & MGS2))
    {
        return;
    }



    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? B1", "dogtag capture month (2002)", {
            month = (int)ctx.rax;
            //spdlog::info("captured month: {}", month);
                  });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? B1", "dogtag capture day (2002)", {
            day = (int)ctx.rax;
                  });

    MAKE_HOOK_MID(baseModule, "B1 ?? E9 ?? ?? ?? ?? E8", "dogtag capture blood (2002)", {
            bloodtype = (int)ctx.rax;
                  });


    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? E8", "dogtag capture month (2001)", {
            month = (int)ctx.rax;
            //spdlog::info("captured month: {}", month);
                  });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? E8", "dogtag capture day (2001)", {
            day = (int)ctx.rax;
                  });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? ?? 81 E9", "dogtag capture blood (2001)", {
            bloodtype = (int)ctx.rax;
                  });

    MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 44 89 44 24", "dogtag restoration month/day/blood", {
        //spdlog::info("month: {}, day: {}, bloodtype: {}", month, day, bloodtype);
        *reinterpret_cast<int*>(ctx.rsp + 0x28) = month;
        *reinterpret_cast<int*>(ctx.rsp + 0x30) = day;
        *reinterpret_cast<int*>(ctx.rsp + 0x38) = bloodtype;
                     });

    /*
    MAKE_HOOK_MID(baseModule, "48 8D 4C 24 ?? E8 ?? ?? ?? ?? 0F B6 6E ?? EB ?? 80 7E", "dogtag sprintf", {
            ctx.rdx = reinterpret_cast<uintptr_t>(&"%02d/%02d");
                  });
                  */

    MAKE_HOOK_MID(baseModule, "C6 43 ?? ?? EB ?? E8", "Node Staff Names", {
            auto* node = reinterpret_cast<char*>(ctx.rbx);
            auto* pWork = reinterpret_cast<char*>(ctx.r15);

            *reinterpret_cast<int*>(pWork + 0x114) = *reinterpret_cast<int*>(node + 0x38); // sex
            *reinterpret_cast<int*>(pWork + 0x148) = *reinterpret_cast<int*>(node + 0x3C); // year
            *reinterpret_cast<int*>(pWork + 0x14C) = *reinterpret_cast<int*>(node + 0x40); // month
            *reinterpret_cast<int*>(pWork + 0x150) = *reinterpret_cast<int*>(node + 0x44); // day
            *reinterpret_cast<int*>(pWork + 0x1C0) = *reinterpret_cast<int*>(node + 0x48); // blood
            *reinterpret_cast<int*>(pWork + 0x1E8) = *reinterpret_cast<int*>(node + 0x4C); // nation
                  });


}
