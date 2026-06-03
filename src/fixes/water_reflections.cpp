#include "stdafx.h"
#include "common.hpp"
#include "water_reflections.hpp"
#include "logging.hpp"


// cipherxof's Water Surface Rendering Fix
SafetyHookInline MGS3_RenderWaterSurface_hook {};

int64_t __fastcall MGS3_RenderWaterSurface(int64_t work)
{
    g_WaterReflectionFix.MGS3_UseAdjustedOffsetY = false;
    auto result = MGS3_RenderWaterSurface_hook.fastcall<int64_t>(work);
    g_WaterReflectionFix.MGS3_UseAdjustedOffsetY = true;
    return result;
}

void WaterReflectionFix::Initialize() const
{
    if (eGameType & MGS3)
    {
        uint8_t* MGS3_RenderWaterSurfaceScanResult = Memory::PatternScanSilent(baseModule, "0F 57 ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B ?? ?? ?? 48 89 ?? ?? ?? ?? ??");
        uintptr_t MGS3_RenderWaterSurfaceScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_RenderWaterSurfaceScanResult + 0x10);
        if (MGS3_RenderWaterSurfaceScanResult && MGS3_RenderWaterSurfaceScanAddress)
        {
            MGS3_RenderWaterSurface_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_RenderWaterSurfaceScanAddress), reinterpret_cast<void*>(MGS3_RenderWaterSurface));
            LOG_HOOK(MGS3_RenderWaterSurface_hook, "MGS 3: Render Water Surface")
        }
        else
        {
            spdlog::error("MGS 3:  Render Water Surface: Pattern scan failed.");
        }

    }

}
