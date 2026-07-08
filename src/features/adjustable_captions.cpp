#include "stdafx.h"
#include "adjustable_captions.hpp"
#include "common.hpp"
#include "logging.hpp"
#include "mgs2_restore_action_level_selection.hpp"


void AdjustableCaptions::Apply()
{
    if (!(eGameType & (MGS2 | MGS3)))
    {
        return;
    }

    if (iSubtitleAlpha != 100)
    {
        static const float percentage = (static_cast<float>(iSubtitleAlpha) * 0.01f);
        MAKE_HOOK_MID(baseModule, "44 8B CA 48 8D 4C 24 ?? 44 8B C2 E8 ?? ?? ?? ?? 48 8B D6", "Adjustable Captions: Caption Opacity", {
            *reinterpret_cast<int*>(ctx.rsp + 0x20) *= percentage;
                      });
    }

    if (iOutlineOpacity != 100)
    {
        static const float percentage = (static_cast<float>(iOutlineOpacity) * 0.01f);
        if (eGameType & MGS2)
        {
            MAKE_HOOK_MID(baseModule, "41 0F 45 FD", "Adjustable Captions: Caption Background Opacity", {
                uint8_t a = static_cast<uint8_t>(0x80 * (percentage / 2));
                ctx.rdi = (ctx.rdi & 0x00FFFFFF) | (static_cast<uint32_t>(a) << 24);
                //ctx.rdi = (ctx.rdi & 0x00FFFFFF);
                          });
        }
        else // eGameType & MGS3
        {
            MAKE_HOOK_MID(baseModule, "44 0F 45 FB", "Adjustable Captions: Caption Background Opacity", {
                uint8_t a = static_cast<uint8_t>(0x80 * (percentage / 2));
                ctx.r15 = (ctx.r15 & 0x00FFFFFF) | (static_cast<uint32_t>(a) << 24);
                          });
        }
    }


    if (fSubtitleScale != 100)
    {
        static const float percentage = (static_cast<float>(fSubtitleScale) * 0.01f);
        if (eGameType & MGS2)
        {
            MAKE_HOOK_MID(baseModule, "48 8B C4 48 89 58 ?? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? F3 0F 10 05 ?? ?? ?? ?? 48 8B F1", "Adjustable Captions: Scale", {
                *reinterpret_cast<int*>(ctx.rcx + 0x40) = static_cast<int>(256 * percentage);
                          });
        }
        else // eGameType & MGS3
        {
            MAKE_HOOK_MID(baseModule, "40 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 8B 41", "Adjustable Captions: Scale", {
                *reinterpret_cast<int*>(ctx.rcx + 0x58) = static_cast<int>(256 * percentage);
                          });
        }
    }

    {
        constexpr float kSubtitleYOffsetPerPercent = 8.0f / 30.0f;
        static const int32_t subtitleYOffset = static_cast<int32_t>(std::lround((100 - fSubtitleScale) * kSubtitleYOffsetPerPercent));
        if (eGameType & MGS2)
        {
            MAKE_HOOK_MID(baseModule, "C1 E0 ?? 99 81 E2 ?? ?? ?? ?? 03 C2 C1 F8 ?? 2B C8", "MGS2: game\\jimaku.c -> set_pos()", {
                if (MGS2_RestoreActionLevelSelection::IsActionLevelCaptionActive())
                {
                    const int32_t captionY = MGS2_RestoreActionLevelSelection::kActionLevelCaptionY - (Util::IsJapanese() ? MGS2_RestoreActionLevelSelection::kJapaneseActionLevelCaptionYOffset : 0);
                    if (static_cast<int32_t>(ctx.rcx) > captionY)
                    {
                        ctx.rcx = static_cast<uint32_t>(captionY);
                    }
                    //spdlog::info("MGS2: game\\jimaku.c -> set_pos() - Adjusted caption Y position to {}", ctx.rcx);
                    ctx.rcx += subtitleYOffset;
                }
                          });
        }
        else //eGameType & MGS3
        {
            MAKE_HOOK_MID(baseModule, "99 81 E2 ?? ?? ?? ?? ?? ?? ?? ?? B8 ?? ?? ?? ?? F7 E9 41 C1 F8 ?? ?? ?? ?? C1 FB ?? 8B F3", "MGS3: game\\jimaku.c -> set_pos()", {
                ctx.rcx += subtitleYOffset;
                          });
        }
    }



}
