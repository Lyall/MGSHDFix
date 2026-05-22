#include "stdafx.h"

#include "mgs2_difficulty.hpp"
#include "common.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

namespace
{
    //All numbers below are directly from the original GCL scripts' varinit.h file. They are 100% accurate. <3

    /*   byte patched.
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_TIME_VERY_EASY         =   600;    //HDC = 600
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_TIME_EASY              =   635;    //HDC = 650
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_TIME_NORMAL            =   900;    //HDC = 700
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_TIME_HARD              =   1200;   //HDC = 750
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_TIME_EXTREME           =   1500;   //HDC = 800
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_TIME_EUROPEAN_EXTREME  =   3000;   //HDC = 850 -> bp typo'd this as 30,000 in their description, varinit.h really says 60*50

    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_LIFE_VERY_EASY         =   200;    //HDC = 200
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_LIFE_EASY              =   120;    //HDC = 184
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_LIFE_NORMAL            =   100;    //HDC = 168
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_LIFE_HARD              =   75;     //HDC = 152
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_LIFE_EXTREME           =   50;     //HDC = 136
    constexpr int PS2_DIFFICULTY_SOLIDUS_CHOKING_LIFE_EUROPEAN_EXTREME  =   30;     //HDC = 120*/

    std::int32_t* gBP_Game_GrenadeExplodeInHand_Enable = nullptr;
    //gBP_Game_GrenadeBlast_OuterRange


}

void MGS2_RestoreOriginalDifficulty::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!(bEnableGrenadeCooking || bRestoreOriginalSolidusChokingDuration || bRestoreOriginalSolidusChokingLife))
    {
        spdlog::info("MGS2: Restore Original Difficulty: Config disabled, skipping.");
        return;
    }

    if (bEnableGrenadeCooking)
    {
        const auto grenade_c__Act_987 = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 0F 85 ?? ?? ?? ?? B9", "MGS2_RestoreOriginalDifficulty: gBP_Game_GrenadeExplodeInHand_Enable");
        if (grenade_c__Act_987 == nullptr)
        {
            spdlog::error("MGS2_RestoreOriginalDifficulty: gBP_Game_GrenadeExplodeInHand_Enable - failed to find");
            return;
        }
        gBP_Game_GrenadeExplodeInHand_Enable = reinterpret_cast<std::int32_t*>(Memory::GetRipRelativeAddress(grenade_c__Act_987, 2, 7));
        *gBP_Game_GrenadeExplodeInHand_Enable = 1;

        g_InputHandler.RegisterHotkey(vkToggleGrenadeCooking, "Toggle grenade cooking", []() {
            *gBP_Game_GrenadeExplodeInHand_Enable = !*gBP_Game_GrenadeExplodeInHand_Enable;
        });

    }

    if (bRestoreOriginalSolidusChokingLife)
    {
        if (uint8_t* MGS2_Solidus_Choking_Life = Memory::PatternScan(baseModule, "C8 00 00 00 B8 00 00 00 A8 00 00 00 98 00 00 00 88 00 00 00 78 00 00 00", "MGS2: gBP_Game_SolidusChoke_Life"))
        {
            Memory::PatchBytes((uintptr_t)MGS2_Solidus_Choking_Life, "\xC8\x00\x00\x00\x78\x00\x00\x00\x64\x00\x00\x00\x4B\x00\x00\x00\x32\x00\x00\x00\x1E\x00\x00\x00", 24);
            spdlog::info("MGS2: Restored original Solidus Choking Life values.");
        }
    }

    if (bRestoreOriginalSolidusChokingDuration)
    {
        if (uint8_t* MGS2_Solidus_Choking_Duration = Memory::PatternScan(baseModule, "58 02 00 00 8A 02 00 00 BC 02 00 00 EE 02 00 00 20 03 00 00 52 03 00 00", "MGS2: gBP_Game_SolidusChoke_Time"))
        {
            Memory::PatchBytes((uintptr_t)(MGS2_Solidus_Choking_Duration), "\x58\x02\x00\x00\x7B\x02\x00\x00\x84\x03\x00\x00\xB0\x04\x00\x00\xDC\x05\x00\x00\xB8\x0B\x00\x00", 24);
            spdlog::info("MGS2: Restored original Solidus Choking Duration values.");
        }
    }

    

}

