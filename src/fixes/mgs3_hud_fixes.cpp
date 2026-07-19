#include "stdafx.h"
#include "common.hpp"
#include "mgs3_hud_fixes.hpp"

#include "logging.hpp"

void MGS3HudFixes::Initialize()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    MAKE_HOOK_MID(baseModule, "F3 0F 59 C7 F3 0F 2C C8 66 89 48 ?? 66 3B D1 74 ?? 81 48 ?? ?? ?? ?? ?? 0F B7 48", "MGS3: NVG Cross", {
        ctx.xmm1.f32[0] += 160.0f; //y axis
        ctx.xmm0.f32[0] = ((((ctx.xmm0.f32[0] - 256.0f) * 2) + 512.0f) - 32.0f); //x axis
        });

    if (uint8_t* addr = Memory::PatternScan(baseModule, "41 B9 3F 01 00 00 C7 44 24 ?? 00 00 00 00", "MGS3: user\\skoba\\weapon\\thermal_layout.c -> NewThermalSight() -> NormalAct() -> NumberMove()"))
    {
        static SafetyHookMid hook {};
        hook = safetyhook::create_mid(addr + 0x1A, [](SafetyHookContext& ctx) {
            ctx.r8 = 377;
                                      });
        LOG_HOOK(hook, "MGS3: user\\skoba\\weapon\\thermal_layout.c -> NewThermalSight() -> NormalAct() -> NumberMove()");
    }

}
