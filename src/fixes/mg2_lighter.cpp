#include "stdafx.h"
#include "mg2_lighter.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    // MSX capture: 38-40 video frames; ten MC ticks at 15 Hz.
    constexpr int nPoseTicks = 10;
    constexpr int nNativePoseTicks = 21;
    constexpr int nStartTime = nNativePoseTicks - nPoseTicks;
    constexpr int nFootOffsetY = 4;
    constexpr int nRed = 1;
    constexpr int nBlue = 3;
    constexpr int anAnimations[] = { MG2_SNAKE_ANIM_LIGHTER_UP, MG2_SNAKE_ANIM_LIGHTER_RIGHT, MG2_SNAKE_ANIM_LIGHTER_DOWN, MG2_SNAKE_ANIM_LIGHTER_LEFT };
    constexpr int aanOffsets[][2] = { { -8, -48 }, { 0, -43 }, { -10, -34 }, { -16, -43 } };
    constexpr uint16_t anFlameRows[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0080, 0x00C0, 0x01C0, 0x01C0, 0x0080, 0x0080 };
    bool s_bActive = false;
    int s_nFacing = MG2_FACING_UP;
    SafetyHookMid s_hUseWeapon;
    SafetyHookMid s_hSetSpriteTime;
    SafetyHookMid s_hDrawFlame;
    SafetyHookInline s_hDrawSprite;
    struct FlameDraw
    {
        int nX;
        int nY;
        int nClipLeft;
        int nClipTop;
        int nClipRight;
        int nClipBottom;
    };
    FlameDraw* s_pSouthFlame = nullptr;

    bool IsLighterSprite(const Sprite* pSprite)
    {
        if (!s_bActive)
        {
            return false;
        }
        if (GM_Scene != MG2_SCENE_GAMEPLAY)
        {
            s_bActive = false;
            return false;
        }
        auto* pPlayer = MG2_Gamefuncs::pfnGetPlayer();
        return pPlayer && pSprite == MG2_Gamefuncs::pfnGetObjectSprite(pPlayer);
    }

    void UseWeapon_hook(SafetyHookContext& ctx)
    {
        auto* pState = reinterpret_cast<EngineState*>(ctx.rdi);
        s_bActive = false;
        if (pState->GM_SelectedItem == MG2_ITEM_INDEX_SPRAY || pState->GM_Facing < MG2_FACING_UP || pState->GM_Facing > MG2_FACING_LEFT)
        {
            return;
        }
        auto* pPlayer = reinterpret_cast<ObjectPosition*>(ctx.rbp);
        s_bActive = true;
        s_nFacing = pState->GM_Facing;
        pPlayer->nState = MG2_PLAYER_ACTION_WEAPON;
        pState->nWeaponAction = 1;
        pState->nAnimationState = MG2_SNAKE_MODE_LIGHTER;
        MG2_Gamefuncs::pfnRequestSound(MG2_SOUND_LIGHTER);
    }

    void SetSpriteTime_hook(SafetyHookContext& ctx)
    {
        const auto* pSprite = reinterpret_cast<const Sprite*>(ctx.rdi);
        if (!IsLighterSprite(pSprite))
        {
            return;
        }
        if (pSprite->nSheet != MG2_SPRITE_SHEET_SNAKE || pSprite->nAnim != anAnimations[s_nFacing])
        {
            s_bActive = false;
            return;
        }
        if (static_cast<uint32_t>(ctx.r13) == 0)
        {
            ctx.r13 = nStartTime;
        }
    }

    void DrawFlame(const FlameDraw& stFlame)
    {
        auto* pLayer = MG2_Gamefuncs::pfnGetLayerImage(MG2_LAYER_SPRITES);
        const int nOldColour = pLayer->nColour;
        MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_SPRITES, (GM_FrameCounter & 1) ? nRed : nBlue);
        for (int nRow = 0; nRow < static_cast<int>(std::size(anFlameRows)); ++nRow)
        {
            for (int nColumn = 0; nColumn < 16; ++nColumn)
            {
                if ((anFlameRows[nRow] & (0x8000 >> nColumn)) == 0 || stFlame.nX + nColumn < stFlame.nClipLeft || stFlame.nX + nColumn >= stFlame.nClipRight || stFlame.nY + nRow < stFlame.nClipTop || stFlame.nY + nRow >= stFlame.nClipBottom)
                {
                    continue;
                }
                const int anRectangle[] = { stFlame.nX + nColumn, stFlame.nY + nRow, 1, 1 };
                MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_SPRITES, anRectangle);
            }
        }
        MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_SPRITES, nOldColour);
    }

    void __fastcall DrawSprite_hook(Sprite* pSprite, int nMode)
    {
        FlameDraw stFlame{};
        auto* pPreviousFlame = s_pSouthFlame;
        s_pSouthFlame = &stFlame;
        s_hDrawSprite.call<void>(pSprite, nMode);
        s_pSouthFlame = pPreviousFlame;
        if (stFlame.nClipRight > stFlame.nClipLeft && stFlame.nClipBottom > stFlame.nClipTop)
        {
            DrawFlame(stFlame);
        }
    }

    void DrawFlame_hook(SafetyHookContext& ctx)
    {
        const auto* pSprite = reinterpret_cast<const Sprite*>(ctx.rsi);
        if (!IsLighterSprite(pSprite) || pSprite->nSheet != MG2_SPRITE_SHEET_SNAKE || pSprite->nAnim != anAnimations[s_nFacing] || pSprite->nTime <= nStartTime || pSprite->nTime >= nNativePoseTicks || ctx.r13 != reinterpret_cast<uintptr_t>(&pSprite->nFirstTrackFrame))
        {
            return;
        }
        const auto* pRecord = reinterpret_cast<const uint8_t*>(ctx.rbp + ctx.r12);
        const int nGlyphX = static_cast<int16_t>((pRecord[nAnimationRecordXOffset] << 8) | pRecord[nAnimationRecordXOffset + 1]);
        const int nGlyphY = static_cast<int16_t>((pRecord[nAnimationRecordYOffset] << 8) | pRecord[nAnimationRecordYOffset + 1]);
        const int nX = static_cast<int>(ctx.rbx) - nGlyphX + aanOffsets[s_nFacing][0];
        const int nY = static_cast<int>(ctx.r9) - nGlyphY + aanOffsets[s_nFacing][1] + nFootOffsetY;
        const int nClipLeft = static_cast<int>(ctx.r15);
        const int nClipTop = static_cast<int>(ctx.r14);
        const int nClipRight = nClipLeft + *reinterpret_cast<const int*>(ctx.rsp + 0x30);
        const int nClipBottom = nClipTop + *reinterpret_cast<const int*>(ctx.rsp + 0x34);
        const FlameDraw stFlame{ nX, nY, nClipLeft, nClipTop, nClipRight, nClipBottom };
        if (s_nFacing == MG2_FACING_DOWN)
        {
            if (s_pSouthFlame)
            {
                *s_pSouthFlame = stFlame;
            }
        }
        else
        {
            DrawFlame(stFlame);
        }
    }
}

void MG2_Lighter::Initialize()
{
    if (!bEnabled)
    {
        return;
    }
    //UseWeapon+32C
    auto* pUseWeapon = Memory::PatternScan(mg2Module, "83 7F ?? ?? 75 ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 75 ?? 8B 5F", "MG2: Lighter : UseWeapon()+0x32C : use without the spray");
    //SetSpriteTime+2E
    auto* pSetSpriteTime = Memory::PatternScan(mg2Module, "44 8B 47 ?? 4C 8D 4C 24", "MG2: Lighter : SetSpriteTime()+0x2E : lighter pose duration");
    //DrawSprite+32D
    auto* pDrawFlame = Memory::PatternScan(mg2Module, "48 8B 44 24 ?? 48 8B 54 24", "MG2: Lighter : DrawSprite()+0x32D : flame behind Snake");
    if (!pUseWeapon || !pSetSpriteTime || !pDrawFlame || !pScriptGlobals || !MG2_Gamefuncs::pfnGetPlayer || !MG2_Gamefuncs::pfnGetObjectSprite || !MG2_Gamefuncs::pfnGetLayerImage || !MG2_Gamefuncs::pfnSetDrawColor || !MG2_Gamefuncs::pfnLayerFillRect || !MG2_Gamefuncs::pfnRequestSound || !MG2_Gamefuncs::pfnDrawSprite)
    {
        spdlog::error("MG2: Lighter : Missing action or drawing functions. Skipping restoration.");
    }
    else
    {
        s_hDrawSprite = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprite, DrawSprite_hook);
        LOG_HOOK(s_hDrawSprite, "MG2: Lighter : DrawSprite()")
        s_hUseWeapon = safetyhook::create_mid(pUseWeapon, UseWeapon_hook);
        LOG_HOOK(s_hUseWeapon, "MG2: Lighter : UseWeapon()+0x32C")
        s_hSetSpriteTime = safetyhook::create_mid(pSetSpriteTime, SetSpriteTime_hook);
        LOG_HOOK(s_hSetSpriteTime, "MG2: Lighter : SetSpriteTime()+0x2E")
        s_hDrawFlame = safetyhook::create_mid(pDrawFlame, DrawFlame_hook);
        LOG_HOOK(s_hDrawFlame, "MG2: Lighter : DrawSprite()+0x32D")
        if (!s_hUseWeapon || !s_hSetSpriteTime || !s_hDrawFlame || !s_hDrawSprite)
        {
            s_hDrawSprite.reset();
            s_hDrawFlame.reset();
            s_hSetSpriteTime.reset();
            s_hUseWeapon.reset();
            spdlog::error("MG2: Lighter : Incomplete hook setup. Disabled restoration.");
        }
    }
}
