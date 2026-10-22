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
        const auto* pSprite = reinterpret_cast<const Sprite*>(ctx.rdi);
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
        if (pSprite->nSheet == MG2_SPRITE_SHEET_SNAKE && static_cast<uint32_t>(ctx.r13) == 0)
        {
            ctx.r13 = nInitialSpriteTime;
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
        // puff puff pass
        ctx.rax = pSprite->nTime == 0 && pSprite->nLoopsDone > 0 ? nMCSmokeTime : 0;
    }
}

void MG2_CigaretteIdle::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    //SetSpriteTime+44
    auto* pSetSpriteTime = Memory::PatternScan(mg2Module, "8B 5C 24 ?? 48 8B CD 8D 53", "MG2: Cigarette Idle : SetSpriteTime()+0x44 : restore animation timing");
    //sub_180019E20+4E5
    auto* pSmoke = Memory::PatternScan(mg2Module, "83 F8 ?? 0F 85 ?? ?? ?? ?? 48 8D 15", "MG2: Cigarette Idle : sub_180019E20()+0x4E5 : emit smoke after the raised pose");
    if (!pSetSpriteTime || !pSmoke)
    {
        spdlog::error("MG2: Cigarette Idle : Missing animation hooks. Skipping restoration.");
    }
    else
    {
        s_hSetSpriteTime = safetyhook::create_mid(pSetSpriteTime, SetSpriteTime_hook);
        LOG_HOOK(s_hSetSpriteTime, "MG2: Cigarette Idle : SetSpriteTime()+0x44")
        s_hSmoke = safetyhook::create_mid(pSmoke, Smoke_hook);
        LOG_HOOK(s_hSmoke, "MG2: Cigarette Idle : sub_180019E20()+0x4E5")
        if (!s_hSetSpriteTime || !s_hSmoke)
        {
            s_hSmoke.reset();
            s_hSetSpriteTime.reset();
            spdlog::error("MG2: Cigarette Idle : Incomplete hook setup. Disabled restoration.");
        }
    }
}
