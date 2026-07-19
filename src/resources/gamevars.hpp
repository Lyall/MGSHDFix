// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

#include "mgs2_linkvarbuf.hpp"
#include "mgs2_equipment_enums.hpp"
#include "game_stages.hpp"
#include "mgs2_status_flags.hpp"

struct alignas(16) FVECTOR { float x, y, z, w; };

struct alignas(16) FMATRIX { float m[4][4]; };

struct alignas(16) DG_CHANL
{
    FMATRIX eye_pers;               // 0x000
    FMATRIX eye_inv;                // 0x040
    FMATRIX eye;                    // 0x080
    FMATRIX pers;                   // 0x0C0
    FMATRIX eye_pers2;              // 0x100
    FMATRIX pers2;                  // 0x140
    FMATRIX raise_pers;             // 0x180
    FMATRIX raise_pers2;            // 0x1C0
    FMATRIX raise_eye_pers;         // 0x200
    FMATRIX raise_eye_pers2;        // 0x240
    FMATRIX pers_no_offset;         // 0x280
    FMATRIX eye_pers_no_offset;     // 0x2C0
    float   screen;                 // 0x300
    int     flag;                   // 0x304
    int     group_id;               // 0x308
    int     _pad0;                  // 0x30C
    void* obj_queue;              // 0x310
    int     n_stage;                // 0x318
    int     _pad1;                  // 0x31C
    void* stage_list;             // 0x320
    int     width;                  // 0x328
    int     height;                 // 0x32C
    int     offset_x;               // 0x330
    int     offset_y;               // 0x334
    uint8_t _draw_env_align[8];     // 0x338
    uint8_t _draw_env[416];         // 0x340  draw_env[2]
    uint8_t _draw_offset[64];       // 0x4E0  draw_offset[2]
    int     bg_clear_flag;          // 0x520
    int     chanl_num;              // 0x524
    int     high_reso;              // 0x528
    int     _pad_end;               // 0x52C
};
static_assert(sizeof(DG_CHANL) == 0x530);
static_assert(offsetof(DG_CHANL, eye_pers) == 0x000);
static_assert(offsetof(DG_CHANL, chanl_num) == 0x524);

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

enum class MGS3GameMode
{
    Unknown,

    Menu,

    VirtuousMission,
    SnakeEater
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
    [[nodiscard]] MGS3GameMode MGS3_GetGameMode() const;

    [[nodiscard]] int& GV_PauseLevel() const;
    [[nodiscard]] int& GM_MenuStatus() const;
    [[nodiscard]] int& DG_Clock() const;

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

    [[nodiscard]] DG_CHANL* DG_Chanl(int i) const { return p_DG_Chanls ? p_DG_Chanls + i : nullptr; }
    [[nodiscard]] int GM_CurrentStageMap() const { return p_GM_CurrentStageMap ? *p_GM_CurrentStageMap : 0; }

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
    int* p_DG_Clock = nullptr;
    DG_CHANL* p_DG_Chanls = nullptr;

    int32_t* p_GM_CurrentStageMap = nullptr;


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
