#include "stdafx.h"
#include "mg1_scorpions.hpp"

#include "common.hpp"

void MG1_Scorpions::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //TickScorpion+135
    uint8_t* pDirectionCount = Memory::PatternScan(mg1Module, "B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 63 C8", "MG1: Scorpions pathfinding: TickScorpion()+0x135 : pathfinding directions");
    if (!pDirectionCount)
    {
        spdlog::error("MG1: Scorpions : Missing direction count. Skipping pathfinding fix.");
        return;
    }
    constexpr uint32_t nOriginalDirectionCount = 3;
    constexpr uint32_t nDirectionCount = 4;
    auto* pCount = reinterpret_cast<uint32_t*>(pDirectionCount + 1);
    if (*pCount != nOriginalDirectionCount)
    {
        spdlog::error("MG1: Scorpions : Unexpected direction count {}. Skipping pathfinding fix.", *pCount);
        return;
    }
    Memory::Write(reinterpret_cast<uintptr_t>(pCount), nDirectionCount);
    spdlog::info("MG1: Scorpions : Pathfinding fix {}.", (*pCount == nDirectionCount) ? "successful" : "failed");
}
