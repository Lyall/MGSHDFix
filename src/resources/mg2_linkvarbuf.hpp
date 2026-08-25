// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once

namespace MG2_LinkVarBuf
{
    inline uintptr_t* stateSlot = nullptr;
    inline uintptr_t* statsBlockAnchor = nullptr;

    template <typename T, uintptr_t Offset>
    struct StateVarValue
    {
        [[nodiscard]] static T* resolve()
        {
            if (!stateSlot || !*stateSlot) return nullptr;
            return reinterpret_cast<T*>(*stateSlot + Offset);
        }

        operator T () const
        {
            T* p = resolve();
            return p ? *p : T{};
        }

        StateVarValue& operator=(const T value)
        {
            if (T* p = resolve()) *p = value;
            return *this;
        }
    };

    template <typename T, int32_t Offset>
    struct StatsVarValue
    {
        operator T& () const
        {
            return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(statsBlockAnchor) + Offset);
        }

        T& get() const
        {
            return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(statsBlockAnchor) + Offset);
        }

        StatsVarValue& operator=(const T value)
        {
            get() = value;
            return *this;
        }
    };

    inline StateVarValue<uint32_t, 0x88> GM_Difficulty;

    inline StatsVarValue<uint32_t, 0>  GM_PlayTime; 
    inline StatsVarValue<uint32_t, 4>  GM_RationUseCount;
    inline StatsVarValue<uint32_t, 8>  GM_KillCount;
    inline StatsVarValue<uint32_t, 12> GM_AlertCount;
    inline StatsVarValue<uint64_t, 16> GM_SpecialItemUsed;
    inline StatsVarValue<uint32_t, 24> GM_ContinueCount;


    inline void Initialize()
    {
        HMODULE mg2Module = GetModuleHandleW(L"mg2.dll");
        if (!mg2Module)
        {
            spdlog::error("MG2_LinkVarBuf::Initialize: mg2.dll module handle not found - called too early?");
            return;
        }

        stateSlot = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg2Module, "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B F8 E8 ?? ?? ?? ?? 48 8B C8", "MG2: stateSlot") + 3));

        statsBlockAnchor = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(mg2Module, "F2 0F 10 0D", "MG2: statsBlockAnchor (GM_PlayTime)") + 11));

        spdlog::info("GameVars: MG2 stateSlot address is mg2.dll+{:X}", (uintptr_t)stateSlot - (uintptr_t)mg2Module);
        spdlog::info("GameVars: MG2 statsBlockAnchor address is mg2.dll+{:X}", (uintptr_t)statsBlockAnchor - (uintptr_t)mg2Module);
    }
}
