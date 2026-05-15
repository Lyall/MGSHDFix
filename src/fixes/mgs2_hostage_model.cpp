#include "stdafx.h"

#include "mgs2_hostage_model.hpp"

#include "common.hpp"

#include "logging.hpp"

#define HOS_MALEA_DEF (0x97fa2e)
#define HOS_MALEA_MID (0x981eac)
#define HOS_MALEA_LOW (0x981b7f)
#define SPACECORE_HOS_MALEB_DEF (0xe4731b)
#define SPACECORE_HOS_MALEB_MID (0xe49799)
#define SPACECORE_HOS_MALEB_LOW (0xe4946c)


void HostageModel::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    // Depends on custom models (provided by community bugfix pack)
    // Note: no easy way to check for manifest and tri edits? Try it without checking, assume the models come with the manifest.
    if (!exists(sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_def.kms")
        || !exists(sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_mid.kms")
        || !exists(sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_low.kms")
        || !exists(sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_def.cmdl")
        || !exists(sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_mid.cmdl")
        || !exists(sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_low.cmdl")
        || !exists(sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "spacecore_hos_arm02.bmp.ctxr")) {
        //spdlog::warn("Missing one or more assets for hostage hand color fix. Do you have the latest version of the Community Bugfix Pack?");
        return;
    }

    // There are three hostage functions (LODs), all basically identical. They're also basically identical to two other head swap functions.
    // We can filter by reading which model is used at runtime, but we do need to hook five times instead of three to be sure.
    // We only want to swap the model for IDs 9 and 10 (the black hostages)
    {
        MAKE_HOOK_MID(baseModule, "8B D0 89 6C 24 20", "Hostage Hands", {
            if (ctx.rsi == 9 || ctx.rsi == 10) {
                if (ctx.rax == HOS_MALEA_DEF) {
                    ctx.rax = SPACECORE_HOS_MALEB_DEF;
                } else if (ctx.rax == HOS_MALEA_MID) {
                    ctx.rax = SPACECORE_HOS_MALEB_MID;
                } else if (ctx.rax == HOS_MALEA_LOW) {
                    ctx.rax = SPACECORE_HOS_MALEB_LOW;
                }
            }
        });
    }
    {
        MAKE_HOOK_MID(baseModule, "8B D0 89 6C 24 20", "Hostage Hands", {
            if (ctx.rsi == 9 || ctx.rsi == 10) {
                if (ctx.rax == HOS_MALEA_DEF) {
                    ctx.rax = SPACECORE_HOS_MALEB_DEF;
                } else if (ctx.rax == HOS_MALEA_MID) {
                    ctx.rax = SPACECORE_HOS_MALEB_MID;
                } else if (ctx.rax == HOS_MALEA_LOW) {
                    ctx.rax = SPACECORE_HOS_MALEB_LOW;
                }
            }
        });
    }
    {
        MAKE_HOOK_MID(baseModule, "8B D0 89 6C 24 20", "Hostage Hands", {
            if (ctx.rsi == 9 || ctx.rsi == 10) {
                if (ctx.rax == HOS_MALEA_DEF) {
                    ctx.rax = SPACECORE_HOS_MALEB_DEF;
                } else if (ctx.rax == HOS_MALEA_MID) {
                    ctx.rax = SPACECORE_HOS_MALEB_MID;
                } else if (ctx.rax == HOS_MALEA_LOW) {
                    ctx.rax = SPACECORE_HOS_MALEB_LOW;
                }
            }
        });
    }
    {
        MAKE_HOOK_MID(baseModule, "8B D0 89 6C 24 20", "Hostage Hands", {
            if (ctx.rsi == 9 || ctx.rsi == 10) {
                if (ctx.rax == HOS_MALEA_DEF) {
                    ctx.rax = SPACECORE_HOS_MALEB_DEF;
                } else if (ctx.rax == HOS_MALEA_MID) {
                    ctx.rax = SPACECORE_HOS_MALEB_MID;
                } else if (ctx.rax == HOS_MALEA_LOW) {
                    ctx.rax = SPACECORE_HOS_MALEB_LOW;
                }
            }
        });
    }
    {
        MAKE_HOOK_MID(baseModule, "8B D0 89 6C 24 20", "Hostage Hands", {
            if (ctx.rsi == 9 || ctx.rsi == 10) {
                if (ctx.rax == HOS_MALEA_DEF) {
                    ctx.rax = SPACECORE_HOS_MALEB_DEF;
                } else if (ctx.rax == HOS_MALEA_MID) {
                    ctx.rax = SPACECORE_HOS_MALEB_MID;
                } else if (ctx.rax == HOS_MALEA_LOW) {
                    ctx.rax = SPACECORE_HOS_MALEB_LOW;
                }
            }
        });
    }
}
