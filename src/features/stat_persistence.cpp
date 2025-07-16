#include "common.hpp"
#include "stat_persistence.hpp"

#include "logging.hpp"
#include "steamworks_api.hpp"
#include "steam_achievements.hpp"
#pragma warning(push)
#pragma warning(disable:4828)
#include <isteamuserstats.h>
#pragma warning(pop)

using BoolPtr = bool StatPersistence::*;

struct AchievementEntry
{
    const char* steamID;
    const char* friendlyName;
    BoolPtr boolPtr;
};

///Contains all achievements with stats that need to be tracked for persistence.
///All other achievements are unlocked at the time of event, so no need to handle them.
///if you add more, don't forget to update the size of the array 
static constexpr std::array<AchievementEntry, 6> MGS2_Achvmt_Map = { {
    {MGS2_Achvmt_ByeByeBigBrother, "Bye Bye Big Brother", &StatPersistence::bMGS2_Achvmt_ByeByeBigBrother_Unlocked},  // Destroy 5 cameras //89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 8B 05 //eax
    {MGS2_Achvmt_JohnnyOnTheSpot, "Johnny on the Spot", &StatPersistence::bMGS2_Achvmt_JohnnyOnTheSpot_Unlocked},    // Hear Johnny's bowel noises in two locations //83 3D ?? ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 74 // eax // C7 05 ?? ?? ?? ?? ?? ?? ?? ?? 7E -> lets cache both instances as their own vars (or just always directly control the pointer)
    {MGS2_Achvmt_HoldUpAholic, "Hold Up-aholic", &StatPersistence::bMGS2_Achvmt_HoldUpAholic_Unlocked},           // Hold up 30 enemies //89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 48 89 5C 24 //eax
    {MGS2_Achvmt_DontTazeMeBro, "Don't Taze Me, Bro", &StatPersistence::bMGS2_Achvmt_DontTazeMeBro_Unlocked},      // Tranquilize 100 enemies //89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 48 81 EC ?? ?? ?? ?? 0F 29 B4 24 //eax
    {MGS2_Achvmt_NothingPersonal, "Nothing Personal", &StatPersistence::bMGS2_Achvmt_NothingPersonal_Unlocked},      // Break the neck of 30 enemies //89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 40 53 48 83 EC ?? 8B 02 //eax
    {MGS2_Achvmt_RentMoney, "Rent Money", &StatPersistence::bMGS2_Achvmt_RentMoney_Unlocked},                  // Beat 30 enemies unconscious //89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 81 F9 //eax
} };

static constexpr std::array<AchievementEntry, 1> MGS3_Achvmt_Map = { {
   // {MGS3_Achvmt_TuneInTokyo, "Tune In Tokyo", &StatPersistence::bMGS3_Achvmt_TuneInTokyo_Unlocked},                            // Call every Healing Radio frequency
   // {MGS3_Achvmt_BelieveItOrNot, "Believe It or Not", &StatPersistence::bMGS3_Achvmt_BelieveItOrNot_Unlocked},                     // Catch a Tsuchinoko (mythical serpent)
    {MGS3_Achvmt_SnakeEyes, "Snake Eyes", &StatPersistence::bMGS3_Achvmt_SnakeEyes_Unlocked},     // Discover all first-person views not indicated by the button icon
    /*{MGS3_Achvmt_JustWhatTheDoctorOrdered, "Just What the Doctor Ordered", &StatPersistence::bMGS3_Achvmt_JustWhatTheDoctorOrdered_Unlocked},// Collect every type of medicinal plant
    {MGS3_Achvmt_EverythingIsInSeason, "Everything Is in Season", &StatPersistence::bMGS3_Achvmt_EverythingIsInSeason_Unlocked},           // Collect every type of fruit
    {MGS3_Achvmt_FungusAmongUs, "Fungus Among Us", &StatPersistence::bMGS3_Achvmt_FungusAmongUs_Unlocked},                          // Collect every type of mushroom
    {MGS3_Achvmt_ABirdInTheHand, "A Bird in the Hand...", &StatPersistence::bMGS3_Achvmt_ABirdInTheHand_Unlocked},                   // Collect every type of bird
    {MGS3_Achvmt_Charmer, "Charmer", &StatPersistence::bMGS3_Achvmt_Charmer_Unlocked},                                        // Collect every type of snake
    {MGS3_Achvmt_TallTale, "Tall Tale", &StatPersistence::bMGS3_Achvmt_TallTale_Unlocked},                                     // Collect every type of fish
    {MGS3_Achvmt_ThemGoodEatin, "Them's Good Eatin'", &StatPersistence::bMGS3_Achvmt_ThemGoodEatin_Unlocked},                       // Collect every type of frog
    {MGS3_Achvmt_WithAllGunsBlazing, "With All Guns Blazing", &StatPersistence::bMGS3_Achvmt_WithAllGunsBlazing_Unlocked},               // Collect every type of weapon
    {MGS3_Achvmt_Fashionista, "Fashionista", &StatPersistence::bMGS3_Achvmt_Fashionista_Unlocked},                                // Find every type of camouflage
    {MGS3_Achvmt_OnlySkinDeep, "Only Skin Deep", &StatPersistence::bMGS3_Achvmt_OnlySkinDeep_Unlocked},                            // Find every type of face paint*/
} };

void StatPersistence::GetAchievementStats()
{
    SteamUserStats()->StoreStats(); //always call this before RequestCurrentStats() to ensure local stats are synced with the server.
    bool requestSuccess = SteamUserStats()->RequestCurrentStats();
    if (!requestSuccess)
    {
        spdlog::warn("Steam Stat Persistance: Failed to request current Steam stats.");
        // Proceed anyway, might have cached data
    }

    if (eGameType & MGS2)
    {
        for (const auto& entry : MGS2_Achvmt_Map)
        {
            bool achieved = false;
            if (SteamUserStats()->GetAchievement(entry.steamID, &achieved))
            {
                this->*entry.boolPtr = achieved;
            }
            else
            {
                spdlog::error("Steam Stat Persistance: MGS3 - Failed to get achievement '{}'", entry.friendlyName);
                this->*entry.boolPtr = false;
            }
        }
    }


    if (eGameType & MGS3)
    {
        for (const auto& entry : MGS3_Achvmt_Map)
        {
            bool achieved = false;
            if (SteamUserStats()->GetAchievement(entry.steamID, &achieved))
            {
                this->*entry.boolPtr = achieved;
            }
            else
            {
                spdlog::error("Steam Stat Persistance: MGS3 - Failed to get achievement '{}'", entry.friendlyName);
                this->*entry.boolPtr = false;
            }
        }
    }
    spdlog::info("Steam Stat Persistance: Completed achievement check.");

}

void StatPersistence::OnSteamInitialized()
{
    if (!bAchievementPersistenceEnabled || !(eGameType & (MGS2|MGS3)))
    {
        return;
    }
    pTempSaveFile = pPersistenceSaveFile = (sGameSavePath / std::to_string(*g_SteamAPI.steamID) / ("MGSHDFix_" + std::string((eGameType & MGS2) ? "mgs2" : "mgs3") + "_stats.sav"));
    pTempSaveFile += ".tmp";

    LoadPersistentStats();
    GetAchievementStats();
}

void StatPersistence::MGS2_JohnnyOnTheSpot_SetSeen(int iSceneNumber)
{
    if (bMGS2_Achvmt_JohnnyOnTheSpot_Unlocked)
    {
        return; //already unlocked.
    }
    switch (iSceneNumber)
    {
    case 1:
        bMGS2_Achvmt_JohnnyOnTheSpot_Scene1_Watched = true;
        break;
    case 2:
        bMGS2_Achvmt_JohnnyOnTheSpot_Scene2_Watched = true;
        break;
    default:
        spdlog::error("Steam Stat Persistance: MGS2_JohnnyOnTheSpot_SetSeen: Invalid scene number {}", iSceneNumber);
        return;
    }
    bUpdatedState = true;
    if (bMGS2_Achvmt_JohnnyOnTheSpot_Scene1_Watched && bMGS2_Achvmt_JohnnyOnTheSpot_Scene2_Watched)
    {
        if (SteamAPI::SetAchievement(MGS2_Achvmt_JohnnyOnTheSpot))
        {
            bMGS2_Achvmt_JohnnyOnTheSpot_Unlocked = true;
        }
    }
}

void StatPersistence::MGS3_SnakeEyes_SetSeen(int iSceneNumber)
{
    if (bMGS3_Achvmt_SnakeEyes_Unlocked)
    {
        return; //already unlocked.
    }

    switch (iSceneNumber)
    {
    case 1:
        bMGS3_Achvmt_SnakeEyes_Scene1_Watched = true;
        break;
    case 2:
        bMGS3_Achvmt_SnakeEyes_Scene2_Watched = true;
        break;
    case 3:
        bMGS3_Achvmt_SnakeEyes_Scene3_Watched = true;
        break;
    case 4:
        bMGS3_Achvmt_SnakeEyes_Scene4_Watched = true;
        break;
    case 5:
        bMGS3_Achvmt_SnakeEyes_Scene5_Watched = true;
        break;
    case 6:
        bMGS3_Achvmt_SnakeEyes_Scene6_Watched = true;
        break;
    default:
        spdlog::error("Steam Stat Persistance: MGS3_SnakeEyes_SetSeen: Invalid scene number {}", iSceneNumber);
        return;
    }
    bUpdatedState = true;
    if (bMGS3_Achvmt_SnakeEyes_Scene1_Watched && bMGS3_Achvmt_SnakeEyes_Scene2_Watched && bMGS3_Achvmt_SnakeEyes_Scene3_Watched && bMGS3_Achvmt_SnakeEyes_Scene4_Watched && bMGS3_Achvmt_SnakeEyes_Scene5_Watched && bMGS3_Achvmt_SnakeEyes_Scene6_Watched)
    {
        if (SteamAPI::SetAchievement(MGS3_Achvmt_SnakeEyes))
        {
            bMGS3_Achvmt_SnakeEyes_Unlocked = true;
        }
    }
}

#define HANDLE_COUNT_ACHIEVEMENT(ctx, unlockedFlag, currentCountVar, targetCount) \
    do { \
        if (g_StatPersistence.unlockedFlag) \
            return; \
        const unsigned int iCurrentNum = reghelpers::get_eax(ctx); \
        if (iCurrentNum >= targetCount) { \
            g_StatPersistence.unlockedFlag = true; \
            return; \
        } \
        if (g_StatPersistence.currentCountVar < iCurrentNum) { \
            g_StatPersistence.currentCountVar = iCurrentNum; \
            g_StatPersistence.bUpdatedState = true; \
            return; \
        } \
        spdlog::info("setting {} to cached val {}", iCurrentNum, g_StatPersistence.currentCountVar+1);\
        reghelpers::set_eax(ctx, g_StatPersistence.currentCountVar++); \
    } while (0)

void StatPersistence::Setup() const
{
    if (!g_SteamAPI.bIsLegitCopy)
    {
        spdlog::warn("Steam Stat Persistance: Disabled due to non-legitimate copy.");
        return;
    }
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }
    if (!bAchievementPersistenceEnabled)
    {
        spdlog::info("Steam Stat Persistance: Disabled via config.");
        return;
    }

    if (eGameType & MGS2)
    {        
        if (uint8_t* MGS2_Achvmt_JohnnyOnTheSpot_Scene1_Result = Memory::PatternScan(baseModule, "C7 05 ?? ?? ?? ?? ?? ?? ?? ?? 7E", "Steam Stat Persistance: MGS2 - Johnny on the Spot (Scene #1)"))
        {
            static SafetyHookMid MGS2_Achvmt_JohnnyOnTheSpot_Scene1_Midhook {};
            MGS2_Achvmt_JohnnyOnTheSpot_Scene1_Midhook = safetyhook::create_mid(MGS2_Achvmt_JohnnyOnTheSpot_Scene1_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS2_JohnnyOnTheSpot_SetSeen(1);
                });
            LOG_HOOK(MGS2_Achvmt_JohnnyOnTheSpot_Scene1_Midhook, "Steam Stat Persistance: MGS2 - Johnny on the Spot (Scene #1)") // Hear Johnny's bowel noises in two locations
        }

        if (uint8_t* MGS2_Achvmt_JohnnyOnTheSpot_Scene2_Result = Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 74 ?? 83 F8 ?? 7E ?? B9", "Steam Stat Persistance: MGS2 - Johnny on the Spot (Scene #2)"))
        {
            static SafetyHookMid MGS2_Achvmt_JohnnyOnTheSpot_Scene2_Midhook {};
            MGS2_Achvmt_JohnnyOnTheSpot_Scene2_Midhook = safetyhook::create_mid(MGS2_Achvmt_JohnnyOnTheSpot_Scene2_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS2_JohnnyOnTheSpot_SetSeen(2);
                });
            LOG_HOOK(MGS2_Achvmt_JohnnyOnTheSpot_Scene2_Midhook, "Steam Stat Persistance: MGS2 - Johnny on the Spot (Scene #2)") // Hear Johnny's bowel noises in two locations
        }

        if (uint8_t* MGS2_Achvmt_ByeByeBigBrother_Result = Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 8B 05", "Steam Stat Persistance: MGS2 - Bye Bye Big Brother (Destroy 5 cameras)"))
        {
            static SafetyHookMid MGS2_Achvmt_ByeByeBigBrother_Midhook {};
            MGS2_Achvmt_ByeByeBigBrother_Midhook = safetyhook::create_mid(MGS2_Achvmt_ByeByeBigBrother_Result,
                [](SafetyHookContext& ctx)
                {
                    HANDLE_COUNT_ACHIEVEMENT(ctx, bMGS2_Achvmt_ByeByeBigBrother_Unlocked, iMGS2_Achvmt_ByeByeBigBrother_Current_Count, 5);
                });
            LOG_HOOK(MGS2_Achvmt_ByeByeBigBrother_Midhook, "Steam Stat Persistance: MGS2 - Bye Bye Big Brother (Destroy 5 cameras)")
        }

        if (uint8_t* MGS2_Achvmt_HoldUpAholic_Result = Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 48 89 5C 24", "Steam Stat Persistance: MGS2 - Hold Up-aholic (Hold up 30 enemies)"))
        {
            static SafetyHookMid MGS2_Achvmt_HoldUpAholic_Midhook {};
            MGS2_Achvmt_HoldUpAholic_Midhook = safetyhook::create_mid(MGS2_Achvmt_HoldUpAholic_Result,
                [](SafetyHookContext& ctx)
                {
                    HANDLE_COUNT_ACHIEVEMENT(ctx, bMGS2_Achvmt_HoldUpAholic_Unlocked, iMGS2_Achvmt_HoldUpAholic_Current_Count, 30);
                });
            LOG_HOOK(MGS2_Achvmt_HoldUpAholic_Midhook, "Steam Stat Persistance: MGS2 - Hold Up-aholic (Hold up 30 enemies)")
        }

        if (uint8_t* MGS2_Achvmt_DontTazeMeBro_Result = Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 48 81 EC ?? ?? ?? ?? 0F 29 B4 24", "Steam Stat Persistance: MGS2 - Don't Taze Me, Bro (Tranquilize 100 enemies)"))
        {
            static SafetyHookMid MGS2_Achvmt_DontTazeMeBro_Midhook {};
            MGS2_Achvmt_DontTazeMeBro_Midhook = safetyhook::create_mid(MGS2_Achvmt_DontTazeMeBro_Result,
                [](SafetyHookContext& ctx)
                {
                    HANDLE_COUNT_ACHIEVEMENT(ctx, bMGS2_Achvmt_DontTazeMeBro_Unlocked, iMGS2_Achvmt_DontTazeMeBro_Current_Count, 100);
                });
            LOG_HOOK(MGS2_Achvmt_DontTazeMeBro_Midhook, "Steam Stat Persistance: MGS2 - Don't Taze Me, Bro (Tranquilize 100 enemies)")
        }
        
        if (uint8_t* MGS2_Achvmt_NothingPersonal_Result = Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 40 53 48 83 EC ?? 8B 02", "Steam Stat Persistance: MGS2 - Nothing Personal (Break the neck of 30 enemies)"))
        {
            static SafetyHookMid MGS2_Achvmt_NothingPersonal_Midhook {};
            MGS2_Achvmt_NothingPersonal_Midhook = safetyhook::create_mid(MGS2_Achvmt_NothingPersonal_Result,
                [](SafetyHookContext& ctx)
                {
                    HANDLE_COUNT_ACHIEVEMENT(ctx, bMGS2_Achvmt_NothingPersonal_Unlocked, iMGS2_Achvmt_NothingPersonal_Current_Count, 30);
                });
            LOG_HOOK(MGS2_Achvmt_NothingPersonal_Midhook, "Steam Stat Persistance: MGS2 - Nothing Personal (Break the neck of 30 enemies)")
        }
        
        if (uint8_t* MGS2_Achvmt_RentMoney_Result = Memory::PatternScan(baseModule, "89 05 ?? ?? ?? ?? 83 F8 ?? 7C ?? B9 ?? ?? ?? ?? E9 ?? ?? ?? ?? C3 CC CC 81 F9", "Steam Stat Persistance: MGS2 - Rent Money (Beat 30 enemies unconscious)"))
        {
            static SafetyHookMid MGS2_Achvmt_RentMoney_Midhook {};
            MGS2_Achvmt_RentMoney_Midhook = safetyhook::create_mid(MGS2_Achvmt_RentMoney_Result,
                [](SafetyHookContext& ctx)
                {
                    HANDLE_COUNT_ACHIEVEMENT(ctx, bMGS2_Achvmt_RentMoney_Unlocked, iMGS2_Achvmt_RentMoney_Current_Count, 30);
                });
                LOG_HOOK(MGS2_Achvmt_RentMoney_Midhook, "Steam Stat Persistance: MGS2 - Rent Money (Beat 30 enemies unconscious)")
        }
    }
    else if (eGameType & MGS3)
    {
        if (uint8_t* MGS3_SnakeEyes_Scene_Result = Memory::PatternScan(baseModule, "83 C8 ?? 89 05 ?? ?? ?? ?? E9 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 85 C0 75 ?? 81 FF", "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #1)"))
        {
            static SafetyHookMid MGS3_Achievement_SnakeEyes_Scene1_Midhook {};
            MGS3_Achievement_SnakeEyes_Scene1_Midhook = safetyhook::create_mid(MGS3_SnakeEyes_Scene_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS3_SnakeEyes_SetSeen(1);
                });
            LOG_HOOK(MGS3_Achievement_SnakeEyes_Scene1_Midhook, "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #1)")
        }

        if (uint8_t* MGS3_SnakeEyes_Scene_Result = Memory::PatternScan(baseModule, "83 C8 ?? 89 05 ?? ?? ?? ?? E9 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 85 C0 75 ?? 8B 05", "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #2)"))
        {
            static SafetyHookMid MGS3_Achievement_SnakeEyes_Scene2_Midhook {};
            MGS3_Achievement_SnakeEyes_Scene2_Midhook = safetyhook::create_mid(MGS3_SnakeEyes_Scene_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS3_SnakeEyes_SetSeen(2);
                });
            LOG_HOOK(MGS3_Achievement_SnakeEyes_Scene2_Midhook, "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #2)")
        }

        if (uint8_t* MGS3_SnakeEyes_Scene_Result = Memory::PatternScan(baseModule, "83 C8 ?? 89 05 ?? ?? ?? ?? EB ?? 48 8D 15 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 85 C0 75 ?? 8B 05 ?? ?? ?? ?? 83 C8", "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #3)"))
        {
            static SafetyHookMid MGS3_Achievement_SnakeEyes_Scene3_Midhook {};
            MGS3_Achievement_SnakeEyes_Scene3_Midhook = safetyhook::create_mid(MGS3_SnakeEyes_Scene_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS3_SnakeEyes_SetSeen(3);
                });
            LOG_HOOK(MGS3_Achievement_SnakeEyes_Scene3_Midhook, "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #3)")
        }


        if (uint8_t* MGS3_SnakeEyes_Scene_Result = Memory::PatternScan(baseModule, "83 C8 ?? 89 05 ?? ?? ?? ?? EB ?? 48 8D 15 ?? ?? ?? ?? 48 8B CB E8 ?? ?? ?? ?? 85 C0 75 ?? 8B 05 ?? ?? ?? ?? 81 FF", "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #4)"))
        {
            static SafetyHookMid MGS3_Achievement_SnakeEyes_Scene4_Midhook {};
            MGS3_Achievement_SnakeEyes_Scene4_Midhook = safetyhook::create_mid(MGS3_SnakeEyes_Scene_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS3_SnakeEyes_SetSeen(4);
                });
            LOG_HOOK(MGS3_Achievement_SnakeEyes_Scene4_Midhook, "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #4)")
        }

        if (uint8_t* MGS3_SnakeEyes_Scene_Result = Memory::PatternScan(baseModule, "83 C8 ?? 0F 2F 44 24", "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #5)"))
        {
            static SafetyHookMid MGS3_Achievement_SnakeEyes_Scene5_Midhook {};
            MGS3_Achievement_SnakeEyes_Scene5_Midhook = safetyhook::create_mid(MGS3_SnakeEyes_Scene_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS3_SnakeEyes_SetSeen(5);
                });
            LOG_HOOK(MGS3_Achievement_SnakeEyes_Scene5_Midhook, "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #5)")
        }
        if (uint8_t* MGS3_SnakeEyes_Scene_Result = Memory::PatternScan(baseModule, "83 C8 ?? 89 05 ?? ?? ?? ?? EB ?? CC", "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #6)"))
        {
            static SafetyHookMid MGS3_Achievement_SnakeEyes_Scene6_Midhook {};
            MGS3_Achievement_SnakeEyes_Scene6_Midhook = safetyhook::create_mid(MGS3_SnakeEyes_Scene_Result,
                [](SafetyHookContext& ctx)
                {
                    g_StatPersistence.MGS3_SnakeEyes_SetSeen(6);
                });
            LOG_HOOK(MGS3_Achievement_SnakeEyes_Scene6_Midhook, "Steam Stat Persistance: MGS3 - Snake Eyes (Scene #6)")
        }
    }
}
#undef HANDLE_ACHIEVEMENT


void StatPersistence::LoadPersistentStats()
{
    if (g_SteamAPI.bResetAchievements)
    {
        spdlog::info("Steam Stat Persistence: Resetting persistent stats.");
        return;
    }

    if (!std::filesystem::exists(pPersistenceSaveFile))
    {
        spdlog::info("Steam Stat Persistence: No persistent stats found in {}, skipping load.", pPersistenceSaveFile.string());
        return;
    }

    std::ifstream ifs(pPersistenceSaveFile);
    if (!ifs)
    {
        spdlog::error("Steam Stat Persistence: Failed to open save file for reading: {}", pPersistenceSaveFile.string());
        return;
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        int intval = std::stoi(value);

        if (eGameType & MGS2)
        {
            if (key == "MGS2_JohnnyOnTheSpot_Scene1") bMGS2_Achvmt_JohnnyOnTheSpot_Scene1_Watched = intval;
            else if (key == "MGS2_JohnnyOnTheSpot_Scene2") bMGS2_Achvmt_JohnnyOnTheSpot_Scene2_Watched = intval;

            else if (key == "MGS2_ByeByeBigBrother_Count") iMGS2_Achvmt_ByeByeBigBrother_Current_Count = intval;
            else if (key == "MGS2_HoldUpAholic_Count")     iMGS2_Achvmt_HoldUpAholic_Current_Count = intval;
            else if (key == "MGS2_DontTazeMeBro_Count")    iMGS2_Achvmt_DontTazeMeBro_Current_Count = intval;
            else if (key == "MGS2_NothingPersonal_Count")  iMGS2_Achvmt_NothingPersonal_Current_Count = intval;
            else if (key == "MGS2_RentMoney_Count")        iMGS2_Achvmt_RentMoney_Current_Count = intval;
        }

        if (eGameType & MGS3)
        {
            if (key == "MGS3_SnakeEyes_Scene1") bMGS3_Achvmt_SnakeEyes_Scene1_Watched = intval;
            else if (key == "MGS3_SnakeEyes_Scene2") bMGS3_Achvmt_SnakeEyes_Scene2_Watched = intval;
            else if (key == "MGS3_SnakeEyes_Scene3") bMGS3_Achvmt_SnakeEyes_Scene3_Watched = intval;
            else if (key == "MGS3_SnakeEyes_Scene4") bMGS3_Achvmt_SnakeEyes_Scene4_Watched = intval;
            else if (key == "MGS3_SnakeEyes_Scene5") bMGS3_Achvmt_SnakeEyes_Scene5_Watched = intval;
            else if (key == "MGS3_SnakeEyes_Scene6") bMGS3_Achvmt_SnakeEyes_Scene6_Watched = intval;
        }
    }

    spdlog::info("Steam Stat Persistence: Loaded persistent stats from {}", pPersistenceSaveFile.string());
}


/////// From Steamworks's documentation on best practices for cloud saves:
/// "We recommend that you write save data to a temporary file, and then rename the temporary file to the actual save file."
/// "Renaming is an atomic operation and ensures that at no point is the file in an incomplete or invalid state."
void StatPersistence::SaveStats() const
{
    if (!g_StatPersistence.bUpdatedState)
    {
        return;
    }

    try
    {
        std::ofstream ofs(pTempSaveFile, std::ios::trunc);
        if (!ofs)
        {
            throw std::runtime_error("Failed to open temp file for writing");
        }

        if (eGameType & MGS2)
        {
            if (!bMGS2_Achvmt_JohnnyOnTheSpot_Unlocked)
            {
                ofs << "MGS2_JohnnyOnTheSpot_Scene1=" << bMGS2_Achvmt_JohnnyOnTheSpot_Scene1_Watched << "\n";
                ofs << "MGS2_JohnnyOnTheSpot_Scene2=" << bMGS2_Achvmt_JohnnyOnTheSpot_Scene2_Watched << "\n";
            }

            if (!iMGS2_Achvmt_ByeByeBigBrother_Current_Count == 0)
                ofs << "MGS2_ByeByeBigBrother_Count=" << iMGS2_Achvmt_ByeByeBigBrother_Current_Count << "\n";

            if (!bMGS2_Achvmt_HoldUpAholic_Unlocked && iMGS2_Achvmt_HoldUpAholic_Current_Count > 0)
                ofs << "MGS2_HoldUpAholic_Count=" << iMGS2_Achvmt_HoldUpAholic_Current_Count << "\n";

            if (!bMGS2_Achvmt_DontTazeMeBro_Unlocked && iMGS2_Achvmt_DontTazeMeBro_Current_Count > 0)
                ofs << "MGS2_DontTazeMeBro_Count=" << iMGS2_Achvmt_DontTazeMeBro_Current_Count << "\n";

            if (!bMGS2_Achvmt_NothingPersonal_Unlocked && iMGS2_Achvmt_NothingPersonal_Current_Count > 0)
                ofs << "MGS2_NothingPersonal_Count=" << iMGS2_Achvmt_NothingPersonal_Current_Count << "\n";

            if (!bMGS2_Achvmt_RentMoney_Unlocked && iMGS2_Achvmt_RentMoney_Current_Count > 0)
                ofs << "MGS2_RentMoney_Count=" << iMGS2_Achvmt_RentMoney_Current_Count << "\n";
        }
        
        if ((eGameType & MGS3) && !bMGS3_Achvmt_SnakeEyes_Unlocked)
        {
            ofs << "MGS3_SnakeEyes_Scene1=" << bMGS3_Achvmt_SnakeEyes_Scene1_Watched << "\n";
            ofs << "MGS3_SnakeEyes_Scene2=" << bMGS3_Achvmt_SnakeEyes_Scene2_Watched << "\n";
            ofs << "MGS3_SnakeEyes_Scene3=" << bMGS3_Achvmt_SnakeEyes_Scene3_Watched << "\n";
            ofs << "MGS3_SnakeEyes_Scene4=" << bMGS3_Achvmt_SnakeEyes_Scene4_Watched << "\n";
            ofs << "MGS3_SnakeEyes_Scene5=" << bMGS3_Achvmt_SnakeEyes_Scene5_Watched << "\n";
            ofs << "MGS3_SnakeEyes_Scene6=" << bMGS3_Achvmt_SnakeEyes_Scene6_Watched << "\n";
        }

        ofs.flush();
        if (!ofs)
        {
            throw std::runtime_error("Failed to flush temp save file");
        }

        ofs.close();

        // Rename temp -> final (atomic)
        std::filesystem::rename(pTempSaveFile, pPersistenceSaveFile);

        spdlog::info("Steam Stat Persistence: Stats saved successfully.");
        g_StatPersistence.bUpdatedState = false;
    }
    catch (const std::exception& e)
    {
        spdlog::error("Steam Stat Persistence: Failed to save stats: {}", e.what());
    }
}
