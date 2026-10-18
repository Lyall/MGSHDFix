// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

#include "game_funcs.hpp"
#include "mg1_gamevar_enums.hpp"
#include "game_stages.hpp"

namespace MG1_LinkVarBuf
{
    struct PlayStats
    {
        int GM_PlayTime;           // 15 tick/second
        int GM_RationUseCount;
        int GM_KillCount;
        int GM_AlertCount;
        int GM_SpecialItemUsed;
        int GM_SaveCount;
        int GM_ContinueCount;
    };

    static_assert(sizeof(PlayStats) == 0x1C, "PlayStats size");

    struct RoomPosition
    {
        int GM_Stage;
        int GM_Room;
    };

    struct SceneState
    {
        int GM_SceneChangePending;
        int GM_CurrentScene;
        int GM_RequestedScene;
    };

    struct DrawLayer
    {
        uintptr_t hPixels;
        int nWidth;
        int nHeight;
        int nColour;
        int nClipLeft;
        int nClipTop;
        int nClipRight;
        int nClipBottom;
        int nReserved;
    };

    static_assert(sizeof(DrawLayer) == 0x28);

    inline int32_t* pScriptGlobals = nullptr;
    inline int32_t* pMessageLength = nullptr;
    inline PlayStats* playStats = nullptr;
    inline RoomPosition* roomPosition = nullptr;
    inline SceneState* sceneState = nullptr;
    inline uintptr_t* alertStateHandleSlot = nullptr;
    inline int32_t* pTelescopeMode = nullptr;
    inline int32_t* pBossSurvival = nullptr;
    inline int32_t* pGuardCount = nullptr;
    inline int* pTextFade = nullptr;

    template <int32_t*& Ptr>
    struct GlobalValue
    {
        operator int32_t () const
        {
            return Ptr ? *Ptr : 0;
        }

        GlobalValue& operator=(const int32_t nValue)
        {
            if (Ptr)
            {
                *Ptr = nValue;
            }
            return *this;
        }
    };

    inline GlobalValue<pTelescopeMode> GM_TelescopeMode;   // 0 off, 1 -> 4 the direction shown on the HUD
    inline GlobalValue<pBossSurvival>  GM_BossSurvival;
    inline GlobalValue<pGuardCount>    GM_GuardCount;

    template <size_t nIndex>
    struct ScriptGlobalValue
    {
        operator int32_t () const
        {
            return pScriptGlobals ? pScriptGlobals[nIndex] : 0;
        }

        ScriptGlobalValue& operator=(const int32_t nValue)
        {
            if (pScriptGlobals)
            {
                pScriptGlobals[nIndex] = nValue;
            }
            return *this;
        }
    };

    inline ScriptGlobalValue<150> GM_CurrentDrawLayer;
    inline ScriptGlobalValue<199> GM_MessagePos;
    inline ScriptGlobalValue<202> GM_MessageDone; // Waiting at the end of a page or message.
    inline ScriptGlobalValue<205> GM_MessageActive;
    inline ScriptGlobalValue<209> GM_MessageInstant;

    inline constexpr uintptr_t kPlayStatsOffsetInRecord = 0x188;

    [[nodiscard]] inline uint8_t* GetGameRecord()
    {
        if (!playStats)
        {
            return nullptr;
        }
        return reinterpret_cast<uint8_t*>(playStats) - kPlayStatsOffsetInRecord;
    }

    template <typename T, uintptr_t Offset>
    struct RecordVarValue
    {
        [[nodiscard]] static T* resolve()
        {
            uint8_t* pRecord = GetGameRecord();
            return pRecord ? reinterpret_cast<T*>(pRecord + Offset) : nullptr;
        }

        operator T () const
        {
            T* p = resolve();
            return p ? *p : T{};
        }

        RecordVarValue& operator=(const T value)
        {
            if (T* p = resolve())
            {
                *p = value;
            }
            return *this;
        }
    };

    template <typename T, uintptr_t Offset>
    struct AlertStateVarValue
    {
        [[nodiscard]] static T* resolve()
        {
            if (!alertStateHandleSlot)
            {
                return nullptr;
            }

            const uintptr_t hNode = *alertStateHandleSlot;
            if (!hNode)
            {
                return nullptr;
            }

            const uintptr_t pPayload = *reinterpret_cast<uintptr_t*>(hNode);
            if (!pPayload)
            {
                return nullptr;
            }

            return reinterpret_cast<T*>(pPayload + Offset);
        }

        operator T () const
        {
            T* p = resolve();
            return p ? *p : T{};
        }
    };

    inline AlertStateVarValue<int32_t, 0x00> GM_AlertPhase;

    inline RecordVarValue<int32_t, 0x00>  GM_EquipCursorCol;     // 0 -> 5
    inline RecordVarValue<int32_t, 0x04>  GM_WeaponCursorCol;    // 0 -> 2
    inline RecordVarValue<int32_t, 0x0C>  GM_EquipCursorRow;     // 0 -> 4
    inline RecordVarValue<int32_t, 0x10>  GM_WeaponCursorRow;    // 0 -> 2 (col2 clamped to 0 -> 1)
    inline RecordVarValue<int32_t, 0x1C>  GM_Life;
    inline RecordVarValue<int32_t, 0x2C>  GM_Class;
    inline RecordVarValue<int32_t, 0x30>  GM_SelectedEquipIconId;
    inline RecordVarValue<int32_t, 0x34>  GM_SelectedWeaponIconId; // icon id; weapon slot = (this - 18), 0 -> 6
    inline RecordVarValue<int32_t, 0xB0>  GM_EquippedCardNumber;    // position of the equipped card among the cards held
    inline RecordVarValue<int32_t, 0xB4>  GM_CardsHeld;
    inline RecordVarValue<int32_t, 0xB8>  GM_RationCount;
    inline RecordVarValue<int32_t, 0xBC>  GM_RationSlotIndex;
    inline RecordVarValue<int32_t, 0xC0>  GM_TransmitterSlotIndex;
    inline RecordVarValue<uint32_t, 0xC4> GM_Difficulty;

    // slot 0 is never filled
    inline constexpr size_t kItemSlotCount = 30;
    inline constexpr uintptr_t kItemSlotsOffsetInRecord = 0x38;

    [[nodiscard]] inline int32_t* GetItemSlots()
    {
        uint8_t* pRecord = GetGameRecord();
        return pRecord ? reinterpret_cast<int32_t*>(pRecord + kItemSlotsOffsetInRecord) : nullptr;
    }

    inline constexpr uintptr_t kItemStocksOffsetInRecord = 0x124;

    // one int per item index, weapons (21 -> 25) and ammo-based items
    [[nodiscard]] inline int32_t* GetItemStock(MG1ItemIndex eItem)
    {
        uint8_t* pRecord = GetGameRecord();
        return pRecord ? reinterpret_cast<int32_t*>(pRecord + kItemStocksOffsetInRecord + eItem * sizeof(int32_t)) : nullptr;
    }

    [[nodiscard]] inline bool HasItem(MG1ItemIndex eItem)
    {
        return MG1_Gamefuncs::TestFlag(eItem + 1);
    }

    [[nodiscard]] inline bool IsFlagSet(MG1Flag eFlag)
    {
        return MG1_Gamefuncs::TestFlag(eFlag);
    }

    [[nodiscard]] inline MG1ItemIndex GetCardItemIndex(int nLevel)
    {
        return nLevel == 1 ? MG1_ITEM_INDEX_CARD_1 : static_cast<MG1ItemIndex>(MG1_ITEM_INDEX_CARD_2 + nLevel - 2);
    }

    [[nodiscard]] inline int GetHighestCardLevel()
    {
        for (int nLevel = MG1_ITEM_INDEX_MAX_CARD_LEVEL; nLevel >= 1; --nLevel)
        {
            if (HasItem(GetCardItemIndex(nLevel)))
            {
                return nLevel;
            }
        }
        return 0;
    }

    [[nodiscard]] inline bool IsAlert()
    {
        return GM_AlertPhase != MG1_ALERT_PHASE_NONE;
    }

    [[nodiscard]] inline bool IsStage(MG1Stage eStage)
    {
        return roomPosition && roomPosition->GM_Stage == eStage;
    }

    [[nodiscard]] inline bool IsAnyStage(std::initializer_list<MG1Stage> stages)
    {
        return std::any_of(stages.begin(), stages.end(), [](MG1Stage eStage) { return IsStage(eStage); });
    }

    [[nodiscard]] inline std::string GetRichPresenceString()
    {
        if (sceneState)
        {
            switch (sceneState->GM_CurrentScene)
            {
            case MG1_SCENE_TITLE_MENU:
                return "Metal Gear: Title Screen";
            case MG1_SCENE_ENDING:
                return "Metal Gear: Ending";
            case MG1_SCENE_RESULTS:
                return "Metal Gear: Results";
            default:
                break;
            }
        }

        if (!roomPosition)
        {
            return "Unknown Stage";
        }

        const int nStage = roomPosition->GM_Stage;
        if (nStage < 0 || nStage >= static_cast<int>(std::size(g_MG1StageNames)))
        {
            return "Unknown Stage (" + std::to_string(nStage) + ")";
        }

        if (!*g_MG1StageNames[nStage])
        {
            return "Metal Gear: Stage " + std::to_string(nStage);
        }

        return std::string("Metal Gear: ") + g_MG1StageNames[nStage];
    }

    inline void Initialize()
    {
        //sub_18001B2A0+D12
        uint8_t* pScriptGlobalsScan = Memory::PatternScan(mg1Module, "48 8D 0D ?? ?? ?? ?? 8B 35", "MG1: sub_18001B2A0()+0xD12 : script globals");
        if (pScriptGlobalsScan)
        {
            pScriptGlobals = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(pScriptGlobalsScan, 3, 7));
        }

        //ShowMessage+89
        uint8_t* pMessageLengthScan = Memory::PatternScan(mg1Module, "89 0D ?? ?? ?? ?? 41 3B FE", "MG1: ShowMessage()+0x89 : message length");
        if (pMessageLengthScan)
        {
            pMessageLength = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(pMessageLengthScan, 2, 6));
        }

        uint8_t* pTextFadeScan = Memory::PatternScan(mg1Module, "44 8B 0D ?? ?? ?? ?? 41 81 F9", "MG1: ComposeFrame() : text layer fade");
        if (pTextFadeScan)
        {
            pTextFade = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(pTextFadeScan, 3, 7));
        }

        playStats = reinterpret_cast<PlayStats*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "48 63 05 ?? ?? ?? ?? 48 6B C8", "MG1: playStats (GM_PlayTime)") + 3));

        roomPosition = reinterpret_cast<RoomPosition*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "8B 15 ?? ?? ?? ?? 48 8B C8", "MG1: roomPosition (GM_Stage)") + 2));

        sceneState = reinterpret_cast<SceneState*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "39 1D ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 8B 0D", "MG1: sceneState (GM_SceneChangePending)") + 2));

        alertStateHandleSlot = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 38", "MG1: alertStateHandleSlot (GM_AlertPhase)") + 3));

        pTelescopeMode = reinterpret_cast<int32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "8B 05 ?? ?? ?? ?? 83 F8 01", "MG1: pTelescopeMode (GM_TelescopeMode)") + 2));

        pBossSurvival = reinterpret_cast<int32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "39 1D ?? ?? ?? ?? 8B CB", "MG1: pBossSurvival (GM_BossSurvival)") + 2));

        pGuardCount = reinterpret_cast<int32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "8B 15 ?? ?? ?? ?? 03 CA", "MG1: pGuardCount (GM_GuardCount)") + 2));

        spdlog::info("GameVars: MG1 playStats address is mg1.dll+{:X}", (uintptr_t)playStats - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 roomPosition address is mg1.dll+{:X}", (uintptr_t)roomPosition - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 sceneState address is mg1.dll+{:X}", (uintptr_t)sceneState - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 alertStateHandleSlot address is mg1.dll+{:X}", (uintptr_t)alertStateHandleSlot - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 pTelescopeMode address is mg1.dll+{:X}", (uintptr_t)pTelescopeMode - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 pBossSurvival address is mg1.dll+{:X}", (uintptr_t)pBossSurvival - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 pGuardCount address is mg1.dll+{:X}", (uintptr_t)pGuardCount - (uintptr_t)mg1Module);
    }
}
