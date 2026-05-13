#include "stdafx.h"

#include "mgs2_coolant_mirror.hpp"

#include "common.hpp"

#include "logging.hpp"



void CoolantMirrorFix::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    // The function for creating the mist-receiving object uses incorrect offsets for breakable glass.
    // This seems to be an issue with calculating an average of two vectors, so dividing each component works.

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 89 83 68 02 00 00 33 C0", "MGS2: Coolant Mirror", {
        ((float*)ctx.rcx)[0] /= 2.0f;
        ((float*)ctx.rcx)[1] /= 2.0f;
        ((float*)ctx.rcx)[2] /= 2.0f;
    });

}
