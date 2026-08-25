// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once


namespace MG1_LinkVarBuf
{
    inline uintptr_t* gameStatsAnchor = nullptr;
    inline uintptr_t* runtimeStateHandleSlot = nullptr;

    template <typename T, int32_t Offset>
    struct StatsVarValue
    {
        operator T& () const
        {
            return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(gameStatsAnchor) + Offset);
        }

        T& get() const
        {
            return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(gameStatsAnchor) + Offset);
        }

        StatsVarValue& operator=(const T value)
        {
            get() = value;
            return *this;
        }
    };

    template <typename T, uintptr_t Offset>
    struct RuntimeVarValue
    {
        [[nodiscard]] static T* resolve()
        {
            if (!runtimeStateHandleSlot) return nullptr;
            const uintptr_t handle = *runtimeStateHandleSlot;
            if (!handle) return nullptr;
            const uintptr_t payload = *reinterpret_cast<uintptr_t*>(handle);
            if (!payload) return nullptr;
            return reinterpret_cast<T*>(payload + Offset);
        }

        operator T () const
        {
            T* p = resolve();
            return p ? *p : T{};
        }

        RuntimeVarValue& operator=(const T value)
        {
            if (T* p = resolve()) *p = value;
            return *this;
        }
    };


    inline StatsVarValue<uint32_t, -196> GM_Difficulty;      // inside a scratch buffer
    inline StatsVarValue<uint32_t, 0>    GM_PlayTime;        // snapshot, NOT the live counter - see GM_LivePlayTime
    inline StatsVarValue<uint32_t, 4>    GM_RationUseCount;
    inline StatsVarValue<uint32_t, 8>    GM_KillCount;
    inline StatsVarValue<uint32_t, 12>   GM_AlertCount;
    inline StatsVarValue<uint64_t, 16>   GM_SpecialItemUsed;
    inline StatsVarValue<uint32_t, 24>   GM_ContinueCount;

    inline RuntimeVarValue<int32_t, 0x188> GM_LivePlayTime;       // 15 ticks/real-second, confirmed
    inline RuntimeVarValue<int32_t, 0x00>  GM_EquipCursorCol;     // 0..5
    inline RuntimeVarValue<int32_t, 0x0C>  GM_EquipCursorRow;     // 0..4
    inline RuntimeVarValue<int32_t, 0x04>  GM_WeaponCursorCol;    // 0..2
    inline RuntimeVarValue<int32_t, 0x10>  GM_WeaponCursorRow;    // 0..2 (col2 clamped to 0..1)
    inline RuntimeVarValue<int32_t, 0x30>  GM_SelectedEquipIconId;
    inline RuntimeVarValue<int32_t, 0x34>  GM_SelectedWeaponIconId; // icon id; weapon slot = (this - 18), 0-6


    inline void Initialize()
    {
        HMODULE mg1Module = GetModuleHandleW(L"mg1.dll");
        if (!mg1Module)
        {
            spdlog::error("MG1_LinkVarBuf::Initialize: mg1.dll module handle not found - called too early?");
            return;
        }

        gameStatsAnchor = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "48 63 05 ?? ?? ?? ?? 48 6B C8", "MG1: GameStats anchor (GM_PlayTime)") + 3));

        runtimeStateHandleSlot = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg1Module, "E8 ?? ?? ?? ?? 45 33 C0 48 8D 0D ?? ?? ?? ?? C7 40", "MG1: runtimeStateHandleSlot") + 11));

        spdlog::info("GameVars: MG1 gameStatsAnchor address is mg1.dll+{:X}", (uintptr_t)gameStatsAnchor - (uintptr_t)mg1Module);
        spdlog::info("GameVars: MG1 runtimeStateHandleSlot address is mg1.dll+{:X}", (uintptr_t)runtimeStateHandleSlot - (uintptr_t)mg1Module);
    }

}
