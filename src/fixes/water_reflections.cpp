#include "common.hpp"
#include "water_reflections.hpp"
#include "logging.hpp"


// cipherxof's Water Surface Rendering Fix
bool MGS3_UseAdjustedOffsetY = true;
SafetyHookInline MGS3_RenderWaterSurface_hook {};
SafetyHookInline MGS3_GetViewportCameraOffsetY_hook {};

int64_t __fastcall MGS3_RenderWaterSurface(int64_t work)
{
    MGS3_UseAdjustedOffsetY = false;
    auto result = MGS3_RenderWaterSurface_hook.fastcall<int64_t>(work);
    MGS3_UseAdjustedOffsetY = true;
    return result;
}

float MGS3_GetViewportCameraOffsetY()
{
    return MGS3_UseAdjustedOffsetY ? MGS3_GetViewportCameraOffsetY_hook.stdcall<float>() : 0.00f;
}

// cipherxof's Skybox Rendering Fix
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

        uint8_t* MGS3_GetViewportCameraOffsetYScanResult = Memory::PatternScanSilent(baseModule, "E8 ?? ?? ?? ?? F3 44 ?? ?? ?? E8 ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? 00 00");
        uintptr_t MGS3_GetViewportCameraOffsetYScanAddress = Memory::GetAbsolute((uintptr_t)MGS3_GetViewportCameraOffsetYScanResult + 0xB);
        if (MGS3_GetViewportCameraOffsetYScanResult && MGS3_GetViewportCameraOffsetYScanAddress)
        {
            MGS3_GetViewportCameraOffsetY_hook = safetyhook::create_inline(reinterpret_cast<void*>(MGS3_GetViewportCameraOffsetYScanAddress), reinterpret_cast<void*>(MGS3_GetViewportCameraOffsetY));
            LOG_HOOK(MGS3_GetViewportCameraOffsetY_hook, "MGS 3: Get Viewport Camera Offset")
        }
        else
        {
            spdlog::error("MGS 3: Get Viewport Camera Offset: Pattern scan failed.");
        }
    }

}
