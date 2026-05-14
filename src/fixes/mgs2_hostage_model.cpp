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
    if (!exists((sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_def.kms"))
        || !exists((sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_mid.kms"))
        || !exists((sExePath / "assets" / "kms" / "us" / "spacecore_hos_maleb_low.kms"))
        || !exists((sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_def.cmdl"))
        || !exists((sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_mid.cmdl"))
        || !exists((sExePath / "assets" / "kms" / "us" / "_win" / "spacecore_hos_maleb_low.cmdl"))
        || !exists((sExePath / "textures" / "flatlist" / "ovr_stm" / "ovr_eu" / "_win" / "spacecore_hos_arm02.bmp.ctxr"))) {
        //spdlog::warn("Missing one or more assets for hostage hand color fix. Do you have the latest version of the Community Bugfix Pack?");
        return;
    }
    // TODO: Hook hostage generation, change model ID contingent on type variable being 9 or 10 (should correspond to dark-skinned heads)
}
