#include "stdafx.h"
#include "mgs2_body_splash_guard.hpp"

#include "common.hpp"
#include "logging.hpp"

namespace
{
    // 22 joints of {n_vertex 0, null verts/norms} - every downstream walk no-ops.
    alignas(16) uint8_t g_nullModelTable[0x18 + 22 * 0x60] = {};
    SafetyHookMid g_tableGuardHook{};
}

void MGS2BodySplashGuard::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    // Can crash if low_poly variant launched from menu.
    if (uint8_t* address = Memory::PatternScan(
            baseModule,
            "8B AF A4 B0 00 00 83 ED 01 0F 88 ?? ?? ?? ?? 48",
            "MGS 2: Body Splash Guard: okajima\\effect2\\drop_body_splush.c -> GetResources()"))
    {
        g_tableGuardHook = safetyhook::create_mid(address, [](SafetyHookContext& ctx)
        {
            if (ctx.rax < 0x10000)
            {
                ctx.rax = reinterpret_cast<uintptr_t>(g_nullModelTable);
            }
        });
        LOG_HOOK(g_tableGuardHook, "MGS 2: Body Splash Guard: okajima\\effect2\\drop_body_splush.c -> GetResources()");
    }
}
