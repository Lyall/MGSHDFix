#include "stdafx.h"
#include "mg1_guard_spawning.hpp"

#include "common.hpp"

void MG1_GuardSpawning::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //SpawnReinforcement+11E
    uint8_t* pSpawnPointCount = Memory::PatternScan(mg1Module, "8D 4B ?? E8 ?? ?? ?? ?? 8B D8 03 DB", "MG1: Guard Spawning : SpawnReinforcement()+0x11E : spawn point count");
    if (!pSpawnPointCount)
    {
        spdlog::error("MG1: Guard Spawning : Missing spawn point count. Skipping fix.");
        return;
    }
    constexpr uint8_t nOriginalCountAdjustment = 0xFF;
    constexpr uint8_t nCountAdjustment = 0;
    auto* pCountAdjustment = pSpawnPointCount + 2;
    if (*pCountAdjustment != nOriginalCountAdjustment)
    {
        spdlog::error("MG1: Guard Spawning : Unexpected count adjustment {}. Skipping fix.", *pCountAdjustment);
        return;
    }
    Memory::Write(reinterpret_cast<uintptr_t>(pCountAdjustment), nCountAdjustment);
    spdlog::info("MG1: Guard Spawning : Fix {}.", (*pCountAdjustment == nCountAdjustment) ? "successful" : "failed");
}
