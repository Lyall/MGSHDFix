#include "stdafx.h"
#include "mgs3_crossfade_capture.hpp"

#include "common.hpp"
#include "logging.hpp"

// The crossfade captures the scene through the renderer's store path, and its teardown can
// present one ungraded frame ~3 presents after the actor dies. Arm a short window there so
// the double-render check in HoldPreviousFrame only ever runs where the race lives.
namespace
{
    SafetyHookInline dieHook {};
    std::atomic<int> g_releaseWindow = 0;

    void __fastcall Die_Detour(void* work)
    {
        g_releaseWindow.store(8, std::memory_order_relaxed);
        dieHook.fastcall<void>(work);
    }
}

bool MGS3_CrossfadeCapture::ReleaseWindowTick()
{
    int v = g_releaseWindow.load(std::memory_order_relaxed);
    while (v > 0 && !g_releaseWindow.compare_exchange_weak(v, v - 1, std::memory_order_relaxed))
    {
    }
    return v > 0;
}

void MGS3_CrossfadeCapture::Initialize()
{
    uint8_t* dieAddr = Memory::PatternScan(baseModule,
        "48 8B C4 48 89 58 ?? 48 89 70 ?? 57 48 83 EC ?? 33 F6",
        "MGS 3: Crossfade : user\\takabe\\effect\\crosfade.c -> NewCrossFadeEffect() -> Die()");
    if (dieAddr)
    {
        dieHook = safetyhook::create_inline(dieAddr, reinterpret_cast<void*>(Die_Detour));
        LOG_HOOK(dieHook, "MGS 3: Crossfade : user\\takabe\\effect\\crosfade.c -> Die()");
    }
}
