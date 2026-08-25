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
            //_GM_JimakuDaemonStart+10B
            MAKE_HOOK_MID(baseModule, "48 8B CF 48 89 87 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 74 24", "MGS 2: Adjustable Captions: Scale: game\\jimaku.c -> GM_JimakuDaemonStart() -> GetResources() | @l244: ", {
                int* rate = reinterpret_cast<int*>(ctx.rdi + 0x40);
                *rate = static_cast<int>(*rate * percentage);
                          });

            //_GM_JimakuSetZoom
            MAKE_HOOK_MID(baseModule, "8B C1 48 8B 0D ?? ?? ?? ?? 89 41", "MGS 2: Adjustable Captions: Scale: game\\jimaku.c -> GM_JimakuSetZoom() | @l355: ", {
                ctx.rcx = static_cast<uint32_t>(static_cast<int32_t>(ctx.rcx) * percentage);
                          });
        }
        else // eGameType & MGS3
        {
            //GM_JimakuDaemonStart::GetResources+10A
            MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 99 2B C2 D1 F8 8B F8 E8 ?? ?? ?? ?? 03 F8 03 FF E8 ?? ?? ?? ?? 03 C7", "MGS 3: Adjustable Captions: Scale: game\\jimaku.c -> GetResources() | @l244: ", {
                int* rate = reinterpret_cast<int*>(ctx.rbp + 0x58);
                *rate = static_cast<int>(*rate * percentage);
                          });

            //GM_JimakuSetZoom
            MAKE_HOOK_MID(baseModule, "48 8B 05 ?? ?? ?? ?? 8B D1 48 8B C8 48 C1 E1 04 48 85 C0 74 ?? 48 39 41 ?? 75 ?? 89 51", "MGS 3: Adjustable Captions: Scale: game\\jimaku.c -> GM_JimakuSetZoom() | @l355: ", {
                ctx.rcx = static_cast<uint32_t>(static_cast<int32_t>(ctx.rcx) * percentage);
                          });
        }
    }

    // set_pos() only recenters for FONT_SIZE_H, not quad height - resulting it in aligning top on resize. give it a lil manual bump to realign center.
    constexpr float kSubtitleYOffsetPerPercent = 8.0f / 30.0f;

    if (eGameType & MGS2)
    {
        //set_pos+AB
        MAKE_HOOK_MID(baseModule, "C1 E0 ?? 99 81 E2 ?? ?? ?? ?? 03 C2 C1 F8 ?? 2B C8", "MGS 2: Caption Position: game\\jimaku.c -> set_pos() | @l109: ", {
            if (MGS2_RestoreActionLevelSelection::IsActionLevelCaptionActive())
            {
                const int32_t captionY = MGS2_RestoreActionLevelSelection::kActionLevelCaptionY - (Util::IsJapanese() ? MGS2_RestoreActionLevelSelection::kJapaneseActionLevelCaptionYOffset : 0);
                if (static_cast<int32_t>(ctx.rcx) > captionY)
                {
                    ctx.rcx = static_cast<uint32_t>(captionY);
                }
            }

            const int32_t rate = *reinterpret_cast<int32_t*>(ctx.rsi + 0x40);
            const int32_t percentOff = (256 - rate) * 100 / 256;
            ctx.rcx += static_cast<int32_t>(std::lround(percentOff * kSubtitleYOffsetPerPercent));
                      });
    }
    else // eGameType & MGS3
    {
        //set_pos+49
        MAKE_HOOK_MID(baseModule, "99 81 E2 FF 01 00 00 ?? ?? ?? ?? B8 93 24 49 92 F7 E9 41 C1 F8 09 ?? ?? ?? C1 FB 08 8B F3", "MGS 3: Caption Position: game\\jimaku.c -> set_pos() | @l109: ", {
            const int32_t rate = *reinterpret_cast<int32_t*>(ctx.r13 + 0x58);
            const int32_t percentOff = (256 - rate) * 100 / 256;
            ctx.rcx += static_cast<int32_t>(std::lround(percentOff * kSubtitleYOffsetPerPercent));
                      });
    }

}
