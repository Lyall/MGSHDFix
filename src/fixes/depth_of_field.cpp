#include "stdafx.h"

#include "common.hpp"
#include "depth_of_field.hpp"
#include "helper.hpp"
#include "logging.hpp"

namespace
{
    constexpr int kRegUvOffset0 = 96;
    constexpr int kRegUvOffset3 = 99;

    SafetyHookInline SetVertexRegistersHook {};
    SafetyHookMid FarFocusBlurBeginHook {};
    SafetyHookMid FarFocusBlurEndHook {};
    thread_local bool gInsideFarFocusBlur = false;

    bool LooksLikeBlurUvOffset(const float* regs)
    {
        if (!regs)
        {
            return false;
        }

        float maxAbs = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            if (!std::isfinite(regs[i]))
            {
                return false;
            }

            maxAbs = std::max(maxAbs, std::abs(regs[i]));
        }

        return maxAbs > 0.000001f && maxAbs <= 0.08f;
    }

    void __fastcall SetVertexRegisters_Hook(void* backend, int startRegister, int numVectors, const float* regs)
    {
        thread_local float scaledRegs[4];
        static bool loggedFirstHit = false;

        if (gInsideFarFocusBlur &&
            startRegister >= kRegUvOffset0 &&
            startRegister <= kRegUvOffset3 &&
            numVectors == 1 &&
            LooksLikeBlurUvOffset(regs))
        {
            std::copy(regs, regs + 4, scaledRegs);
            for (float& value : scaledRegs)
            {
                value *= g_DepthOfFieldFixes.fBlurUvMultiplier;
            }

            if (!loggedFirstHit)
            {
                loggedFirstHit = true;
                spdlog::info("MGS 2: Depth of Field: far-focus blur UV register upload intercepted. reg={}, in=({}, {}, {}, {}), out=({}, {}, {}, {}).",
                             startRegister,
                             regs[0], regs[1], regs[2], regs[3],
                             scaledRegs[0], scaledRegs[1], scaledRegs[2], scaledRegs[3]);
            }

            SetVertexRegistersHook.fastcall<void>(backend, startRegister, numVectors, scaledRegs);
            return;
        }

        SetVertexRegistersHook.fastcall<void>(backend, startRegister, numVectors, regs);
    }

    void InstallFarFocusBlurScopeHooks()
    {
        uint8_t* farFocusBlurCallSetup = Memory::PatternScan(
            baseModule,
            "F3 44 0F 11 44 24 ?? E8 ?? ?? ?? ?? 48 8B 0D",
            "MGS 2: Depth of Field: far focus blur scope");

        if (!farFocusBlurCallSetup)
        {
            return;
        }

        FarFocusBlurBeginHook = safetyhook::create_mid(farFocusBlurCallSetup, [](SafetyHookContext&) {
            gInsideFarFocusBlur = true;
        });
        LOG_HOOK(FarFocusBlurBeginHook, "MGS 2: Depth of Field: far focus blur scope begin")

        FarFocusBlurEndHook = safetyhook::create_mid(farFocusBlurCallSetup + 0x0C, [](SafetyHookContext&) {
            gInsideFarFocusBlur = false;
        });
        LOG_HOOK(FarFocusBlurEndHook, "MGS 2: Depth of Field: far focus blur scope end")
    }

    void ForceFarFocusBlurEnabled()
    {
        uint8_t* blurGate = Memory::PatternScan(
            baseModule,
            "83 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 45 0F 28 D4",
            "MGS 2: Depth of Field: far focus blur enable gate");

        if (!blurGate)
        {
            return;
        }

        uintptr_t blurEnableAddress = Memory::GetRipRelativeAddress(blurGate, 0x02, 0x07);
        Memory::Write<int>(blurEnableAddress, 1);

        spdlog::info("MGS 2: Depth of Field: far focus blur enabled at {:s}+{:X}.",
                     sExeName.c_str(),
                     blurEnableAddress - reinterpret_cast<uintptr_t>(baseModule));
    }

    void ForceFarFocusMaxPlaneCount()
    {
        uint8_t* maxPlaneClamp = Memory::PatternScan(
            baseModule,
            "39 1D ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 0F 4C 1D ?? ?? ?? ?? 89 9D",
            "MGS 2: Depth of Field: far focus max plane count");

        if (!maxPlaneClamp)
        {
            return;
        }

        uintptr_t maxPlaneCountAddress = Memory::GetRipRelativeAddress(maxPlaneClamp, 0x02, 0x06);
        Memory::Write<int>(maxPlaneCountAddress, 16);
        Memory::Write<float>(maxPlaneCountAddress + 0x04, 4.0f);
        Memory::Write<float>(maxPlaneCountAddress + 0x08, 0.175f);
        Memory::Write<float>(maxPlaneCountAddress + 0x0C, 0.025f);
        Memory::Write<float>(maxPlaneCountAddress + 0x10, 0.0f);

        spdlog::info("MGS 2: Depth of Field: far focus max plane count set to 16 at {:s}+{:X}.",
                     sExeName.c_str(),
                     maxPlaneCountAddress - reinterpret_cast<uintptr_t>(baseModule));
        spdlog::info("MGS 2: Depth of Field: far focus blur weights set to 4.0, 0.175, 0.025, 0.0.");
    }

    void InstallBlurUvScaleHook()
    {
        uint8_t* blurUvRegisterUpload = Memory::PatternScan(
            baseModule,
            "48 8B 0D ?? ?? ?? ?? 4C 8D 4D ?? 8D 57 60 44 8D 47 01 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 4C 8D 4C 24 ?? 8D 57 61 44 8D 47 01 E8",
            "MGS 2: Depth of Field: blur UV register upload");

        if (!blurUvRegisterUpload)
        {
            return;
        }

        uint8_t* setVertexRegistersCall = blurUvRegisterUpload + 0x12;
        if (*setVertexRegistersCall != 0xE8)
        {
            spdlog::error("MGS 2: Depth of Field: expected SetVertexRegisters call was not found; blur UV scale disabled.");
            return;
        }

        const uintptr_t setVertexRegisters = Memory::GetRelativeOffset(setVertexRegistersCall + 1);
        SetVertexRegistersHook = safetyhook::create_inline(reinterpret_cast<void*>(setVertexRegisters), reinterpret_cast<void*>(SetVertexRegisters_Hook));
        LOG_HOOK(SetVertexRegistersHook, "MGS 2: Depth of Field: blur UV register scale")
    }
}

void DepthOfFieldFixes::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Depth of Field: disabled by config.");
        return;
    }

    fBlurUvMultiplier = std::clamp(fBlurUvMultiplier, 0.0f, 20.0f);
    spdlog::info("MGS 2: Depth of Field: blur UV multiplier set to {}.", fBlurUvMultiplier);

    ForceFarFocusBlurEnabled();
    ForceFarFocusMaxPlaneCount();
    InstallFarFocusBlurScopeHooks();
    InstallBlurUvScaleHook();
}
