#include "stdafx.h"
#include "mgs2_reverb_wet_level.hpp"

#include "common.hpp"
#include "logging.hpp"

void FixReverbWetLevel::Initialize()
{
    if (!(eGameType & (MGS2 | MGS3)) || !bEnabled)
    {
        return;
    }

    if (fWetVolumeScale == 1.0f)
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "FF 50 ?? 4C 8D 9C 24 ?? ?? ?? ?? 49 8B 5B ?? 41 0F 28 73", "bp\\shared\\BP_SoundSupportX360.cpp -> AudioDriver::ApplyReverbSetting() @ L1997", {
        //spdlog::info("FixReverbWetLevel: xmm1 .f32[0] = {} -> {} (wet volume scale)", ctx.xmm1.f32[0], ctx.xmm1.f32[0] * FixReverbWetLevel::fWetVolumeScale);
        ctx.xmm1.f32[0] *= FixReverbWetLevel::fWetVolumeScale;
                  });


}
