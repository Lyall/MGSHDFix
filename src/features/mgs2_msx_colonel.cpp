#include "stdafx.h"
#include "mgs2_msx_colonel.hpp"
#include "common.hpp"
#include "expand_bp_assets.hpp"
#include "logging.hpp"

namespace {
    constexpr unsigned int STRCODE_ROYMGS = 0x5c68db; // MGS (PlayStation)
    constexpr unsigned int STRCODE_ROYMSX = 0x3914b1; // MG2:SS (MSX2)
    constexpr unsigned int STRCODE_ROYGBC = 0x8fc20d; // MGS/GB (GameBoy)
    constexpr unsigned int STRCODE_ROYSUB = 0xd8bf7e; // MG2:SS (PlayStation/etc)

    int faceIdList[3] = { STRCODE_ROYMGS, STRCODE_ROYMSX, STRCODE_ROYGBC };
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

#define EU_JP(X) (exists(sExePath / "eu" / X) && exists(sExePath / "jp" / X))

#define TEXTURE_EU_JP(X) (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / X) \
                          && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_jp" / X))

    if (BP_FilesysChanges::bLoaded && //linux filesystems are stupid and shit reliant on expand_bp_assets can randomly cause crashing.
        g_MGS2RetroColonel.bUseNewSprite
        && exists(sExePath / "assets" / "tri" / "us" / "spacecore_taisa.tri")
        && TEXTURE_EU_JP("_win" / "spacecore_taisa_subsist_alp_ovl.bmp.ctxr")
        && EU_JP("face" / "f01e" / "manifest_cbfc_spacecore_msx_subsis.txt")
        && EU_JP("face" / "f01f" / "manifest_cbfc_spacecore_msx_subsis.txt")
        && EU_JP("face" / "f01e" / "bp_assets_cbfc_spacecore_msx_subsis.txt")
        && EU_JP("face" / "f01f" / "bp_assets_cbfc_spacecore_msx_subsis.txt"))
    {
        // Swap sprite and coordinates
        faceIdList[1] = STRCODE_ROYSUB;
        // Coordinates based on GB sprite since they're practically the same art
        camPosList[1][0] = 601793;
        camPosList[1][1] = 7250;
        camPosList[1][2] = 262; // Positive heading to face left, flipping the GB sprite's -262
        camPosList[1][3] = -14421; // Pan also mirrored
        camPosList[1][4] = 6;
        camPosList[1][5] = 10796;
    }
    else if (BP_FilesysChanges::bLoaded && //linux filesystems are stupid and shit reliant on expand_bp_assets can randomly cause crashing.
             g_MGS2RetroColonel.bUseNewSprite)
    {
        spdlog::warn("MGS2: MG2 Colonel Sprite: MGS2 Community Bugfix Compilation files missing. Using original sprite and position.");
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
