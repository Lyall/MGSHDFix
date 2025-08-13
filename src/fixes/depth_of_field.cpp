#include "depth_of_field.hpp"

#include "common.hpp"
#include "logging.hpp"


void DepthOfFieldFixes::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 83 C7 ?? FF C3", "MGS2: Depth of Field loc 1", {
        spdlog::info("dof loc 1");
        Util::DumpContext(ctx);
    });
    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 81 C7 ?? ?? ?? ?? FF C3 83 FB", "MGS2: Depth of Field loc 2", {
        spdlog::info("dof loc 2");
        Util::DumpContext(ctx);
        });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? BA ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 48 8B C8", "MGS2: blur 1", {
    spdlog::info("blur 1");
    Util::DumpContext(ctx);
        });


    MAKE_HOOK_MID(baseModule, "76 ?? 89 93 ?? ?? ?? ?? EB ?? 0F 2F F0", "MGS2: MGS2_Resolution_Conversion", {
    spdlog::info("MGS2_Resolution_Conversion");
    Util::DumpContext(ctx);
        });

}



