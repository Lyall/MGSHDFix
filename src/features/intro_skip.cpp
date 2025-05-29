#include "common.hpp"
#include "spdlog/spdlog.h"
#include "intro_skip.hpp"

void IntroSkip::Initialize() const
{
    if (!isEnabled)
        return;
    if (!(eGameType & (MG | MGS2 | MGS3)))
        return;

    uint8_t* MGS2_MGS3_InitialIntroStateScanResult = Memory::PatternScanSilent(baseModule, "75 ? C7 05 ? ? ? ? 01 00 00 00 C3");
    if (!MGS2_MGS3_InitialIntroStateScanResult)
    {
        spdlog::error("MGS 2 | MGS 3: Skip Intro Logos: Pattern scan failed.");
        return;
    }

    uint32_t* MGS2_MGS3_InitialIntroStatePtr = (uint32_t*)(MGS2_MGS3_InitialIntroStateScanResult + 8);
    spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Initial state: {}", *MGS2_MGS3_InitialIntroStatePtr);

    uint32_t NewState = 3;
    Memory::PatchBytes((uintptr_t)MGS2_MGS3_InitialIntroStatePtr, (const char*)&NewState, sizeof(NewState));
    spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Patched state: {}", *MGS2_MGS3_InitialIntroStatePtr);
}
