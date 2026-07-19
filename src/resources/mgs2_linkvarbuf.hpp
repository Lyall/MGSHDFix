// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once


namespace MGS2_LinkVarBuf
{
    inline uintptr_t* linkvarbuf = nullptr;

    template <typename T, uintptr_t Offset>
    struct LinkVarValue
    {
        operator T& () const
        {
            return *reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        T& get() const
        {
            return *reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        LinkVarValue& operator=(const T value)
        {
            get() = value;
            return *this;
        }
    };

    template <typename T, uintptr_t Offset>
    struct LinkVarPointer
    {
        operator T* () const
        {
            return reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        T* get() const
        {
            return reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        T& operator[](const size_t index) const
        {
            return get()[index];
        }
    };


    enum MGS2_ConfigFlags : uint32_t
    {
        GM_CONFIG_VIBRATION_OFF = (1u << 0),
        GM_CONFIG_CAPTION_OFF = (1u << 1),
        GM_CONFIG_RADAR_OFF = (1u << 2),
        GM_CONFIG_BLOOD_OFF = (1u << 3),
        GM_CONFIG_CUTSCENES_LETTERBOXED = (1u << 4),
        GM_CONFIG_RADAR_OFF_INTRUDE = (1u << 5),
        GM_CONFIG_SHUKAN_REVERSE = (1u << 6),   /// First-person controls inverted
        GM_CONFIG_OLD_TYPE_MENU = (1u << 7),
        GM_CONFIG_WATERUD_REVERSE = (1u << 8),   /// Underwater up/down inverted
        GM_CONFIG_MENU_QCHANGE_EX = (1u << 9),
        GM_CONFIG_SOUND_5_1CHANL = (1u << 10),  /// 5.1ch surround sound
        GM_CONFIG_END_IF_FOUND = (1u << 11),  /// Game over if discovered

        GM_CONFIG_STORY_TANKER = (1u << 12),  /// Tanker chapter; if unset, Plant chapter
        GM_CONFIG_TANKER_CLEARED = (1u << 13),  /// Tanker chapter cleared this playthrough
        GM_CONFIG_PLAYTIME_STOP = (1u << 14),  /// Playtime counter stopped
    };

    inline LinkVarValue<short, 0>       GM_GameClearCount;
    inline LinkVarValue<short, 2>       GM_TankerClearCount;
    inline LinkVarValue<short, 4>       GM_PlantClearCount;
    inline LinkVarValue<short, 6>       GM_Configuration;
    inline LinkVarValue<int, 8>         GM_Configuration2;
    inline LinkVarValue<int, 12>        GM_VRConfiguration;
    inline LinkVarValue<short, 16>      GM_GameLevel;
    inline LinkVarValue<short, 18>      GM_Result;
    inline LinkVarValue<short, 20>      GM_Language;
    inline LinkVarValue<short, 22>      GM_ClearFlag;
    inline LinkVarValue<short, 24>      GM_ScrAdjX;
    inline LinkVarValue<short, 26>      GM_ScrAdjY;
    inline LinkVarPointer<char, 28>     GM_SaveResidentDir;
    inline LinkVarPointer<char, 44>     GM_SaveAreaDir;
    inline LinkVarPointer<int, 60>      GM_DogTagFlag;
    inline LinkVarValue<int, 188>       GM_SaveArea;
    inline LinkVarValue<int, 192>       GM_SaveMap;
    inline LinkVarPointer<int, 196>     GM_AreaHistory;
    inline LinkVarValue<int, 212>       GM_PrevArea;
    inline LinkVarValue<int, 216>       GM_SaveX;
    inline LinkVarValue<int, 220>       GM_SaveY;
    inline LinkVarValue<int, 224>       GM_SaveZ;
    inline LinkVarValue<int, 228>       GM_StagePlayTime;
    inline LinkVarValue<int, 232>       GM_PlayerPosX;  ///east / west
    inline LinkVarValue<int, 236>       GM_PlayerPosY;  ///Height
    inline LinkVarValue<int, 240>       GM_PlayerPosZ;  ///north / south
    inline LinkVarValue<short, 244>     GM_PlayerDir;
    inline LinkVarValue<short, 246>     GM_PlayerMotion;
    inline LinkVarValue<short, 248>     GM_BehindRot;
    inline LinkVarValue<short, 250>     GM_Vitality;
    inline LinkVarValue<short, 252>     GM_VitalityMax;
    inline LinkVarValue<short, 254>     GM_O2;
    inline LinkVarValue<short, 256>     GM_O2Max;
    inline LinkVarValue<short, 258>     GM_PlayerStance;
    inline LinkVarValue<short, 260>     GM_Weapon;
    inline LinkVarValue<short, 262>     GM_Item;
    inline LinkVarValue<short, 264>     GM_PlayerCold;
    inline LinkVarValue<short, 266>     GM_PlayerSneezeTime;
    inline LinkVarValue<int, 268>       GM_PlayerColdCount;
    inline LinkVarValue<int, 272>       GM_PlayerColdStartTime;
    inline LinkVarValue<short, 276>     GM_PlayerStateFlag;
    inline LinkVarValue<short, 278>     GM_WeaponPrev;
    inline LinkVarValue<short, 280>     GM_ItemPrev;
    inline LinkVarValue<short, 282>     GM_AlertMode;
    inline LinkVarValue<short, 284>     GM_StartAlertMode;
    inline LinkVarPointer<short, 286>   GM_SnakeGripMax;
    inline LinkVarPointer<short, 294>   GM_RaidenGripMax;
    inline LinkVarValue<short, 302>     GM_SnakeChin_Up;
    inline LinkVarValue<short, 304>     GM_RaidenChin_Up;
    inline LinkVarValue<short, 306>     GM_ContinueCount;
    inline LinkVarValue<short, 308>     GM_GameOverCount;
    inline LinkVarValue<short, 310>     GM_SaveCount;
    inline LinkVarValue<int, 312>       GM_PlayTime;
    inline LinkVarValue<int, 316>       GM_LastSave;
    inline LinkVarValue<short, 320>     GM_ShootCount;
    inline LinkVarValue<short, 322>     GM_AlertCount;
    inline LinkVarValue<short, 324>     GM_KillCount;
    inline LinkVarValue<short, 326>     GM_DamageCount;
    inline LinkVarPointer<int, 328>     GM_ClearCode;
    inline LinkVarValue<short, 344>     GM_MecaKillCount;
    inline LinkVarValue<short, 346>     Padding_Dummy;
    inline LinkVarPointer<short, 348>   GM_Weapons;
    inline LinkVarPointer<short, 420>   GM_WeaponsMax;
    inline LinkVarPointer<short, 492>   GM_Items;
    inline LinkVarPointer<short, 588>   GM_ItemsMax;
    inline LinkVarPointer<short, 684>   GM_WeaponsR;
    inline LinkVarPointer<short, 756>   GM_WeaponsMaxR;
    inline LinkVarPointer<short, 828>   GM_ItemsR;
    inline LinkVarPointer<short, 924>   GM_ItemsMaxR;
    inline LinkVarPointer<short, 1020>  GM_WeaponsSaved;
    inline LinkVarPointer<short, 1092>  GM_ItemsSaved;
    /// east / west
    inline LinkVarValue<int, 1188>      GM_CameraX; 
    /// height
    inline LinkVarValue<int, 1192>      GM_CameraY;
    /// north / south
    inline LinkVarValue<int, 1196>      GM_CameraZ; 
    inline LinkVarValue<int, 1200>      GM_CamTargX;
    inline LinkVarValue<int, 1204>      GM_CamTargY;
    inline LinkVarValue<int, 1208>      GM_CamTargZ;
    inline LinkVarValue<int, 1212>      GM_CamRotX;
    inline LinkVarValue<int, 1216>      GM_CamRotY;
    inline LinkVarValue<int, 1220>      GM_AlertLevel;
    inline LinkVarValue<short, 1224>    GM_LastCodecFreq;
    inline LinkVarValue<short, 1226>    GM_GlobalLoadCount;
    inline LinkVarPointer<int, 1228>    ENEMEM_EneMem;
    inline LinkVarValue<short, 5452>    GM_ResetLoadCount;
    inline LinkVarValue<short, 5454>    ENEMEM_CurrentNum;
    inline LinkVarValue<int, 5456>      GM_TnkerCamStatus;
    inline LinkVarPointer<char, 5460>   GM_MyName;
    inline LinkVarValue<int, 5480>      GM_MySexData;
    inline LinkVarValue<int, 5484>      GM_MyYearData;
    inline LinkVarValue<int, 5488>      GM_MyMonthData;
    inline LinkVarValue<int, 5492>      GM_MyDayData;
    inline LinkVarValue<int, 5496>      GM_MyBloodData;
    inline LinkVarValue<int, 5500>      GM_MyRegionData;
    inline LinkVarValue<int, 5504>      GM_StageBreakPoint;
    inline LinkVarValue<int, 5508>      GM_SelectStageLimit;
    inline LinkVarValue<short, 5512>    GM_StageNum;
    inline LinkVarValue<short, 5514>    GM_TitleMenuStatus;
    inline LinkVarValue<short, 5516>    GM_ShipwormFlag;
    inline LinkVarValue<short, 5518>    GM_ShipwormCorrode;
    inline LinkVarValue<short, 5520>    GM_RationUseCount;
    inline LinkVarValue<short, 5522>    GM_ClearingCount;
    inline LinkVarValue<short, 5524>    GM_RedFindCount;
    inline LinkVarValue<short, 5526>    GM_ClearCodeFlag;
}

