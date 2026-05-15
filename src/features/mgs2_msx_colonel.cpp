#include "stdafx.h"
#include "mgs2_msx_colonel.hpp"
#include "common.hpp"
#include "logging.hpp"

namespace {
    int faceIdList[3] = { 0x5c68db, 0x3914b1, 0x8fc20d };
    int camPosList[3][6] = {
        { 1000000, 10565, -18, -1477, -2, 9897 }, // MGS
        { 1000000, 12890, 176, -36977, 5, -503 }, // MG2 (coordinates commented in face_bug.c)
        { 601793,  7250, -262, 14421, 6, 10796 }  // GB
    };
}

void MGS2RetroColonel::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!g_MGS2RetroColonel.bEnabled)
    {
        spdlog::info("MGS2: MG2 Colonel Sprite: Disabled via config, skipping.");
        return;
    }

    if (g_MGS2RetroColonel.bUseNewSprite
        && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "spacecore_taisa_subsist_alp_ovl.bmp.ctxr")
        && true) // TODO: Manifest verification
    {
        // Swap sprite and coordinates
        faceIdList[1] = 0xd8bf7e;
        // Coordinates based on GB sprite since they're practically the same art
        camPosList[1][0] = 601793;
        camPosList[1][1] = 7250;
        camPosList[1][2] = 262; // Positive heading to face left, flipping the GB sprite's -262
        camPosList[1][3] = -14421; // Pan also mirrored
        camPosList[1][4] = 6;
        camPosList[1][5] = 10796;
    }

    // user/mode/codec/face_bug.c -> Act() - see also resolution_scaling_fixes.cpp
    {
        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 48 8B 8F 90 00 00 00 BA 80 00 00 00", "MGS2: MG2 Colonel Sprite", {
            ctx.rbx = ((short)rand()) % 3; // Rerandomize with 3 options instead of what got optimized to rand() & 1
            ctx.rdx = faceIdList[ctx.rbx];
        });
    }
    {
        MAKE_HOOK_MID(baseModule, "8B 4F 58 0F 5B DB", "MGS2: MG2 Colonel Sprite", {
            ctx.xmm1.u32[0] = camPosList[ctx.rbx][0];
            ctx.xmm2.u32[0] = camPosList[ctx.rbx][1];
            ctx.xmm3.u32[0] = camPosList[ctx.rbx][2];
            ctx.xmm4.u32[0] = camPosList[ctx.rbx][3];
            ctx.xmm7.u32[0] = camPosList[ctx.rbx][4]; // Why did the compiler put xmm7 in here
            ctx.xmm6.u32[0] = camPosList[ctx.rbx][5];
        });
    }


}
