#include "common.hpp"
#include "mg1_custom_loading_screens.hpp"

#include "config.hpp"
#include "logging.hpp"

void MG1CustomLoadingScreens::Initialize() const
{

    /* config for once this is completed.
[MG1 Custom Loading Screens]
; Adds support for 1080p/1440p/4k loading screens to MG1/MG2 while using a custom Output Resolution.
; Place custom loading screens in "MG and MG2\Misc\loading\_win" 
; load_fh.ctxr, load_wq.ctxr, load_4k.ctxr, load_fh_jp.ctxr, load_wq_jp.ctxr, load_4k_jp.ctxr
Enabled = true
    */

    //todo: This all functions, but the loading screen isn't clamped to the corner of the window properly.
    //Need to figure out the scaling control, which will lead to more bugfixes (such as the GW colonel's MSX/Gameboy sprites being sized incorrectly.)
    if (!(eGameType & MG))
    {
        spdlog::info("MG1/2: Custom Loading Screens: Unsupported game, skipping");

        return;
    }
    if (!isEnabled)
    {
        spdlog::info("MG1/2: Custom Loading Screens: Disabled in config.");

        return;
    }
    if (iOutputResY < 1080)
    {
        spdlog::info("MG1/2: Custom Loading Screens: Resolution below 1080p, skipping.");
        return;
    }

    //lea rdx, &"$/misc/loading/****/loading.ctxr"
    if (uint8_t* loadingScreenEnglish = Memory::PatternScan(baseModule, "48 8D 48 ?? 48 89 70", "MG1/2: Custom Loading Screens: English"))
    {
        static SafetyHookMid MG1CustomLoadingScreenEnglishMidHook {};
        MG1CustomLoadingScreenEnglishMidHook = safetyhook::create_mid(loadingScreenEnglish,
            [](SafetyHookContext& ctx)
            {
                ctx.rdx = iOutputResY >= 2160 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_4k.ctxr") :
                    iOutputResY >= 1440 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_wqhd.ctxr") :
                    /*iOutputResY >= 1080*/ reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_fhd.ctxr");
            });
        LOG_HOOK(MG1CustomLoadingScreenEnglishMidHook, "MG1/2: Custom Loading Screens: English")
    }

    //lea rdx, &"$/misc/loading/****/loading_jp.ctxr"
    if (uint8_t* loadingScreenJapanese = Memory::PatternScan(baseModule, "FF 15 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 4C 8D 44 24 ?? 48 8D 54 24 ?? 48 8B 08 48 8D 44 24", "MG1 Custom Loading Screen: Japanese"))
    {
        static SafetyHookMid MG1CustomLoadingScreenJapaneseMidHook {};
        MG1CustomLoadingScreenJapaneseMidHook = safetyhook::create_mid(loadingScreenJapanese,
            [](SafetyHookContext& ctx)
            {
                ctx.rdx = iOutputResY >= 2160 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_4k.ctxr") :
                    iOutputResY >= 1440 ? reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_wqhd.ctxr") :
                    /*iOutputResY >= 1080*/ reinterpret_cast<uintptr_t>(&"$/misc/loading/****/loading_jp_fhd.ctxr");
            });
        LOG_HOOK(MG1CustomLoadingScreenJapaneseMidHook, "MG1/2: Custom Loading Screens: Japanese")
    }

}
