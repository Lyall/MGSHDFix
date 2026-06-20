#include "stdafx.h"

#include "mgs2_coolant_mirror.hpp"

#include "common.hpp"
#include "gamevars.hpp"

#include "logging.hpp"

namespace
{
    constexpr uint64_t kThermalGoggleSubtractiveAlphaMode = 0x42ULL; /// SCE_GS_SET_ALPHA(2, 0, 0, 1, 0)
    constexpr uint64_t kDefaultAlphaMode = 0x44ULL;        /// SCE_GS_SET_ALPHA(0, 1, 0, 1, 0)
}

void CoolantMirrorFix::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    spdlog::info("MGS2 - Fixing coolant mirror effects.");

    // The function for creating the mist-receiving object uses incorrect offsets for breakable glass.
    // This seems to be an issue with calculating an average of two vectors, so dividing each component works.

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 89 83 68 02 00 00 33 C0", "MGS2: Coolant Mirror", {
        ((float*)ctx.rcx)[0] /= 2.0f;
        ((float*)ctx.rcx)[1] /= 2.0f;
        ((float*)ctx.rcx)[2] /= 2.0f;
    });

                      // Xbox ifdefed feature: Different alpha settings in thermal vision
    MAKE_HOOK_MID(baseModule, "44 8B 43 ?? B8", "MGS2: Coolant Mirror", {
        const auto prim = *reinterpret_cast<uintptr_t*>(ctx.rbx + 88);
        *reinterpret_cast<uint64_t*>(prim + 0x200) = (MGS2_LinkVarBuf::GM_Item == MGS2_ITEM_INDEX_THERMAL_GOGGLES) ? kThermalGoggleSubtractiveAlphaMode : kDefaultAlphaMode;
                  });


}
