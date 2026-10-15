// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

#include "game_funcs.hpp"
#include "mg2_equipment_enums.hpp"
#include "mg2_gamevar_enums.hpp"
#include "game_stages.hpp"

namespace MG2_LinkVarBuf
{
    struct PlayStats
    {
        int GM_PlayTime;
        int GM_RationUseCount;
        int GM_KillCount;
        int GM_AlertCount;
        int GM_SpecialItemUsed;
        int GM_SaveCount;
        int GM_ContinueCount;
    };

    static_assert(sizeof(PlayStats) == 0x1C, "PlayStats size");

    struct DoorState
    {
        int anReserved[4];
        int nCardLevel;
    };

    static_assert(offsetof(DoorState, nCardLevel) == 0x10);

    struct Sprite
    {
        int nFlags;
        int _pad1;
        int nX;
        int nY;
        int nSheet;
        int nAnim;
        int nTime;
        int nLoopsDone;
        int nShown;
        int nDrawOrder;
    };

    inline uintptr_t* stateSlot = nullptr;
    inline PlayStats* playStats = nullptr;
    inline int32_t* pScriptGlobals = nullptr;
    inline uintptr_t* pSpriteLists = nullptr;
    inline int* pTextFade = nullptr;


    template <typename T, uintptr_t Offset>
    struct StateVarValue
    {
        [[nodiscard]] static T* resolve()
        {
            if (!stateSlot || !*stateSlot)
            {
                return nullptr;
            }

            return reinterpret_cast<T*>(*stateSlot + Offset);
        }

        operator T () const
        {
            T* p = resolve();
            return p ? *p : T{};
        }

        StateVarValue& operator=(const T value)
        {
            if (T* p = resolve())
            {
                *p = value;
            }
            return *this;
        }
    };

    template <int Index>
    struct ScriptGlobalValue
    {
        operator int32_t () const
        {
            return pScriptGlobals ? pScriptGlobals[Index] : 0;
        }

        ScriptGlobalValue& operator=(const int32_t nValue)
        {
            if (pScriptGlobals)
            {
                pScriptGlobals[Index] = nValue;
            }
            return *this;
        }
    };

    inline StateVarValue<uint32_t, 0x88> GM_Difficulty;

    inline ScriptGlobalValue<1>    GM_Scene;
    inline ScriptGlobalValue<2>    GM_NextScene;
    inline ScriptGlobalValue<8>    GM_GameplayHalt;
    inline ScriptGlobalValue<25>   GM_MessageId;
    inline ScriptGlobalValue<26>   GM_MessageSub;
    inline ScriptGlobalValue<27>   GM_MessagePos;
    inline ScriptGlobalValue<28>   GM_MessageCol;
    inline ScriptGlobalValue<29>   GM_MessageRow;
    inline ScriptGlobalValue<30>   GM_MessageSpeed;
    inline ScriptGlobalValue<31>   GM_MessageBlink;
    inline ScriptGlobalValue<40>   GM_MessageDone;
    inline ScriptGlobalValue<49>   GM_RoomX;
    inline ScriptGlobalValue<50>   GM_RoomY;
    inline ScriptGlobalValue<51>   GM_Area;
    inline ScriptGlobalValue<52>   GM_Room;
    inline ScriptGlobalValue<55>   GM_RoomLighting;
    inline ScriptGlobalValue<56>   GM_AlertRequest;
    inline ScriptGlobalValue<57>   GM_AlertMode;
    inline ScriptGlobalValue<58>   GM_PreviousAlertMode;
    inline ScriptGlobalValue<59>   GM_AlertTimer;
    inline ScriptGlobalValue<64>   GM_AlertCountdown;
    inline ScriptGlobalValue<66>   GM_SoldiersInRoom;
    inline ScriptGlobalValue<67>   GM_SoldiersDowned;
    inline ScriptGlobalValue<73>   GM_ScriptedAlarm;
    inline ScriptGlobalValue<77>   GM_SnakeRoomX;
    inline ScriptGlobalValue<78>   GM_SnakeRoomY;
    inline ScriptGlobalValue<79>   GM_SnakeRoom;
    inline ScriptGlobalValue<219>  GM_ScriptedCall;
    inline ScriptGlobalValue<222>  GM_Frequency;
    inline ScriptGlobalValue<223>  GM_ContactSlot;
    inline ScriptGlobalValue<224>  GM_ReplyIndex;
    inline ScriptGlobalValue<226>  GM_ReplyBlock;
    inline ScriptGlobalValue<227>  GM_SnakePortraitFrame;
    inline ScriptGlobalValue<228>  GM_ContactPortraitFrame;
    inline ScriptGlobalValue<231>  GM_SpeakerIsSnake;

    [[nodiscard]] inline int GetMasterCardForLevel(int nLevel)
    {
        return nLevel <= 3 ? MG2_ITEM_INDEX_MASTER_CARD_LOW : nLevel <= 6 ? MG2_ITEM_INDEX_MASTER_CARD_MID : MG2_ITEM_INDEX_MASTER_CARD_HIGH;
    }

    [[nodiscard]] inline bool HasCardForLevel(int nLevel)
    {
        return MG2_Gamefuncs::HasItem(MG2_ITEM_INDEX_CARD_1 + nLevel - 1) || MG2_Gamefuncs::HasItem(GetMasterCardForLevel(nLevel));
    }

    [[nodiscard]] inline Sprite* GetSprite(int nList, int nSlot)
    {
        if (!pSpriteLists || nList < 0 || nSlot < 0)
        {
            return nullptr;
        }

        const uintptr_t hList = pSpriteLists[nList];
        if (!Memory::IsReadable(reinterpret_cast<void*>(hList), sizeof(uintptr_t)))
        {
            return nullptr;
        }

        const uintptr_t* pSlots = *reinterpret_cast<uintptr_t**>(hList);
        if (!Memory::IsReadable(pSlots + nSlot, sizeof(uintptr_t)))
        {
            return nullptr;
        }

        const uintptr_t hSprite = pSlots[nSlot];
        if (!Memory::IsReadable(reinterpret_cast<void*>(hSprite), sizeof(uintptr_t)))
        {
            return nullptr;
        }

        return *reinterpret_cast<Sprite**>(hSprite);
    }

    [[nodiscard]] inline bool IsAlert()
    {
        return GM_AlertMode == MG2_ALERT_MODE_ALERT;
    }

    [[nodiscard]] inline bool IsArea(int nArea)
    {
        return GM_Area == nArea;
    }

    [[nodiscard]] inline std::string GetRichPresenceString()
    {
        if (GM_Scene == MG2_SCENE_RESULTS)
        {
            return "Metal Gear 2: Results";
        }

        const int nArea = GM_Area;
        if (nArea < 0 || nArea >= static_cast<int>(std::size(g_MG2AreaNames)))
        {
            return "Unknown Area (" + std::to_string(nArea) + ")";
        }

        if (!*g_MG2AreaNames[nArea])
        {
            return "Metal Gear 2: Area " + std::to_string(nArea);
        }

        return std::string("Metal Gear 2: ") + g_MG2AreaNames[nArea];
    }

    inline void Initialize()
    {
        stateSlot = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg2Module, "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B F8 E8 ?? ?? ?? ?? 48 8B C8", "MG2: stateSlot") + 3));

        playStats = reinterpret_cast<PlayStats*>(Memory::GetRelativeOffset(Memory::PatternScan(mg2Module, "F2 0F 10 0D", "MG2: playStats (GM_PlayTime)") + 11));

        pScriptGlobals = reinterpret_cast<int32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg2Module, "4C 8D 25 ?? ?? ?? ?? 8D 42", "MG2: pScriptGlobals (GM_Scene)") + 3));

        pSpriteLists = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg2Module, "48 8D 05 ?? ?? ?? ?? 48 63 DA", "MG2: pSpriteLists (GetSprite)") + 3));

        uint8_t* pTextFadeScan = Memory::PatternScan(mg2Module, "44 8B 0D ?? ?? ?? ?? 41 81 F9", "MG2: text layer fade");
        if (pTextFadeScan)
        {
            pTextFade = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(pTextFadeScan, 3, 7));
        }

        spdlog::info("GameVars: MG2 stateSlot address is mg2.dll+{:X}", (uintptr_t)stateSlot - (uintptr_t)mg2Module);
        spdlog::info("GameVars: MG2 playStats address is mg2.dll+{:X}", (uintptr_t)playStats - (uintptr_t)mg2Module);
        spdlog::info("GameVars: MG2 pScriptGlobals address is mg2.dll+{:X}", (uintptr_t)pScriptGlobals - (uintptr_t)mg2Module);
        spdlog::info("GameVars: MG2 pSpriteLists address is mg2.dll+{:X}", (uintptr_t)pSpriteLists - (uintptr_t)mg2Module);
    }
}
