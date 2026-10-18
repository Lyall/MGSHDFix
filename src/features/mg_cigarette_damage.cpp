#include "stdafx.h"
#include "mg_cigarette_damage.hpp"

#include "common.hpp"
#include "mg2_gamevar_enums.hpp"

void MG_Cigarettes::InitializeMG1()
{
    if (!bEnabled)
    {
        return;
    }

    //TickPlayerControl+131
    uint8_t* pWarning = Memory::PatternScan(mg1Module, "81 F9 ?? ?? ?? ?? 7E ?? 41 83 7D", "MG1: Cigarettes : TickPlayerControl()+0x131 : cigarette health drain timer");
    if (!pWarning)
    {
        spdlog::error("MG1: Cigarettes : Missing cigarette health drain timer. Skipping.");
        return;
    }
    constexpr std::array<uint8_t, 6> anTimerCheck = { 0x81, 0xF9, 0x00, 0x01, 0x00, 0x00 };
    if (memcmp(pWarning, anTimerCheck.data(), anTimerCheck.size()) != 0)
    {
        spdlog::error("MG1: Cigarettes : Unexpected instructions. Skipping.");
        return;
    }
    // Reset the timer
    constexpr uint16_t nSkipWarning = 0x43EB;
    Memory::Write(reinterpret_cast<uintptr_t>(pWarning), nSkipWarning);
    if (memcmp(pWarning, &nSkipWarning, sizeof(nSkipWarning)) == 0)
    {
        spdlog::info("MG1: Cigarettes : Initialized.");
    }
    else
    {
        spdlog::error("MG1: Cigarettes : Could not disable cigarette health drain.");
    }
}

void MG_Cigarettes::InitializeMG2()
{
    if (!bEnabled)
    {
        return;
    }

    //StartScript+12
    MAKE_HOOK_MID(mg2Module, "48 85 C9 75 ?? 33 C0 48 83 C4 ?? C3", "MG2: Cigarettes : StartScript()+0x12 : missing script control path", {
        if (ctx.rax == MG2_SCRIPT_CIGARETTES)
        {
            ctx.rcx = 0; // Force the loader to return as missing
        }
    });
}
