#include "stdafx.h"

#include "common.hpp"

#include "mgs2_ntsc_timing_fixes.hpp"

#include "game_funcs.hpp"
#include "game_stages.hpp"
#include "logging.hpp"

    // mc's gcx are compiled using PAL region defines, which have hard-coded frame delays for PAL's 50hz timings, despite the fact MC always runs at 60hz....
    // some sloppy intern shit, ain't it.

namespace
{
    bool bNTSC_w13a_paddemo_00_Present = false;
    bool bNTSC_w23a_paddemo_n00_Present = false;

    // GV_StrCode() over the EUC-JP encoded proc names. (make sure you're not using utf-8 strings if you add more.)
    constexpr uint32_t STRCODE_PADDEMO_END_DELAY = 0xC1E75B;   // "パッドデモ終了ディレイ"
    constexpr uint32_t STRCODE_CYPHER_START_DELAY = 0xF53234;  // "サイファースタートディレイ"
    constexpr uint32_t STRCODE_CAMERA_SHAKE_TIME = 0xC8ED63;   // "カメラゆれタイム"
    constexpr uint32_t STRCODE_DELAY_GENERIC = 0x8BA058;       // "ディレイ"

    int NtscTicksFor(uint32_t nameHash, int currentTicks)
    {
        //spdlog::info("MGS 2: NTSC Timing Fixes: NtscTicksFor called with nameHash {} and ticks {}", nameHash, currentTicks);
        const char* stage = Shared_Gamefuncs::GM_GetArea();
        if (nameHash == STRCODE_PADDEMO_END_DELAY)
        {
            //spdlog::info("MGS 2: NTSC Timing Fixes: Checking for パッドデモ終了ディレイ in stage {} with ticks {}", stage, currentTicks);
            if (bNTSC_w13a_paddemo_00_Present && (!strcmp(stage, MGS2Stages::W13A) || !strcmp(stage, MGS2Stages::W13B)))
            {
                //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W13A/W13B パッドデモ終了ディレイ from 635 to 723 ticks");
                return 723;
            }
            if (bNTSC_w23a_paddemo_n00_Present && (!strcmp(stage, MGS2Stages::W23A) || !strcmp(stage, MGS2Stages::W23B)))
            {
                //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W23A/W23B パッドデモ終了ディレイ from 770 to 720 ticks");
                return 720;
            }
        }
        else if (bNTSC_w23a_paddemo_n00_Present && nameHash == STRCODE_CYPHER_START_DELAY && (!strcmp(stage, MGS2Stages::W23A) || !strcmp(stage, MGS2Stages::W23B)))
        {
            //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W23A/W23B サイファースタートディレイ from 200 to 240 ticks");
            return 240;
        }
        else if (nameHash == STRCODE_CAMERA_SHAKE_TIME && (!strcmp(stage, MGS2Stages::W00A) || !strcmp(stage, MGS2Stages::A00A)))
        {
            //spdlog::info("MGS 2: NTSC Timing Fixes: Checking for W00A/A00A カメラゆれタイム in stage {} with ticks {}", stage, currentTicks);
            if (currentTicks == Shared_Gamefuncs::BP_AdjustTick(21))
            {
                //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W00A/A00A カメラゆれタイム from 21 to 20 ticks");
                return 20;
            }
        }
        else if (nameHash == STRCODE_DELAY_GENERIC && !strcmp(stage, MGS2Stages::W12B))
        {
            // w12b already plays its NTSC rpd - only the .gcx's delays are wrong.
            // w12b_sdemo.h fires 5 ディレイ, we want to hit 2, 3, and 4


            //spdlog::info("MGS 2: NTSC Timing Fixes: W12B ディレイ  : {}", currentTicks);
            if (currentTicks == Shared_Gamefuncs::BP_AdjustTick(140))
            {
                //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W12B ディレイ from 140 to 130 ticks");
                return 130;
            }
            if (currentTicks == Shared_Gamefuncs::BP_AdjustTick(420))
            {
                //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W12B ディレイ from 420 to 400 ticks");
                return 400;
            }
            if (currentTicks == Shared_Gamefuncs::BP_AdjustTick(780))
            {
                //spdlog::info("MGS 2: NTSC Timing Fixes: Fixing W12B ディレイ from 780 to 710 ticks");
                return 710;
            }
        }
        return 0;
    }


    bool CheckNTSCFile(const std::filesystem::path& path, std::string_view expectedSHA1)
    {
        spdlog::info("MGS 2: NTSC Timing Fixes: Checking for file: {}", path.string());
        return std::filesystem::exists(path) && Util::SHA1Check(path, expectedSHA1);
    }

}

void MGS2_NTSCTimingFixes::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }


    if (!Shared_Gamefuncs::BP_AdjustTick || !Shared_Gamefuncs::GM_GetArea)
    {
        spdlog::error("MGS 2: NTSC Timing Fixes: Shared_Gamefuncs::BP_AdjustTick or Shared_Gamefuncs::GM_GetArea is null, cannot apply timing fixes.");
        return;
    }

    spdlog::info("MGS 2: NTSC Timing Fixes: Applying EU .GCX -> NTSC timing fixes.");

    auto w13aFuture = std::async(std::launch::async, CheckNTSCFile, sExePath / "assets" / "row" / "eu" / "00ca766e.row", "f03496720bab8acf405611764f1c10b0230f44dc");   // ntsc = w13a_paddemo_00.row  | pal = w13a_paddemo_00_pal.row
    auto w23aFuture = std::async(std::launch::async, CheckNTSCFile, sExePath / "assets" / "row" / "eu" / "005a9862.row", "3dfd7c949ba46cce542a330a1ba6a154cfbd58d1");  // ntsc = w23a_paddemo_n00.row | pal = w23a_paddemo_n00_pal.row

    bNTSC_w13a_paddemo_00_Present = w13aFuture.get();
    bNTSC_w23a_paddemo_n00_Present = w23aFuture.get();

    if (!bNTSC_w13a_paddemo_00_Present || !bNTSC_w23a_paddemo_n00_Present)
    {
        spdlog::info("MGS 2: NTSC Timing Fixes: Some MGS2 Community Bugfix Compilation files were not detected, some timing fixes will be skipped.");
        spdlog::info("MGS 2: NTSC Timing Fixes: If you want to apply all timing fixes, please download the latest MGS2 Community Bugfix Compilation from: https://www.nexusmods.com/metalgearsolid2mc/mods/52");
    }
    else
    {
        spdlog::info("MGS 2: NTSC Timing Fixes: MGS2 Community Bugfix Compilation fixed paddemos (.row) are installed. Applying additional timing fixes.");
    }


    //_NewDelay+43  | loc_5C48A3: edi=time (post-tick-adjust), r15d=name
    MAKE_HOOK_MID(baseModule, "B1 ?? E8 ?? ?? ?? ?? 48 85 C0 74 ?? E8 ?? ?? ?? ?? 48 8B E8 B1 ?? E8 ?? ?? ?? ?? 41 BC", "MGS 2: NTSC Timing Fixes: user\\uehara\\util\\delay.c -> NewDelay() | @l147: ", {
            const int ntscTicks = NtscTicksFor(static_cast<uint32_t>(ctx.r15), static_cast<int>(ctx.rdi));
            if (ntscTicks != 0)
            {
                ctx.rdi = static_cast<uint32_t>(Shared_Gamefuncs::BP_AdjustTick(ntscTicks));
            }
                  });

}
