#include "stdafx.h"
#include "mg2_pitfalls.hpp"

#include "mg2_room_patches.hpp"
#include "mg2_pitfall_sprites.hpp"

void MG2_Pitfalls::Initialize()
{
    if (!bEnabled)
    {
        return;
    }
    for (const auto& stPitfall : MG2_PitfallSprites::astPitfalls)
    {
        MG2_RoomPatches::AddPitfall(stPitfall);
    }
}
