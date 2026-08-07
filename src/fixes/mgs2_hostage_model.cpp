#include "stdafx.h"

#include "mgs2_hostage_model.hpp"

#include "common.hpp"
#include "expand_bp_assets.hpp"

#include "logging.hpp"

namespace {
    constexpr unsigned int HOS_MALEA_DEF = 0x97fa2e;
    constexpr unsigned int HOS_MALEA_MID = 0x981eac;
    constexpr unsigned int HOS_MALEA_LOW = 0x981b7f;
    constexpr unsigned int SPACECORE_HOS_MALEB_DEF = 0xe4731b;
    constexpr unsigned int SPACECORE_HOS_MALEB_MID = 0xe49799;
    constexpr unsigned int SPACECORE_HOS_MALEB_LOW = 0xe4946c;
}

void HostageModel::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!BP_FilesysChanges::bLoaded) //linux filesystems are stupid and shit reliant on expand_bp_assets can randomly cause crashing.
    {
        return;
    }

    // Depends on custom models (provided by community bugfix pack)
#define EU_JP(X) (exists(sExePath / "eu" / X) && exists(sExePath / "jp" / X))

#define TEXTURE_EU_JP(X) (exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / X) \
                          && exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_jp" / X))
    
    if (!exists(sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_def.kms")
        || !exists(sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_mid.kms")
        || !exists(sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_low.kms")
        || !exists(sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_def.cmdl")
        || !exists(sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_mid.cmdl")
        || !exists(sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_low.cmdl")
        || !exists(sExePath / "assets" / "tri" / "us" / "spacecore_hos_all_def.tri")
        || !TEXTURE_EU_JP("_win" / "spacecore_hos_arm02.bmp.ctxr")
        || !EU_JP("stage" / "a24c" / "manifest_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "d036p03" / "manifest_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "w24c" / "manifest_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "w24e" / "manifest_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "a24c" / "bp_assets_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "d036p03" / "bp_assets_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "w24c" / "bp_assets_cbfc_hostage_fixes.txt")
        || !EU_JP("stage" / "w24e" / "bp_assets_cbfc_hostage_fixes.txt"))
    {
        spdlog::warn("Missing one or more assets for hostage hand color fix. Do you have the latest version of the Community Bugfix Pack?");
        return;
    }

    spdlog::info("MGS 2: Hostage Model Swap: Fixing hostage models.");
    // There are three hostage functions (LODs), all basically identical. They're also basically identical to two other head swap functions.
    // We can filter by reading which model is used at runtime, but we do need to hook five times instead of three to be sure.
#define REP5(X) X X X X X
    REP5({
        MAKE_HOOK_MID(baseModule, "8B D0 89 6C 24 20", "Hostage Hands", {
            // We only want to swap the model for IDs 9 and 10 (the black hostages)
            if (ctx.rsi == 9 || ctx.rsi == 10) {
                if (ctx.rax == HOS_MALEA_DEF) {
                    ctx.rax = SPACECORE_HOS_MALEB_DEF;
                }
                else if (ctx.rax == HOS_MALEA_MID) {
                    ctx.rax = SPACECORE_HOS_MALEB_MID;
                }
                else if (ctx.rax == HOS_MALEA_LOW) {
                    ctx.rax = SPACECORE_HOS_MALEB_LOW;
                }
            }
        });
    });
}
