// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

#include "mgs2_linkvarbuf.hpp"
#include "mgs2_equipment_enums.hpp"
#include "game_stages.hpp"

class GameVars final
{
public:
    void Initialize();
    [[nodiscard]] bool InCutscene() const; // If we're in a full demo cutscene.
    [[nodiscard]] bool InScriptedSequence() const; // If the game is in a scripted sequence (cutscene or pad demo).
    [[nodiscard]] double ActorWaitMultiplier() const;
    [[nodiscard]] double ActorWaitValue() const;
    [[nodiscard]] float get_GM_WaterLevel() const;

    [[nodiscard]] std::string GetRichPresenceString() const;
    [[nodiscard]] std::string GetGameMode() const;
    [[nodiscard]] const char* GetCurrentStage() const;
    [[nodiscard]] bool IsStage(const char* stageConst) const;
    [[nodiscard]] bool IsAnyStage(std::initializer_list<const char*> stages) const;

    void SetAimingState(uint64_t state) const;
    [[nodiscard]] uint64_t GetAimingState() const;

    enum HoldingTriggers : uint32_t
    {
        // MGS2 analog byte triggers
        MGS2_WeaponMenu = 0xFF000000u,
        MGS2_EquipmentMenu = 0x00FF0000u,
        MGS2_FirstPerson = 0x0000FF00u,
        MGS2_LockOn = 0x000000FFu,

        // MGS3 boolean bit triggers
        MGS3_EquipmentMenu = 1u << 8,   // 0x00000100
        MGS3_WeaponMenu = 1u << 9,   // 0x00000200
        MGS3_LockOn = 1u << 10,  // 0x00000400
        MGS3_FirstPerson = 1u << 11   // 0x00000800
    };

    [[nodiscard]] bool MGS2IsHoldingWeaponMenu() const;
    [[nodiscard]] bool MGS2IsHoldingEquipmentMenu() const;
    [[nodiscard]] bool MGS2IsHoldingFirstPerson() const;
    [[nodiscard]] bool MGS2IsHoldingLockOn() const;
    [[nodiscard]] bool MGS3IsHoldingWeaponMenu() const;
    [[nodiscard]] bool MGS3IsHoldingEquipmentMenu() const;
    [[nodiscard]] bool MGS3IsHoldingFirstPerson() const;
    [[nodiscard]] bool MGS3IsHoldingLockOn() const;

    [[nodiscard]] uint64_t Get_PL_Status() const;
    [[nodiscard]] int Get_GM_GameStatus() const;
    [[nodiscard]] int Get_GM_VRStatus() const;
private:
    static void OnLevelTransition();

    uint64_t* aimingState = nullptr;
    int* cutsceneFlag = nullptr;
    int* scriptedSequenceFlag = nullptr;
    double* actorWaitValue = nullptr;
    const char* currentStage = nullptr;
    uint32_t* heldTriggers = nullptr;
    float* GM_WaterLevel = nullptr;

    std::uint64_t* GM_PlayerStatus = nullptr;
    int* GM_GameStatus = nullptr;
    int* GM_VRStatus = nullptr;

};

inline GameVars g_GameVars;

enum MGS2_GameStateFlags : uint32_t
{
    STATE_DETECT        = (1u << 0),   /// Discovery -> caution mode
    STATE_CLEARING      = (1u << 1),   /// Discovery -> caution mode
    STATE_BIG_SNORE     = (1u << 2),   /// Loud snoring
    STATE_ENE_SIGHTIN   = (1u << 3),   /// Seen by enemy
                                       
    STATE_CHAFF         = (1u << 4),   /// Chaff active
    STATE_STUN          = (1u << 5),   /// Stun grenade active
    STATE_CUT_IN        = (1u << 6),   /// Cut-in camera active
    STATE_RADAR_JAMMING = (1u << 7),   /// Radar interference
                                       
    STATE_PAUSE_DISABLE = (1u << 9),   /// Cannot pause
                                       
    STATE_VIB_PAUSE0    = (1u << 12),  /// Level 0 vibration disabled
    STATE_VIB_PAUSE1    = (1u << 13),  /// Level 1 vibration disabled
    STATE_DISP_GAMEOVER = (1u << 14),  /// Game over screen active
                                       
    STATE_VR_ONLY       = (1u << 22),  /// VR mode, not Another Mission
    STATE_VR_ANOTHER    = (1u << 23),  /// VR or Another Mission
    STATE_BOSS_SURVIVAL = (1u << 24),  /// Boss survival mode
    STATE_GLL           = (1u << 25),  /// VS Gurlugon
    STATE_GNO           = (1u << 26),  /// VS Genola
                                       
    STATE_SCN_DEMO      = (1u << 27),  /// Scenario demo
    STATE_DEMO          = (1u << 28),  /// Polygon demo
    STATE_PRG_DEMO      = (1u << 29),  /// Program demo
    STATE_PAD_DEMO      = (1u << 30),  /// Pad demo
    STATE_GAMEOVER      = (1u << 31)   /// Game over processing / call a fucking ambulance BUT NOT FOR ME
};

enum MGS2CameraPriority
{
    GM_CAMERA_CUT_IN = 0,  /// Cut-in camera
    GM_CAMERA_PROG1,       /// Program camera 1
    GM_CAMERA_SUBJECT,     /// First-person / subjective player camera
    GM_CAMERA_PROG2,       /// Program camera 2
    GM_CAMERA_BEHIND,      /// Behind camera
    GM_CAMERA_PROG3,       /// Program camera 3
    GM_CAMERA_AREA,        /// Area-specified camera (scenario controlled)
    GM_CAMERA_PROG4,       /// Program camera 4
    GM_CAMERA_DEFAULT,     /// Stage default camera
    GM_CAMERA_MAX
};

inline constexpr uint32_t GM_STATUS_DETECT = STATE_DETECT;

inline constexpr uint32_t STATE_PLAY_DEMO = (STATE_DEMO | STATE_PRG_DEMO | STATE_PAD_DEMO | STATE_SCN_DEMO);

inline constexpr uint32_t STATE_JAMMING = (STATE_CHAFF | STATE_RADAR_JAMMING);

enum MGS2_PlayerStatusFlags : uint64_t
{
    PLAYER_NORMAL = 0,

    PLAYER_WATCH                 = (1ull << 0),  /// First person view
    PLAYER_INTRUDE               = (1ull << 1),  /// Intrude
                                             
    PLAYER_SQUAT                 = (1ull << 4),  /// Crouching
    PLAYER_GROUND                = (1ull << 5),  /// Prone / downed
    PLAYER_CAUTION               = (1ull << 6),  /// Wall-hugging
    PLAYER_LOCKER                = (1ull << 7),  /// Inside locker
                                             
    PLAYER_ATTACK                = (1ull << 8),  /// Attacking (unused)
    PLAYER_DAMAGED               = (1ull << 9),  /// In damage mode
    PLAYER_DOWNED                = (1ull << 10), /// Downed
    PLAYER_HOLD                  = (1ull << 11), /// Weapon raised
                                             
    PLAYER_BEYOND                = (1ull << 12),  /// Beyond mode
    PLAYER_FORCE                 = (1ull << 13),  /// Forced motion
    PLAYER_CB_BOX                = (1ull << 14),  /// Cardboard box mode
    PLAYER_DEAD                  = (1ull << 15),  /// Game over
                                             
    PLAYER_LADDER                = (1ull << 16),  /// Ladder mode
    PLAYER_ENEMY_PULL            = (1ull << 17),  /// Dragging enemy
    PLAYER_CB_BOX_STAND          = (1ull << 18),  /// Standing in cardboard box
                                             
    // Below here counts as "normal state"       
    PLAYER_BLOOD_DROP            = (1ull << 24), /// Dripping blood
    PLAYER_WEAPON_DISABLE        = (1ull << 25), /// Weapon menu disabled
    PLAYER_ITEM_DISABLE          = (1ull << 26), /// Item menu disabled
    PLAYER_MENU_OPEN             = (1ull << 27), /// Menu open
                                             
    PLAYER_STOP                  = (1ull << 28), /// Processing stopped
    PLAYER_BEHIND_CAMERA_ENABLE  = (1ull << 29), /// Behind camera allowed
    PLAYER_WEAPON_INVISIBLE      = (1ull << 30), /// Weapon hidden
    PLAYER_DEBUG                 = (1ull << 31), /// Debug mode
                                             
    PLAYER_WEAPON_QUICK_ONLY     = (1ull << 32), /// Weapon quick change only
    PLAYER_ITEM_QUICK_ONLY       = (1ull << 33), /// Item quick change only
    PLAYER_NEED_NEW_PRESS        = (1ull << 34), /// Button re-press required
    PLAYER_ENEMY_HANG            = (1ull << 35), /// Holding enemy
                                             
    PLAYER_SIGHT_LOCKON          = (1ull << 36), /// Sight locked on enemy
    PLAYER_SNAKE                 = (1ull << 37), /// Player is Snake
    PLAYER_DARK_AREA             = (1ull << 38), /// In dark area
    PLAYER_ENEMY_HIDDEN          = (1ull << 39), /// Hidden from enemies

    PLAYER_INVINCIBLE            = (1ull << 40), /// Invincible
    PLAYER_PAD_OFF               = (1ull << 41), /// Controller disabled
    PLAYER_BEHIND                = (1ull << 42), /// Behind camera
    PLAYER_MOVE                  = (1ull << 43), /// Moving
                                             
    PLAYER_EVENT_ENABLE          = (1ull << 44), /// Event can trigger
    PLAYER_IN_THE_WATER          = (1ull << 45), /// In water
    PLAYER_ON_CORPSE             = (1ull << 46), /// On corpse
    PLAYER_CROSS                 = (1ull << 47), /// Overlapping
                                             
    PLAYER_INVINCIBLE_SCN        = (1ull << 48), /// Invincible from scenario
    PLAYER_INVINCIBLE_PRG        = (1ull << 49), /// Invincible externally
    PLAYER_BEHIND_ATTACK         = (1ull << 50), /// Pop-out attack
    PLAYER_CB_BOX_CANCELED       = (1ull << 51), /// Cardboard box removed
                                             
    PLAYER_ROLLING               = (1ull << 52), /// Rolling
    PLAYER_NARROW                = (1ull << 53), /// In narrow space
    PLAYER_CB_BOX_HIDDEN         = (1ull << 54), /// Hidden if in cardboard box
    PLAYER_NORECOVER             = (1ull << 55), /// Cannot recover
                                             
    PLAYER_WALK                  = (1ull << 56), /// Walking
    PLAYER_DASH                  = (1ull << 57), /// Dashing
    PLAYER_CBBOX_RUN             = (1ull << 58), /// Moving in cardboard box
    PLAYER_COLD                  = (1ull << 59), /// Has a cold
                                             
    PLAYER_WATER_SURFACE         = (1ull << 60), /// Near water surface
    PLAYER_NO_BREATH             = (1ull << 61), /// Cannot breathe
    PLAYER_STEALTH               = (1ull << 62), /// Stealth camo

    //PLAYER_STATUS_EX = (1ull << 63)              /// GM_PlayerStatus2
};




enum MGS3WeaponIndex : uint8_t
{
    MGS3_WEAPON_INDEX_MK22 = 0x5,
    MGS3_WEAPON_INDEX_M1911A1 = 0x6,
    MGS3_WEAPON_INDEX_EzGun = 0x7,
    MGS3_WEAPON_INDEX_SAA = 0x8,
    MGS3_WEAPON_INDEX_Patriot = 0x9,
    MGS3_WEAPON_INDEX_Scorpion = 0xA,
    MGS3_WEAPON_INDEX_XM16E1 = 0xB,
    MGS3_WEAPON_INDEX_AK47 = 0xC,
    MGS3_WEAPON_INDEX_M63 = 0xD,
    MGS3_WEAPON_INDEX_M37 = 0xE,
    MGS3_WEAPON_INDEX_SVD = 0xF,
    MGS3_WEAPON_INDEX_Mosin = 0x10,
    MGS3_WEAPON_INDEX_RPG7 = 0x11,

    /*
    MGS3_WEAPON_INDEX_SurvivalKnife     = 0x,
    MGS3_WEAPON_INDEX_Fork              = 0x,
    MGS3_WEAPON_INDEX_CigSpray          = 0x,
    MGS3_WEAPON_INDEX_Handkerchief      = 0x,
    MGS3_WEAPON_INDEX_Torch             = 0x,
    MGS3_WEAPON_INDEX_Grenade           = 0x,
    MGS3_WEAPON_INDEX_WpGrenade         = 0x,
    MGS3_WEAPON_INDEX_StunGrenade       = 0x,
    MGS3_WEAPON_INDEX_ChaffGrenade      = 0x,
    MGS3_WEAPON_INDEX_SmokeGrenade      = 0x,
    MGS3_WEAPON_INDEX_EmptyMag          = 0x,
    MGS3_WEAPON_INDEX_TNT               = 0x,
    MGS3_WEAPON_INDEX_C3                = 0x,
    MGS3_WEAPON_INDEX_Claymore          = 0x,
    MGS3_WEAPON_INDEX_Book              = 0x,
    MGS3_WEAPON_INDEX_Mousetrap         = 0x,
    MGS3_WEAPON_INDEX_DirectionalMic    = 0x,
    */
};
