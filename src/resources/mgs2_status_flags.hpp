#pragma once 

namespace MGS2_Characters
{
    enum class PlayerCharacter
    {
        Unknown,

        NormalRaiden,
        NakedRaiden,
        NinjaRaiden,

        NormalSnake,
        SpecialSnake,
        TuxedoSnake,
        MGS1Snake,

        Pliskin,
    };

    enum class PlayerCharacterFamily
    {
        Unknown,
        Raiden,
        Snake,
        Pliskin,
    };

    struct PlayerResidentEntry
    {
        std::string_view residentDirectory;
        PlayerCharacter character;
    };

    inline constexpr std::array PlayerResidentEntries {
        PlayerResidentEntry { "r_plt0",     PlayerCharacter::NormalRaiden },
        PlayerResidentEntry { "r_plt1",     PlayerCharacter::NormalRaiden }, //dive-suit
        PlayerResidentEntry { "r_plt3",     PlayerCharacter::NormalRaiden },
        PlayerResidentEntry { "r_plt4",     PlayerCharacter::NormalRaiden }, //poly demo only - Gurlukovich soldier equipment/loadout (with cap)
        PlayerResidentEntry { "r_plt5",     PlayerCharacter::NormalRaiden }, //poly demo only - Gurlukovich soldier equipment/loadout (with cap)
        PlayerResidentEntry { "r_plt6",     PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt7",     PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt8",     PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt9",     PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt10",    PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt11",    PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt12",    PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_plt13",    PlayerCharacter::NormalRaiden }, //low-poly demo resident
        PlayerResidentEntry { "r_rai_b",    PlayerCharacter::NormalRaiden }, //boss rush resident scenario
        PlayerResidentEntry { "r_vr_r",     PlayerCharacter::NormalRaiden },
        PlayerResidentEntry { "r_vr_rp",    PlayerCharacter::NormalRaiden }, //photo mode

        PlayerResidentEntry { "r_plt2",     PlayerCharacter::NakedRaiden },
        PlayerResidentEntry { "r_vr_x",     PlayerCharacter::NakedRaiden },

        PlayerResidentEntry { "r_vr_b",     PlayerCharacter::NinjaRaiden },

        PlayerResidentEntry { "r_sna_b",    PlayerCharacter::NormalSnake }, //Snake / boss rush resident scenario
        PlayerResidentEntry { "r_tnk0",     PlayerCharacter::NormalSnake }, //Tanker
        PlayerResidentEntry { "r_plt_s",    PlayerCharacter::NormalSnake }, //Snake Tales
        PlayerResidentEntry { "r_vr_s",     PlayerCharacter::NormalSnake },
        PlayerResidentEntry { "r_vr_sp",    PlayerCharacter::NormalSnake },

        PlayerResidentEntry { "r_vr_t",     PlayerCharacter::TuxedoSnake },
        PlayerResidentEntry { "r_vr_1",     PlayerCharacter::MGS1Snake },

        PlayerResidentEntry { "r_vr_p",     PlayerCharacter::Pliskin },
        
        PlayerResidentEntry { "r_tnk_r",    PlayerCharacter::SpecialSnake }, //Tanker - digital camera resident scenario

    };

    /// Please cache the current character when performing multiple checks to avoid repeated resident derefs. <3
    ///
    /// Example:
    ///
    ///      using namespace MGS2_Characters;
    ///     
    ///      if (IsCurrentlyCharacter(PlayerCharacter::TuxedoSnake))
    ///       {
    ///           doNeediful()
    ///       }
    ///     
    /// ---------------------------------------------
    /// or:
    ///     
    ///      using namespace MGS2_Characters;
    ///     
    ///      const PlayerCharacter currentCharacter = GetCurrentPlayerCharacter();
    ///     
    ///      if (currentCharacter == PlayerCharacter::TuxedoSnake)
    ///      {
    ///          doOtherThing();
    ///      }
    ///      else if (currentCharacter == PlayerCharacter::MGS1Snake)
    ///      {
    ///          doThing();
    ///      }
    ///
    [[nodiscard]] PlayerCharacter GetCurrentPlayerCharacter();

    [[nodiscard]] constexpr PlayerCharacterFamily GetPlayerCharacterFamily(const PlayerCharacter character);

    [[nodiscard]] PlayerCharacterFamily GetCurrentCharacterFamily();

    [[nodiscard]] bool IsCurrentlyCharacter(const PlayerCharacter character);

    [[nodiscard]] bool IsRaiden();

    [[nodiscard]] bool IsSnake();

    [[nodiscard]] bool IsPliskin();

    [[nodiscard]] constexpr std::string_view GetPlayerCharacterName(const PlayerCharacter character);

    [[nodiscard]] std::string_view GetCurrentCharacterName();

    [[nodiscard]] bool IsCurrentResident(const std::string_view residentDirectory);

}

namespace MGS2_StatusFlags
{
    enum MGS2PauseLevelFlags : int
    {
        MGS2_GV_PAUSE_NOSTOP = 0x00,
        MGS2_GV_PAUSE_STOP = 0x01,
        MGS2_GV_PAUSE_PAUSE = 0x02,
        MGS2_GV_PAUSE_MENU = 0x04,
        MGS2_GV_PAUSE_READERROR = 0x08,
        MGS2_GV_PAUSE_DISCERROR = 0x10,
        MGS2_GV_PAUSE_DEBUG = 0x20
    };

    constexpr int MGS2_GV_LEVEL_NORMAL =
    MGS2_GV_PAUSE_STOP |
    MGS2_GV_PAUSE_PAUSE |
    MGS2_GV_PAUSE_MENU |
    MGS2_GV_PAUSE_READERROR |
    MGS2_GV_PAUSE_DISCERROR |
    MGS2_GV_PAUSE_DEBUG;

    constexpr int MGS2_GV_LEVEL_STOP =
    MGS2_GV_PAUSE_STOP |
    MGS2_GV_PAUSE_READERROR |
    MGS2_GV_PAUSE_DISCERROR |
    MGS2_GV_PAUSE_DEBUG;

    constexpr int MGS2_GV_LEVEL_NOSTOP = MGS2_GV_PAUSE_NOSTOP;

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

    enum MGS2_MenuStatusFlags : uint32_t
    {
        MENU_NORMAL = 0,

        /* Visibility */
        MENU_WEAPON_OFF = (1u << 0),  /// Weapon menu hidden
        MENU_ITEM_OFF = (1u << 1),  /// Item menu hidden
        MENU_RADAR_OFF = (1u << 2),  /// Radar hidden
        MENU_GAGE_OFF = (1u << 3),  /// Gauge hidden
        MENU_CAPTION_OFF = (1u << 4),  /// Captions hidden
        MENU_SUBWIN_OFF = (1u << 5),  /// Sub-window hidden

        /* Runtime state */
        MENU_WEAPON_OPEN = (1u << 8),  /// Weapon menu open
        MENU_ITEM_OPEN = (1u << 9),  /// Item menu open
        MENU_RADIO_ON = (1u << 10), /// Codec/radio active
        MENU_RADAR_ON = (1u << 11), /// Radar visible
        MENU_GAGE_ON = (1u << 12), /// Gauge visible
        MENU_SUBWIN_ON = (1u << 13), /// Sub-window visible
        MENU_NODE_ACCESSED = (1u << 14), /// Node accessed
        MENU_NODE_ON = (1u << 15), /// Node visible

        /* Access restricted */
        MENU_WEAPON_DISABLE = (1u << 16), /// Weapon menu disabled
        MENU_ITEM_DISABLE = (1u << 17), /// Item menu disabled
        MENU_RADIO_DISABLE = (1u << 18), /// Codec/radio disabled
        MENU_VIBRATE_DISABLE = (1u << 19), /// Vibration disabled

        /* Menu-affecting state */
        MENU_STREAM_CH_0 = (1u << 20), /// STREAM ch0
        MENU_STREAM_CH_1 = (1u << 21), /// STREAM ch1

        /* Misc */
        MENU_MENU_NEWPRESS = (1u << 24), /// Menu button re-press required

        MENU_MENU_OFF = MENU_WEAPON_OFF | MENU_ITEM_OFF,
        MENU_MENU_OPEN = MENU_WEAPON_OPEN | MENU_ITEM_OPEN,
        MENU_MENU_DISABLE = MENU_WEAPON_DISABLE | MENU_ITEM_DISABLE,
    };

    enum MGS2_Config2Masks : uint32_t
    {
        GM_CONFIG_SE_VOLUME = 0x0000000F,  /// SE volume (4-bit, 16 levels, 0=max 15=min, Xbox only)
        GM_CONFIG_MUSIC_VOLUME = 0x000000F0,  /// Music volume (4-bit, 16 levels, 0=max 15=min, Xbox only)
        GM_CONFIG_CONTROLS = 0x00000300,  /// Controller layout (2-bit, 4 types, Xbox only)
        GM_CONFIG_PLATFORM = 0x00000C00,  /// Platform (2-bit, 4 types)
        GM_CONFIG_REGION = 0x00007000,  /// Release region (3-bit, 8 regions)
        GM_CONFIG_DOGTAGS_2002 = (1u << 15),  /// New dog tags
    };


    enum MGS2_ConfigRegion : uint32_t
    {
        GM_CONFIG_REGION_JAPAN = 0,
        GM_CONFIG_REGION_US = 1,
        GM_CONFIG_REGION_EU = 2,
    };

    enum : uint32_t
    {
        GM_CLEAR_GET_MUGENBANDANA = 1u << 0,	/* Obtained Infinite Bandana */
        GM_CLEAR_GET_MUGENWIG = 1u << 1,	/* Obtained Infinite Wig */
        GM_CLEAR_GET_DGCAMERA = 1u << 2,	/* Obtained Digital Camera */
        GM_CLEAR_GET_O2WIG = 1u << 3,	/* Obtained Infinite O2 Wig */
        GM_CLEAR_GET_ELUDEWIG = 1u << 4,	/* Obtained Infinite Grip Wig */
        GM_CLEAR_SPECIAL_ITEM_USED = 1u << 5,	/* Cleared the game using a special item */

        GM_CLEAR_MUGENBANDANA_USED = 1u << 8,	/* Used Infinite Bandana */
        GM_CLEAR_MUGENWIG_USED = 1u << 9,	/* Used Infinite Wig */
        GM_CLEAR_WIG_A_USED = 1u << 10,	/* Used O2 Wig */
        GM_CLEAR_WIG_B_USED = 1u << 11,	/* Used Grip Wig */
        GM_CLEAR_STEALTH_USED = 1u << 12,	/* Used Stealth */
        GM_CLEAR_RADAR_USED = 1u << 13,	/* Used Radar */
    };
}
