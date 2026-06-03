// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

#include "mgs2_linkvarbuf.hpp"
#include "mgs2_equipment_enums.hpp"
#include "game_stages.hpp"
#include "mgs2_status_flags.hpp"

enum class MGS2GameMode
{
    Unknown,

    Menu,

    Tanker,
    Plant,
    Alternate,

    VRSneaking,
    VRVariety,
    VRFirstPerson,
    VRStreaking,
    VRWeapons
};


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

    void Unset_PL_Status(uint64_t bits) const;
    [[nodiscard]] uint64_t Get_PL_Status() const;
    [[nodiscard]] int Get_GM_GameStatus() const;
    [[nodiscard]] int Get_GM_VRStatus() const;
    [[nodiscard]] MGS2GameMode MGS2_GetGameMode() const;

    [[nodiscard]] int& GV_PauseLevel() const;
    [[nodiscard]] int& GM_MenuStatus() const;

    [[nodiscard]] static constexpr uint32_t GV_StrCode(const char* inputString)
    {
        constexpr uint32_t kBitLength = 24;
        constexpr uint32_t kBitMask = (1u << kBitLength) - 1;

        uint32_t hashValue = 0;

        for (const char* currentChar = inputString; *currentChar != '\0'; ++currentChar)
        {
            const auto characterValue = static_cast<unsigned char>(*currentChar);

            hashValue = ((hashValue << 5) | (hashValue >> (kBitLength - 5)));
            hashValue += characterValue;
            hashValue &= kBitMask;
        }

        return (hashValue == 0) ? 1 : hashValue;
    }

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
    int* p_GM_MenuStatus = nullptr;
    int* GM_GameStatus = nullptr;
    int* GM_VRStatus = nullptr;
    int* p_GV_PauseLevel = nullptr;

};

inline GameVars g_GameVars;


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
