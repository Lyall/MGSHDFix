#pragma once

class StatPersistence final
{
private:
    void MGS2_JohnnyOnTheSpot_SetSeen(int iSceneNumber);
    void MGS3_SnakeEyes_SetSeen(int iSceneNumber); // For MGS3 Snake Eyes achievement scenes
    void GetAchievementStats();      // Internal Steam query
    void LoadPersistentStats();      // For loading saved persistent stats from disk or config (stub)
    bool bUpdatedState = false;
    std::filesystem::path pPersistenceSaveFile;
    std::filesystem::path pTempSaveFile;


public:
    void Setup() const;
    void SaveStats() const;                       // Save persistent stats (stub)
    void OnSteamInitialized();              // Call after Steam is ready to fetch achievements

    bool bAchievementPersistenceEnabled = true;

    // MGS2

    bool bMGS2_Achvmt_JohnnyOnTheSpot_Unlocked = false;           // Hear Johnny's bowel noises in two locations
    bool bMGS2_Achvmt_JohnnyOnTheSpot_Scene1_Watched = false;
    bool bMGS2_Achvmt_JohnnyOnTheSpot_Scene2_Watched = false;

    bool bMGS2_Achvmt_ByeByeBigBrother_Unlocked = false;          // Destroy 5 cameras
    unsigned int  iMGS2_Achvmt_ByeByeBigBrother_Current_Count = 0;

    bool bMGS2_Achvmt_HoldUpAholic_Unlocked = false;              // Hold up 30 enemies
    int  iMGS2_Achvmt_HoldUpAholic_Current_Count = 0;

    bool bMGS2_Achvmt_DontTazeMeBro_Unlocked = false;             // Tranquilize 100 enemies
    unsigned int  iMGS2_Achvmt_DontTazeMeBro_Current_Count = 0;

    bool bMGS2_Achvmt_NothingPersonal_Unlocked = false;           // Break the neck of 30 enemies
    unsigned int  iMGS2_Achvmt_NothingPersonal_Current_Count = 0;

    bool bMGS2_Achvmt_RentMoney_Unlocked = false;                 // Beat 30 enemies unconscious
    unsigned int  iMGS2_Achvmt_RentMoney_Current_Count = 0;


    // MGS3 
    bool bMGS3_Achvmt_SnakeEyes_Unlocked = false;                 // Discover all first-person views not indicated by the button icon
    bool bMGS3_Achvmt_SnakeEyes_Scene1_Watched = false;
    bool bMGS3_Achvmt_SnakeEyes_Scene2_Watched = false;
    bool bMGS3_Achvmt_SnakeEyes_Scene3_Watched = false;
    bool bMGS3_Achvmt_SnakeEyes_Scene4_Watched = false;
    bool bMGS3_Achvmt_SnakeEyes_Scene5_Watched = false;
    bool bMGS3_Achvmt_SnakeEyes_Scene6_Watched = false;

    bool bMGS3_Achvmt_TuneInTokyo_Unlocked = false;               // Call every Healing Radio frequency
    bool bMGS3_Achvmt_BelieveItOrNot_Unlocked = false;            // Catch a Tsuchinoko (mythical serpent)
    bool bMGS3_Achvmt_JustWhatTheDoctorOrdered_Unlocked = false;  // Collect every type of medicinal plant
    bool bMGS3_Achvmt_EverythingIsInSeason_Unlocked = false;      // Collect every type of fruit
    bool bMGS3_Achvmt_FungusAmongUs_Unlocked = false;             // Collect every type of mushroom
    bool bMGS3_Achvmt_ABirdInTheHand_Unlocked = false;            // Collect every type of bird
    bool bMGS3_Achvmt_Charmer_Unlocked = false;                   // Collect every type of snake
    bool bMGS3_Achvmt_TallTale_Unlocked = false;                  // Collect every type of fish
    bool bMGS3_Achvmt_ThemGoodEatin_Unlocked = false;             // Collect every type of frog
    bool bMGS3_Achvmt_WithAllGunsBlazing_Unlocked = false;        // Collect every type of weapon
    bool bMGS3_Achvmt_Fashionista_Unlocked = false;               // Find every type of camouflage
    bool bMGS3_Achvmt_OnlySkinDeep_Unlocked = false;              // Find every type of face paint
};
    
inline StatPersistence g_StatPersistence;
