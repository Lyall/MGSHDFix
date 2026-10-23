#include "stdafx.h"
#include "mg2_cigarette_idle.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"
#include "mg2_cigarette_animations.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    constexpr int nInitialSpriteTime = 64;
    constexpr int nMCSmokeTime = 124;
    constexpr size_t nAnimationOffsetStack = 0x74;
    constexpr size_t nContainerHeaderSize = 4;
    bool s_abRestored[std::size(MG2_CigaretteAnimations::astPatches)] = {};
    bool s_abWarned[std::size(MG2_CigaretteAnimations::astPatches)] = {};
    SafetyHookMid s_hSetSpriteTime;
    SafetyHookMid s_hSmoke;
    SafetyHookMid s_hSmokeEnd;
    SafetyHookMid s_hSmokeAdvance;
    SafetyHookInline s_hScriptRun;
    bool s_bInitialized = false;
    bool s_bWindInitialized = false;
    bool s_bWindWarned = false;
    bool s_bSmokeSuppressed = false;
    enum class WindMode { Reversing, Fixed, Random };
    constexpr int nWindPeriod = 240;
    constexpr int nRoofWindDelay = 255;
    WindMode s_eWindMode = WindMode::Random;
    int s_nWindPhase = 0;
    int s_nWindTimer = nWindPeriod;
    constexpr int nWindBlockOffset = 0x5FF;
    constexpr int nWindYieldOffset = 0x689;
    constexpr uint8_t anSkipWind[] = { 0x0A, 0x00, 0x01, 0x00, 0x01, 0x01, 0x02, 0x00, 0x7F };

    bool RestoreWindScript()
    {
        const auto hScript = pLoadedScripts[MG2_SCRIPT_GAMEPLAY];
        const auto* pHandle = reinterpret_cast<const uintptr_t*>(hScript);
        if (!Memory::IsReadable(pHandle, sizeof(uintptr_t)))
        {
            return false;
        }
        auto* pCode = reinterpret_cast<uint8_t*>(*pHandle);
        if (!Memory::IsReadable(pCode, nWindYieldOffset + 3))
        {
            return false;
        }
        auto* pBlock = pCode + nWindBlockOffset;
        if (std::memcmp(pBlock, MG2_CigaretteAnimations::anWindOriginal, sizeof(MG2_CigaretteAnimations::anWindOriginal)) == 0)
        {
            std::memcpy(pBlock, anSkipWind, sizeof(anSkipWind));
        }
        const bool bRestored = std::memcmp(pBlock, anSkipWind, sizeof(anSkipWind)) == 0 && std::memcmp(pBlock + sizeof(anSkipWind), MG2_CigaretteAnimations::anWindOriginal + sizeof(anSkipWind), sizeof(MG2_CigaretteAnimations::anWindOriginal) - sizeof(anSkipWind)) == 0;
        if (!bRestored && !s_bWindWarned)
        {
            spdlog::warn("MG2: Wind : Unexpected wind script. Keeping MC wind behavior.");
            s_bWindWarned = true;
        }
        return bRestored;
    }

    void TickWind()
    {
        if (GM_Scene != MG2_SCENE_GAMEPLAY || GM_InputMode != 0 || GM_GameplayHalt != 0 || GM_InsideElevator != 0 || s_bSmokeSuppressed)
        {
            return;
        }
        const auto* pState = MG2_Gamefuncs::pfnGetEngineState();
        if (pState->GM_SelectedItem == MG2_ITEM_INDEX_BINOCULARS)
        {
            return;
        }
        if (s_eWindMode == WindMode::Fixed)
        {
            GM_Wind = 2;
        }
        else if (s_eWindMode == WindMode::Reversing)
        {
            if (s_nWindPhase == 0)
            {
                GM_Wind = 2;
                s_nWindTimer = nRoofWindDelay;
                s_nWindPhase = 1;
            }
            else if (s_nWindPhase == 1 && --s_nWindTimer == 0)
            {
                GM_Wind = 0;
                s_nWindPhase = 2;
            }
        }
        else if (--s_nWindTimer == 0)
        {
            s_nWindTimer = nWindPeriod;
            GM_Wind = static_cast<int>(MG2_Gamefuncs::pfnRandom(4));
        }
    }

    void __fastcall ScriptRun_hook(int* pScript)
    {
        const bool bWind = IsScript(pScript, MG2_SCRIPT_GAMEPLAY) && RestoreWindScript();
        if (bWind && pScript[MG2_SCRIPT_PROG_COUNT] == 0)
        {
            s_nWindPhase = 0;
        }
        s_hScriptRun.call<void>(pScript);
        if (bWind && pScript[MG2_SCRIPT_INSTRUCTION_PROG_COUNT] == nWindYieldOffset)
        {
            TickWind();
        }
    }

    size_t FindPatch(const Sprite* pSprite)
    {
        const auto& astPatches = MG2_CigaretteAnimations::astPatches;
        for (size_t nIndex = 0; nIndex < std::size(astPatches); ++nIndex)
        {
            if (astPatches[nIndex].nSheet == pSprite->nSheet && astPatches[nIndex].nAnimation == pSprite->nAnim)
            {
                return nIndex;
            }
        }
        return std::size(astPatches);
    }

    void SetSpriteTime_hook(SafetyHookContext& ctx)
    {
        auto* pSprite = reinterpret_cast<Sprite*>(ctx.rdi);
        if (pSprite->nSheet != MG2_SPRITE_SHEET_SNAKE && pSprite->nSheet != MG2_SPRITE_SHEET_CIGARETTE_SMOKE)
        {
            return;
        }
        const size_t nIndex = FindPatch(pSprite);
        if (nIndex == std::size(MG2_CigaretteAnimations::astPatches))
        {
            return;
        }
        const auto& stPatch = MG2_CigaretteAnimations::astPatches[nIndex];
        const auto nOffset = *reinterpret_cast<const uint32_t*>(ctx.rsp + nAnimationOffsetStack);
        if (nOffset < nContainerHeaderSize)
        {
            return;
        }
        auto* pData = reinterpret_cast<uint8_t*>(ctx.rbp) + nOffset - nContainerHeaderSize;
        s_abRestored[nIndex] = false;
        if (std::memcmp(pData, stPatch.pOriginal, nContainerHeaderSize) == 0)
        {
            if (std::memcmp(pData, stPatch.pOriginal, stPatch.nSize) == 0)
            {
                std::memcpy(pData, stPatch.pRestored, stPatch.nSize);
            }
            s_abRestored[nIndex] = std::memcmp(pData, stPatch.pRestored, stPatch.nSize) == 0;
        }
        if (!s_abRestored[nIndex])
        {
            if (!s_abWarned[nIndex])
            {
                spdlog::warn("MG2: Cigarette Idle : Unexpected sheet {} animation {}. Skipping restoration.", stPatch.nSheet, stPatch.nAnimation);
                s_abWarned[nIndex] = true;
            }
            return;
        }
        if (pSprite->nSheet == MG2_SPRITE_SHEET_CIGARETTE_SMOKE && (s_bSmokeSuppressed || HasPlayerWeaponEffects()))
        {
            pSprite->nShown = 0;
        }
        if (pSprite->nSheet == MG2_SPRITE_SHEET_SNAKE && static_cast<uint32_t>(ctx.r13) == 0)
        {
            ctx.r13 = nInitialSpriteTime;
        }
    }

    void SmokeAdvance_hook(SafetyHookContext& ctx)
    {
        const auto* pSprite = reinterpret_cast<const Sprite*>(ctx.rsi);
        if (pSprite->nSheet != MG2_SPRITE_SHEET_CIGARETTE_SMOKE || pSprite->nShown != 0)
        {
            return;
        }
        const size_t nIndex = FindPatch(pSprite);
        if (nIndex < std::size(s_abRestored) && s_abRestored[nIndex])
        {
            constexpr uint32_t nAnimationStoppedFlag = 0x400;
            ctx.rdx |= nAnimationStoppedFlag;
        }
    }

    void SmokeEnd_hook(SafetyHookContext& ctx)
    {
        if (s_bSmokeSuppressed || HasPlayerWeaponEffects())
        {
            ctx.rax = 1;
        }
    }

    void Smoke_hook(SafetyHookContext& ctx)
    {
        const auto* pSprite = reinterpret_cast<const Sprite*>(ctx.r14);
        const size_t nIndex = FindPatch(pSprite);
        if (nIndex == std::size(MG2_CigaretteAnimations::astPatches) || !s_abRestored[nIndex])
        {
            return;
        }
        if (HasPlayerWeaponEffects())
        {
            auto* pState = reinterpret_cast<EngineState*>(ctx.rsi);
            pState->nAnimationState = pState->GM_SelectedWeapon < 0 ? 0 : 3;
            ctx.rax = 0;
            return;
        }
        ctx.rax = !s_bSmokeSuppressed && pSprite->nTime == 0 && pSprite->nLoopsDone > 0 ? nMCSmokeTime : 0;
    }
}

void MG2_CigaretteIdle::HandleLevelTransition()
{
    if (!s_bInitialized && !s_bWindInitialized)
    {
        return;
    }
    s_bSmokeSuppressed = IsArea(18) && (GM_Room < 12 || IsAnyRoom({ { 18, 20 }, { 18, 21 } }));
    if (!s_bWindInitialized)
    {
        return;
    }
    s_eWindMode = IsRoom(13, 4) ? WindMode::Reversing : IsAnyRoom({ { 1, 8 }, { 14, 4 }, { 14, 5 }, { 19, 5 }, { 19, 6 }, { 19, 7 }, { 19, 8 }, { 19, 9 }, { 19, 10 }, { 19, 11 } }) ? WindMode::Fixed : WindMode::Random;
    s_nWindTimer = nWindPeriod;
    if (GM_InsideElevator != 0)
    {
        GM_Wind = 2;
    }
    else if (s_eWindMode == WindMode::Random)
    {
        GM_Wind = 0;
    }
}

void MG2_CigaretteIdle::Initialize()
{
    if (bRestoreWind)
    {
        if (!pScriptGlobals || !pLoadedScripts || !pRunningScriptCode || !MG2_Gamefuncs::pfnScriptRun || !MG2_Gamefuncs::pfnRandom || !MG2_Gamefuncs::pfnGetEngineState)
        {
            spdlog::error("MG2: Wind : Missing wind bindings. Skipping restoration.");
        }
        else
        {
            s_hScriptRun = safetyhook::create_inline(MG2_Gamefuncs::pfnScriptRun, ScriptRun_hook);
            LOG_HOOK(s_hScriptRun, "MG2: Wind : ScriptRun()")
            if (s_hScriptRun)
            {
                s_bWindInitialized = true;
                HandleLevelTransition();
                spdlog::info("MG2: Wind : Initialized restoration.");
            }
        }
    }
    if (!bEnabled)
    {
        return;
    }

    //SetSpriteTime+44
    auto* pSetSpriteTime = Memory::PatternScan(mg2Module, "8B 5C 24 ?? 48 8B CD 8D 53", "MG2: Cigarette Idle : SetSpriteTime()+0x44 : restore animation timing");
    //sub_180019E20+4E5
    auto* pSmoke = Memory::PatternScan(mg2Module, "83 F8 ?? 0F 85 ?? ?? ?? ?? 48 8D 15", "MG2: Cigarette Idle : sub_180019E20()+0x4E5 : emit smoke after the raised pose");
    //sub_18000C0A0+13A
    auto* pSmokeEnd = Memory::PatternScan(mg2Module, "85 C0 74 ?? 48 8B CF E8 ?? ?? ?? ?? 8B 4D", "MG2: Cigarette Idle : sub_18000C0A0()+0x13A : cancel smoke while weapons are active");
    //DrawSprite+479
    auto* pSmokeAdvance = Memory::PatternScan(mg2Module, "0F BA E2 ?? 72 ?? 83 3D ?? ?? ?? ?? 00", "MG2: Cigarette Idle : DrawSprite()+0x479 : hold smoke until visible");
    if (!pSetSpriteTime || !pSmoke || !pSmokeEnd || !pSmokeAdvance || !MG2_Gamefuncs::pfnGetEngineState || !pWeaponEffectCounts || !pScriptGlobals)
    {
        spdlog::error("MG2: Cigarette Idle : Missing animation hooks. Skipping restoration.");
    }
    else
    {
        s_hSetSpriteTime = safetyhook::create_mid(pSetSpriteTime, SetSpriteTime_hook);
        LOG_HOOK(s_hSetSpriteTime, "MG2: Cigarette Idle : SetSpriteTime()+0x44")
        s_hSmoke = safetyhook::create_mid(pSmoke, Smoke_hook);
        LOG_HOOK(s_hSmoke, "MG2: Cigarette Idle : sub_180019E20()+0x4E5")
        s_hSmokeEnd = safetyhook::create_mid(pSmokeEnd, SmokeEnd_hook);
        LOG_HOOK(s_hSmokeEnd, "MG2: Cigarette Idle : sub_18000C0A0()+0x13A")
        s_hSmokeAdvance = safetyhook::create_mid(pSmokeAdvance, SmokeAdvance_hook);
        LOG_HOOK(s_hSmokeAdvance, "MG2: Cigarette Idle : DrawSprite()+0x479")
        if (!s_hSetSpriteTime || !s_hSmoke || !s_hSmokeEnd || !s_hSmokeAdvance)
        {
            s_hSmokeAdvance.reset();
            s_hSmokeEnd.reset();
            s_hSmoke.reset();
            s_hSetSpriteTime.reset();
            spdlog::error("MG2: Cigarette Idle : Incomplete hook setup. Disabled restoration.");
        }
        else
        {
            s_bInitialized = true;
            HandleLevelTransition();
            spdlog::info("MG2: Cigarette Idle : Initialized animation and smoke restoration.");
        }
    }
}
