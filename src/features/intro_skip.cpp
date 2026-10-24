#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"
#include "intro_skip.hpp"

#include "config.hpp"

void IntroSkip::Initialize() const
{
    if (isEnabled && (eGameType & LAUNCHER))
    {
        if (!bLauncherJumpStart)
        {
            spdlog::info("Launcher: Skip Intro Logos: Use the Launcher Jumpstart configuration option to skip the launcher's intro logos!");
        }
        return;
    }
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }
    if (!isEnabled)
    {
        spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Config disabled. Skipping");
        return;
    }

    if (uint8_t* MGS2_MGS3_InitialIntroStateScanResult = Memory::PatternScan(baseModule, "75 ?? C7 05 ?? ?? ?? ?? 01 00 00 00 C3", "Skip Intro Logos"))
    {
        uint32_t* MGS2_MGS3_InitialIntroStatePtr = (uint32_t*)(MGS2_MGS3_InitialIntroStateScanResult + 8);
        uint32_t NewState = 3;
        Memory::PatchBytes((uintptr_t)MGS2_MGS3_InitialIntroStatePtr, (const char*)&NewState, sizeof(NewState));
        spdlog::info("MGS 2 | MGS 3: Intro logos patched.");
    }

}

namespace
{

    void SkipBootLogos(uint8_t* pFirstLogo, const char* szLabel)
    {
        constexpr uint8_t anOriginal[] = { 0xFF, 0xC2 };
        constexpr uint8_t anSkip[] = { 0xB2, 0x04 };
        if (!pFirstLogo)
        {
            spdlog::error("{}: Skip Intro Logos : Missing boot logo sequence. Skipping.", szLabel);
            return;
        }
        if (memcmp(pFirstLogo, anOriginal, sizeof(anOriginal)) != 0)
        {
            spdlog::error("{}: Skip Intro Logos : Unexpected boot logo bytes. Skipping.", szLabel);
            return;
        }
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(pFirstLogo), reinterpret_cast<const char*>(anSkip), sizeof(anSkip));
        spdlog::info("{}: Skip Intro Logos : Boot logos patched.", szLabel);
    }
}

void IntroSkip::InitializeMG1() const
{
    if (!isEnabled)
    {
        return;
    }
    //TickSplash+39
    SkipBootLogos(Memory::PatternScan(mg1Module, "FF C2 89 15 ?? ?? ?? ?? 83 FA", "MG1: Skip Intro Logos : TickSplash()+0x39 : first logo index"), "MG1");
}

void IntroSkip::InitializeMG2() const
{
    if (!isEnabled)
    {
        return;
    }
    //TickSplash+39
    SkipBootLogos(Memory::PatternScan(mg2Module, "FF C2 89 15 ?? ?? ?? ?? 83 FA ?? 75", "MG2: Skip Intro Logos : TickSplash()+0x39 : first logo index"), "MG2");
}

/*
void IntroSkip::Initialize() const
{
    if (isEnabled && (eGameType & LAUNCHER))
    {
        spdlog::info("Launcher: Skip Intro Logos: Use the Launcher Jumpstart configuration option to skip the launcher's intro logos!");
        return;
    }

    if (!(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    if (!isEnabled)
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Skip Intro Logos: Config disabled. Skipping");
        return;
    }



    if (eGameType & MG)
    {
        if (uint8_t* MG1_InitialIntroStateScanResult = Memory::PatternScan(baseModule, "FF 15 ?? ?? ?? ?? C7 05 ?? ?? ?? ?? ?? ?? ?? ?? EB", "Skip Intro Logos"))
        {
            Memory::PatchBytes((uintptr_t)MG1_InitialIntroStateScanResult, "\x90\x90\x90\x90\x90\x90", 6);

        }

        /*
        if (uint8_t* MG1_InitialIntroStateScanResult = Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 89 0D ?? ?? ?? ?? 83 F8", "Skip Intro Logos"))
        {

            uint32_t* MG1_InitialIntroStatePtr = (uint32_t*)Memory::GetRelativeOffset(MG1_InitialIntroStateScanResult + 2);
            spdlog::info("MG/MG2: Skip Intro Logos: Initial state: {}", *MG1_InitialIntroStatePtr);

            uint32_t NewState = 3;
            Memory::PatchBytes((uintptr_t)MG1_InitialIntroStatePtr, (const char*)&NewState, sizeof(NewState));
            spdlog::info("MG/MG2: Skip Intro Logos: Patched state: {}", *MG1_InitialIntroStatePtr);
        }
        return;
        
    }



    if (uint8_t* MGS2_MGS3_InitialIntroStateScanResult = Memory::PatternScan(baseModule, "75 ?? C7 05 ?? ?? ?? ?? 01 00 00 00 C3", "Skip Intro Logos"))
    {
        uint32_t* MGS2_MGS3_InitialIntroStatePtr = (uint32_t*)(MGS2_MGS3_InitialIntroStateScanResult + 8);
        spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Initial state: {}", *MGS2_MGS3_InitialIntroStatePtr);

        uint32_t NewState = 3;
        Memory::PatchBytes((uintptr_t)MGS2_MGS3_InitialIntroStatePtr, (const char*)&NewState, sizeof(NewState));
        spdlog::info("MGS 2 | MGS 3: Skip Intro Logos: Patched state: {}", *MGS2_MGS3_InitialIntroStatePtr);
    }

}
*/
