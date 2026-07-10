#include "stdafx.h"
#include "mgs2_reverb_wet_level.hpp"

#include "common.hpp"
#include "logging.hpp"

namespace
{
    SafetyHookMid gSetWetVolumeHook {};
}

void MGS2ReverbWetLevel::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    // Reverb apply: depth -> float, * (1/32767), * master scalar, then IXAudio2Voice::SetVolume
    // on the reverb submix. Scale the volume in xmm1 just before the call.
    if (uint8_t* setVolume = Memory::PatternScan(baseModule,
        "48 8B 8B A0 2E 00 00 0F 5B C9 48 8B 01 F3 0F 59 0D ?? ?? ?? ?? F3 0F 59 8B B8 2E 00 00 FF 50 60",
        "MGS2: Reverb Wet Level: reverb submix SetVolume"))
    {
        gSetWetVolumeHook = safetyhook::create_mid(setVolume + 29, [](SafetyHookContext& ctx)
        {
            ctx.xmm1.f32[0] *= MGS2ReverbWetLevel::fWetVolumeScale;
        });
        LOG_HOOK(gSetWetVolumeHook, "MGS2: Reverb Wet Level: reverb submix SetVolume")
    }
}
