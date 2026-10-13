#include "stdafx.h"
#include "mgs2_sprite_position_fixes.hpp"
#include "common.hpp"
#include "logging.hpp"
#include "game_funcs.hpp"
#include "game_stages.hpp"
#include "gamevars.hpp"
#include "mgs2_linkvarbuf.hpp"

namespace
{
    bool gApplyJpStaffroll = false;

    constexpr uint32_t STRCODE_TITLE_MGS2 = GameVars::GV_StrCode("title_mgs2_alp_ovl");

    // It's "sr##e", not "sr##j". That's why we need the position fix for JP.
    constexpr uint32_t STRCODE_SR04E = GameVars::GV_StrCode("sr04e_alp_ovl");
    constexpr uint32_t STRCODE_SR05E = GameVars::GV_StrCode("sr05e_alp_ovl");
    constexpr uint32_t STRCODE_SR06E = GameVars::GV_StrCode("sr06e_alp_ovl");
    constexpr uint32_t STRCODE_SR07E = GameVars::GV_StrCode("sr07e_alp_ovl");
    constexpr uint32_t STRCODE_SR08E = GameVars::GV_StrCode("sr08e_alp_ovl");
    constexpr uint32_t STRCODE_SR09E = GameVars::GV_StrCode("sr09e_alp_ovl");
    constexpr uint32_t STRCODE_SR12E = GameVars::GV_StrCode("sr12e_alp_ovl");
    constexpr uint32_t STRCODE_SR13E = GameVars::GV_StrCode("sr13e_alp_ovl");
    constexpr uint32_t STRCODE_SR14E = GameVars::GV_StrCode("sr14e_alp_ovl");
    constexpr uint32_t STRCODE_SR18E = GameVars::GV_StrCode("sr18e_alp_ovl");

    float JpStaffrollPosXFor(uint32_t tex, float currentX)
    {
        if (strcmp(Shared_Gamefuncs::GM_GetArea(), MGS2Stages::D001P01) != 0)
        {
            return currentX;
        }

        switch (tex)
        {
        case STRCODE_SR04E:
        case STRCODE_SR05E:
        case STRCODE_SR06E:
        case STRCODE_SR18E:
        case STRCODE_SR09E:
        case STRCODE_SR12E:
        case STRCODE_SR13E:
        case STRCODE_SR14E:
            return (currentX == 75.0f) ? 95.0f : currentX;

        case STRCODE_SR07E:
            return (currentX == 40.0f) ? 20.0f : currentX;

        case STRCODE_SR08E:
            return (currentX == -45.0f) ? -35.0f : currentX;

        default:
            return currentX;
        }
    }
}

void MGS2_SpritePositionFixes::ApplyFix()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!Util::IsJapanese())
    {
        spdlog::info("MGS 2: Sprite Position Fixes: JP staff roll: Launched as a non-Japanese version, skipping.");
    }
    else if (!Shared_Gamefuncs::GM_GetArea)
    {
        spdlog::error("MGS 2: Sprite Position Fixes: JP staff roll: Shared_Gamefuncs::GM_GetArea is null, cannot apply fixes.");
    }
    else
    {
        gApplyJpStaffroll = true;
    }

    MAKE_HOOK_MID(baseModule, "66 0F 6E F1 48 8B 4E", "MGS 2: Sprite Position Fixes: mgs2x\\source\\user\\shibata\\effect\\2d_sprt.c -> GetResources() | @l252", {
        const uint32_t tex = static_cast<uint32_t>(ctx.rdi);

        if (tex == STRCODE_TITLE_MGS2 && ctx.rcx == 519 && ctx.rax == 125 && (MGS2_LinkVarBuf::GM_Configuration & MGS2_LinkVarBuf::GM_CONFIG_CUTSCENES_LETTERBOXED))
        {
            //FULLSCREEN SETTINGS
            //ctx.rcx = 519;                                        // w
            //ctx.rax = 125;                                        // h
            //*reinterpret_cast<float*>(ctx.rsp + 0x30) = 0.0f;    // pos.x
            //*reinterpret_cast<float*>(ctx.rsp + 0x34) = 20.0f;   // pos.y

            ctx.rcx = 346; // width
            ctx.rax = 83; // height
            *reinterpret_cast<float*>(ctx.rsp + 0x30) = 83.0f;    // pos.x
            *reinterpret_cast<float*>(ctx.rsp + 0x34) = 40.0f;   // pos.y
        }
        else if (gApplyJpStaffroll)
        {
            float& posX = *reinterpret_cast<float*>(ctx.rsp + 0x30);
            const float newX = JpStaffrollPosXFor(tex, posX);

            if (newX != posX)
            {
                if (g_Logging.bVerboseLogging)
                {
                    spdlog::info("MGS 2: Sprite Position Fixes: JP staff roll: tex={:#010x} pos.x {} -> {}", static_cast<uint32_t>(ctx.rdi), posX, newX);
                }

                posX = newX;
            }
        }
        });
}