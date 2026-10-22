// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

#include "game_funcs.hpp"
#include "mg1_gamevar_enums.hpp"
#include "game_stages.hpp"

namespace MG1_LinkVarBuf
{
    constexpr int nFrameBufferWidth = 256;
    constexpr int nFrameBufferHeight = 256;

    inline uintptr_t* pTextChunkHandle = nullptr;

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
        int anReserved[10];
        int GM_EntranceKind;
    };

    static_assert(offsetof(RoomPosition, GM_EntranceKind) == 0x30);

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

    struct Sprite
    {
        int nFlags;
        int nReserved;
        int nX;
        int nY;
        int nSheet;
        int nAnimation;
        int nPreviousAnimation;
        int nElapsedTicks;
        int nLoops;
        int anTrackFrames[20];
        int anTrackTicks[20];
        int nHidden;
        int nDrawOrder;
        int nPaletteIndex;
        int nPaletteTimer;
        int anTrackX[20];
        int anTrackY[20];
        int anTrackWidth[20];
        int anTrackHeight[20];
    };

    static_assert(sizeof(Sprite) == 0x214);
    static_assert(offsetof(Sprite, anTrackFrames) == 0x24);
    static_assert(offsetof(Sprite, nPaletteIndex) == 0xCC);

    inline DrawLayer* pDrawLayers = nullptr;
    inline uint32_t* pOutputPalette = nullptr;
    inline int* pOutputTopRows = nullptr;

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

    inline constexpr uintptr_t kItemFlagsOffsetInRecord = 0xDC;

    inline void SetItemOwned(MG1ItemIndex eItem, bool bOwned)
    {
        uint8_t* pRecord = GetGameRecord();
        if (!pRecord || eItem > MG1_ITEM_INDEX_CARD_8)
        {
            return;
        }
        const unsigned int nFlag = static_cast<unsigned int>(eItem) + 1;
        auto* pFlags = reinterpret_cast<uint32_t*>(pRecord + kItemFlagsOffsetInRecord);
        const uint32_t nMask = 1u << (nFlag & 7);
        pFlags[nFlag >> 3] = bOwned ? pFlags[nFlag >> 3] | nMask : pFlags[nFlag >> 3] & ~nMask;
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

    enum class InventoryChangeResult
    {
        Changed,
        InventoryFull,
    };

    [[nodiscard]] inline int GetInventoryQuantity(int nId, bool bWeapons)
    {
        if (nId == MG1_ITEM_INDEX_RATIONS)
        {
            return GM_RationCount;
        }
        if (!HasItem(static_cast<MG1ItemIndex>(nId)))
        {
            return 0;
        }
        return bWeapons && nId != MG1_ITEM_INDEX_SUPPRESSOR ? *GetItemStock(static_cast<MG1ItemIndex>(nId)) : 1;
    }

    [[nodiscard]] inline InventoryChangeResult ChangeInventory(int nId, bool bWeapons, bool bAdd)
    {
        const auto eItem = static_cast<MG1ItemIndex>(nId);
        const bool bOwned = nId == MG1_ITEM_INDEX_RATIONS ? GM_RationCount > 0 : HasItem(eItem);
        if (!bAdd && !bOwned)
        {
            return InventoryChangeResult::Changed;
        }
        const bool bCard = nId == MG1_ITEM_INDEX_CARD_1 || nId >= MG1_ITEM_INDEX_CARD_2;
        const int nIcon = bCard ? MG1_ITEM_INDEX_CARD_1 : nId;
        const bool bStock = bWeapons && nId != MG1_ITEM_INDEX_SUPPRESSOR;
        const bool bRation = nId == MG1_ITEM_INDEX_RATIONS;
        int32_t* pSlots = GetItemSlots();
        if (bAdd && !bOwned && !bWeapons && nId != MG1_ITEM_INDEX_CIGARETTES)
        {
            int nSlot = 1;
            while (nSlot < static_cast<int>(kItemSlotCount) && pSlots[nSlot] != 0)
            {
                nSlot++;
            }
            if (nSlot == kItemSlotCount)
            {
                return InventoryChangeResult::InventoryFull;
            }
            pSlots[nSlot] = nIcon;
            if (bRation)
            {
                GM_RationSlotIndex = nSlot;
            }
            if (nId == MG1_ITEM_INDEX_TRANSMITTER)
            {
                GM_TransmitterSlotIndex = nSlot;
            }
        }
        if (bStock)
        {
            int* pStock = GetItemStock(eItem);
            const int nClass = std::clamp(static_cast<int>(GM_Class), 0, 3);
            const int nMultiplier = GM_Difficulty == 0 ? 2 : 1;
            int nMaximum = (nClass + 1) * 5;
            if (nId == MG1_ITEM_INDEX_HANDGUN || nId == MG1_ITEM_INDEX_SUBMACHINE_GUN)
            {
                nMaximum = nClass == 0 ? 50 : nClass * 100;
            }
            else if (nId == MG1_ITEM_INDEX_GRENADE_LAUNCHER)
            {
                nMaximum = nClass == 0 ? 15 : nClass * 30;
            }
            else if (nId == MG1_ITEM_INDEX_ROCKET_LAUNCHER)
            {
                nMaximum = nClass == 0 ? 5 : nClass * 10;
            }
            *pStock = bAdd ? std::min(std::max(0, *pStock) + 1, nMaximum * nMultiplier) : 0;
        }
        if (bRation)
        {
            const int nMaximum = (std::clamp(static_cast<int>(GM_Class), 0, 3) + 1) * (GM_Difficulty == 0 ? 2 : 1) * 3;
            GM_RationCount = bAdd ? std::min(static_cast<int>(GM_RationCount) + 1, nMaximum) : 0;
        }
        SetItemOwned(eItem, bAdd);
        if (!bAdd)
        {
            if (!bWeapons)
            {
                for (size_t nSlot = 1; nSlot < kItemSlotCount; nSlot++)
                {
                    if (pSlots[nSlot] == nIcon)
                    {
                        pSlots[nSlot] = 0;
                        if (bCard)
                        {
                            break;
                        }
                    }
                }
                if (GM_SelectedEquipIconId == nIcon)
                {
                    GM_SelectedEquipIconId = 0;
                }
            }
            else if (GM_SelectedWeaponIconId == nId)
            {
                GM_SelectedWeaponIconId = 0;
            }
            if (bRation)
            {
                GM_RationSlotIndex = 0;
            }
            if (nId == MG1_ITEM_INDEX_TRANSMITTER)
            {
                GM_TransmitterSlotIndex = 0;
            }
        }
        if (bCard)
        {
            int nCards = 0;
            for (int nLevel = 1; nLevel <= MG1_ITEM_INDEX_MAX_CARD_LEVEL; nLevel++)
            {
                nCards += HasItem(GetCardItemIndex(nLevel)) ? 1 : 0;
            }
            GM_CardsHeld = nCards;
            GM_EquippedCardNumber = 0;
            GM_SelectedEquipIconId = 0;
        }
        GM_EquipCursorCol = 0;
        GM_EquipCursorRow = 0;
        GM_WeaponCursorCol = 0;
        GM_WeaponCursorRow = 0;
        return InventoryChangeResult::Changed;
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
        const int nRoom = roomPosition->GM_Room;
        const MGRoomName* pRoom = FindMGRoomName(false, nStage, nRoom);
        if (!pRoom)
        {
            return "Metal Gear: Area " + std::to_string(nStage) + ", Room " + std::to_string(nRoom);
        }
        return "Metal Gear: " + GetMGRoomName(false, nStage, nRoom);
    }

    inline void Initialize()
    {
        //BlitImage+20
        uint8_t* pLayers = Memory::PatternScan(mg1Module, "48 8D 05 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? ?? ?? 48 8B F8", "MG1: BlitImage()+0x20 : draw layers");
        if (pLayers)
        {
            pDrawLayers = reinterpret_cast<DrawLayer*>(Memory::GetRipRelativeAddress(pLayers, 3, 7));
        }
        //PresentIndexedFrame+6E
        uint8_t* pPalette = Memory::PatternScan(mg1Module, "48 8D 15 ?? ?? ?? ?? 66 66 66 0F 1F 84 00", "MG1: PresentIndexedFrame()+0x6E : output palette");
        if (pPalette)
        {
            pOutputPalette = reinterpret_cast<uint32_t*>(Memory::GetRipRelativeAddress(pPalette, 3, 7));
        }
        //PresentIndexedFrame+2A
        uint8_t* pTopRows = Memory::PatternScan(mg1Module, "8B 2D ?? ?? ?? ?? 48 8B F8", "MG1: PresentIndexedFrame()+0x2A : top padding");
        if (pTopRows)
        {
            pOutputTopRows = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(pTopRows, 2, 6));
        }

        //ShowMessage+29
        uint8_t* pTextChunk = Memory::PatternScan(mg1Module, "48 8B 0D ?? ?? ?? ?? 33 D2 41 8B ED", "MG1: ShowMessage()+0x29 : text chunk");
        if (pTextChunk)
        {
            pTextChunkHandle = reinterpret_cast<uintptr_t*>(Memory::GetRipRelativeAddress(pTextChunk, 3, 7));
        }

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
