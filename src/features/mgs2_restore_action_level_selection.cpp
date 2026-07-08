#include "stdafx.h"

#include "mgs2_restore_action_level_selection.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"

namespace
{
    constexpr uint16_t kTitleMenuTankerCleared = 0x0004;
    constexpr uint16_t kTitleMenuPlantCleared = 0x0008;

    constexpr uint8_t kDispFirstTimeOnly = 0x01;
    constexpr uint8_t kDispFirstTimeTanker = 0x03;
    constexpr uint8_t kDispFirstTimePlant = 0x05;
    constexpr uint8_t kDispAllStories = 0x0F;

    constexpr ptrdiff_t kWorkBusyFlag = 0x60;
    constexpr ptrdiff_t kWorkStep = 0x74;
    constexpr ptrdiff_t kWorkSubStep = 0x78;
    constexpr ptrdiff_t kWorkGameCursor = 0x90;
    constexpr ptrdiff_t kWorkLevelCursor = 0x92;
    constexpr ptrdiff_t kWorkQuestionCursor = 0x96;
    constexpr ptrdiff_t kWorkDispGameItem = 0x9C;
    constexpr ptrdiff_t kWorkCursor = 0xA8;
    constexpr ptrdiff_t kWorkGame0 = 0xB8;
    constexpr ptrdiff_t kWorkGame1 = 0xC0;
    constexpr ptrdiff_t kWorkGame2 = 0xC8;
    constexpr ptrdiff_t kWorkGame3 = 0xD0;
    constexpr ptrdiff_t kWorkTargetY = 0x154;
    constexpr ptrdiff_t kSpriteParent = 0x00;
    constexpr ptrdiff_t kSpriteFlags = 0x30;
    constexpr ptrdiff_t kEmptyPosY = 0x64;
    constexpr ptrdiff_t kSpritePosY = 0x9C;
    constexpr uint32_t kSpriteHiddenFlag = 0x8000u;
    constexpr ptrdiff_t kQuestionCodeBase = 0x70;
    constexpr ptrdiff_t kQuestionCodeStride = 0x10;
    constexpr ptrdiff_t kQuestionCodeFont = 0x08;
    constexpr ptrdiff_t kQuestionVoiceStartData = 0xC0;
    constexpr ptrdiff_t kQuestionVoiceEndData = 0xD8;
    constexpr int32_t kQuestionIntroVoiceDefaultFlags = 0x21000000;
    constexpr int32_t kStreamPositionMask = 0x00FFFFFF;
    constexpr int32_t kStreamFlagMask = 0xFF000000;
    constexpr int32_t kStreamSectorSize = 2048;
    constexpr char kQuestionIntroVoiceName[] = "vc126101.sdt";
    constexpr char kQuestionIntroVoiceStreamLayer[] = "vox2";

    constexpr int32_t kJapaneseQuestionTextYOffset = 4;
    constexpr int32_t kJapaneseQuestionTextWidthPad = 8;
    constexpr int32_t kJapaneseQuestionTextScreenHeightPad = 2;
    constexpr size_t kQuestionTextCount = 5;

    struct GclStringResource
    {
        uintptr_t blockTop;
        uintptr_t resourceTable;
        uintptr_t stringTable;
        uintptr_t fontData;
        uintptr_t bpStringData;
    };

    GclStringResource** g_GlobalResInfo = nullptr;
    int32_t g_QuestionIntroVoiceLowBits = 0;
    int32_t g_QuestionIntroVoicePos = kQuestionIntroVoiceDefaultFlags;
    bool g_QuestionIntroVoiceResolved = false;
    bool g_QuestionIntroVoiceAvailable = false;
    int32_t* GM_StreamCaptionCurrentName = nullptr;

    int32_t GclGetLong(uintptr_t address)
    {
        if (!Memory::IsReadable(reinterpret_cast<const void*>(address), sizeof(int32_t)))
        {
            return 0;
        }

        const auto* value = reinterpret_cast<const uint8_t*>(address);
        return static_cast<int32_t>(
            (value[3] << 24) |
            (value[2] << 16) |
            (value[1] << 8) |
            value[0]);
    }

    bool IsWorkReadable(uintptr_t work)
    {
        return Memory::IsReadable(reinterpret_cast<const void*>(work), kWorkDispGameItem + sizeof(uint8_t));
    }

    uint8_t GetStoryDisplayMask()
    {
        return (MGS2_LinkVarBuf::GM_TitleMenuStatus & (kTitleMenuTankerCleared | kTitleMenuPlantCleared)) ? kDispAllStories : kDispFirstTimeOnly;
    }

    bool IsFirstTimeDisplay(uint8_t displayMask)
    {
        return displayMask == kDispFirstTimeOnly ||
            displayMask == kDispFirstTimeTanker ||
            displayMask == kDispFirstTimePlant ||
            displayMask == kDispAllStories;
    }

    bool IsFirstTimeDisplay(uintptr_t work)
    {
        if (!IsWorkReadable(work))
        {
            return false;
        }

        return IsFirstTimeDisplay(Memory::ReadField<uint8_t>(work, kWorkDispGameItem));
    }

    bool IsFirstTimeQuestionPath(uintptr_t work)
    {
        if (!IsWorkReadable(work))
        {
            return false;
        }

        return Memory::ReadField<int16_t>(work, kWorkGameCursor, -1) == 0 && IsFirstTimeDisplay(work);
    }

    void UpdateStoryChoices(uintptr_t work)
    {
        auto* displayMask = reinterpret_cast<uint8_t*>(work + kWorkDispGameItem);
        if (Memory::IsWritable(displayMask, sizeof(*displayMask)))
        {
            *displayMask = GetStoryDisplayMask();
        }
    }

    void ShowSprite(uintptr_t sprite)
    {
        if (!sprite)
        {
            return;
        }

        auto* flags = reinterpret_cast<uint32_t*>(sprite + kSpriteFlags);
        if (Memory::IsWritable(flags, sizeof(*flags)))
        {
            *flags &= ~kSpriteHiddenFlag;
        }
    }

    void SetSpriteY(uintptr_t sprite, float y)
    {
        auto* posY = reinterpret_cast<float*>(sprite + kSpritePosY);
        if (Memory::IsWritable(posY, sizeof(*posY)))
        {
            *posY = y;
        }
    }

    void SetEmptyY(uintptr_t empty, float y)
    {
        auto* posY = reinterpret_cast<float*>(empty + kEmptyPosY);
        if (Memory::IsWritable(posY, sizeof(*posY)))
        {
            *posY = y;
        }
    }

    void SetWorkTargetY(uintptr_t work, float y)
    {
        auto* targetY = reinterpret_cast<float*>(work + kWorkTargetY);
        if (Memory::IsWritable(targetY, sizeof(*targetY)))
        {
            *targetY = y;
        }
    }

    float ReadSpriteY(uintptr_t sprite)
    {
        return Memory::ReadField<float>(sprite, kSpritePosY);
    }

    bool IsValidRowStep(float rowStep)
    {
        return rowStep > 1.0f && rowStep < 200.0f && std::isfinite(rowStep);
    }

    float GetFourthRowY(float row0Y, float row1Y, float row2Y)
    {
        float rowStep = row2Y - row1Y;
        if (!IsValidRowStep(rowStep))
        {
            rowStep = row1Y - row0Y;
        }

        return IsValidRowStep(rowStep) ? row2Y + rowStep : 0.0f;
    }

    float ReadParentWorldY(uintptr_t sprite)
    {
        float y = 0.0f;
        uintptr_t parent = Memory::ReadField<uintptr_t>(sprite, kSpriteParent);
        for (int i = 0; i < 8 && parent; i++)
        {
            y += Memory::ReadField<float>(parent, kEmptyPosY);
            parent = Memory::ReadField<uintptr_t>(parent, kSpriteParent);
        }
        return y;
    }

    float ReadWorldY(uintptr_t sprite)
    {
        return ReadSpriteY(sprite) + ReadParentWorldY(sprite);
    }

    void MoveTankerPlantToFourthRow(uintptr_t work)
    {
        const uintptr_t game0 = Memory::ReadField<uintptr_t>(work, kWorkGame0);
        const uintptr_t game1 = Memory::ReadField<uintptr_t>(work, kWorkGame1);
        const uintptr_t game2 = Memory::ReadField<uintptr_t>(work, kWorkGame2);
        const uintptr_t game3 = Memory::ReadField<uintptr_t>(work, kWorkGame3);
        if (!game0 || !game1 || !game2 || !game3)
        {
            return;
        }

        // menuT_P is under a shifted parent during the restored first-time menu.
        const float targetWorldY = GetFourthRowY(ReadWorldY(game0), ReadWorldY(game1), ReadWorldY(game2));
        if (targetWorldY > 0.0f)
        {
            SetSpriteY(game3, targetWorldY - ReadParentWorldY(game3));
        }
    }

    float GetFourthGameRowY(uintptr_t work)
    {
        const uintptr_t game0 = Memory::ReadField<uintptr_t>(work, kWorkGame0);
        const uintptr_t game1 = Memory::ReadField<uintptr_t>(work, kWorkGame1);
        const uintptr_t game2 = Memory::ReadField<uintptr_t>(work, kWorkGame2);
        if (!game0 || !game1 || !game2)
        {
            return 0.0f;
        }

        return GetFourthRowY(ReadSpriteY(game0), ReadSpriteY(game1), ReadSpriteY(game2));
    }

    void AdjustTankerPlantCursorY(uintptr_t work, bool snapCursor)
    {
        if (!IsWorkReadable(work) ||
            Memory::ReadField<uint8_t>(work, kWorkDispGameItem) != kDispAllStories ||
            Memory::ReadField<int16_t>(work, kWorkGameCursor, -1) != 3)
        {
            return;
        }

        const float targetY = GetFourthGameRowY(work);
        if (targetY <= 0.0f || !std::isfinite(targetY))
        {
            return;
        }

        SetWorkTargetY(work, targetY);
        if (snapCursor)
        {
            SetEmptyY(Memory::ReadField<uintptr_t>(work, kWorkCursor), targetY);
        }
    }

    bool CanForceGameMenuVisibility(uintptr_t work)
    {
        return Memory::ReadField<int32_t>(work, kWorkSubStep, -1) != 1 ||
            Memory::ReadField<int32_t>(work, kWorkBusyFlag, -1) == 0;
    }

    void ShowFirstTimeEntry(uintptr_t work)
    {
        if (!IsWorkReadable(work))
        {
            return;
        }

        const uint8_t displayMask = Memory::ReadField<uint8_t>(work, kWorkDispGameItem);
        if (!IsFirstTimeDisplay(displayMask))
        {
            return;
        }

        const bool canForceVisible = CanForceGameMenuVisibility(work);
        const uintptr_t game0 = Memory::ReadField<uintptr_t>(work, kWorkGame0);
        if (canForceVisible)
        {
            ShowSprite(game0);
        }

        if (displayMask == kDispAllStories)
        {
            MoveTankerPlantToFourthRow(work);
            AdjustTankerPlantCursorY(work, false);

            if (canForceVisible)
            {
                ShowSprite(Memory::ReadField<uintptr_t>(work, kWorkGame1));
                ShowSprite(Memory::ReadField<uintptr_t>(work, kWorkGame2));
                ShowSprite(Memory::ReadField<uintptr_t>(work, kWorkGame3));
            }
        }
    }

    uintptr_t LookupBpQuestionString(uintptr_t kpString)
    {
        if (!kpString ||
            !g_GlobalResInfo ||
            !Memory::IsReadable(g_GlobalResInfo, sizeof(*g_GlobalResInfo)) ||
            !*g_GlobalResInfo ||
            !Memory::IsReadable(*g_GlobalResInfo, sizeof(**g_GlobalResInfo)))
        {
            return 0;
        }

        const GclStringResource* resource = *g_GlobalResInfo;
        if (!resource->resourceTable || !resource->stringTable || !resource->bpStringData)
        {
            return 0;
        }

        const auto resourceTable = reinterpret_cast<const int32_t*>(resource->resourceTable);
        const auto bpStringData = resource->bpStringData;
        if (resource->stringTable <= resource->resourceTable)
        {
            return 0;
        }

        const ptrdiff_t stringCount = static_cast<ptrdiff_t>(
            (resource->stringTable - resource->resourceTable) / sizeof(int32_t));
        if (stringCount <= 0 || stringCount > 0x10000 ||
            !Memory::IsReadable(resourceTable, static_cast<size_t>(stringCount) * sizeof(int32_t)) ||
            !Memory::IsReadable(reinterpret_cast<const void*>(bpStringData), sizeof(int32_t)))
        {
            return 0;
        }

        for (ptrdiff_t i = 0; i < stringCount; i++)
        {
            const int32_t stringOffset = GclGetLong(reinterpret_cast<uintptr_t>(resourceTable + i));
            if ((stringOffset & 0x80000000) == 0)
            {
                continue;
            }

            const uintptr_t originalString = resource->stringTable + (stringOffset & 0x7FFFFFFF);
            if (originalString != kpString)
            {
                continue;
            }

            const uintptr_t bpOffsetAddress = bpStringData + sizeof(int32_t) + (i * sizeof(int32_t));
            const int32_t bpOffset = GclGetLong(bpOffsetAddress);
            const uintptr_t bpString = bpStringData + bpOffset;
            return Memory::IsReadable(reinterpret_cast<const void*>(bpString), 1) ? bpString : 0;
        }

        return 0;
    }

    void ConvertQuestionTextToBpFont(uintptr_t work)
    {
        for (size_t i = 0; i < kQuestionTextCount; i++)
        {
            auto* font = reinterpret_cast<uintptr_t*>(work + kQuestionCodeBase + (i * kQuestionCodeStride) + kQuestionCodeFont);
            if (Memory::IsWritable(font, sizeof(*font)))
            {
                const uintptr_t bpString = LookupBpQuestionString(*font);
                if (bpString)
                {
                    *font = bpString;
                }
            }
        }
    }

    void SetQuestionIntroVoice(uintptr_t work)
    {
        if (!g_QuestionIntroVoiceResolved)
        {
            const bool isJapanese = Util::IsJapanese();
            const auto streamList = sExePath / (isJapanese ? "jp" : "eu") / kQuestionIntroVoiceStreamLayer / "_bp" / "bp_streams.txt";
            std::ifstream file(streamList);

            uint64_t offset = 0;
            std::string streamName;
            while (file >> offset >> streamName)
            {
                if (streamName.find(kQuestionIntroVoiceName) == std::string::npos)
                {
                    continue;
                }

                g_QuestionIntroVoiceLowBits = static_cast<int32_t>((offset / kStreamSectorSize) & kStreamPositionMask);
                g_QuestionIntroVoiceAvailable = true;
                //spdlog::info("MGS2: Restore Action Level Selection: {} -> 0x{:06X}", kQuestionIntroVoiceName, static_cast<uint32_t>(g_QuestionIntroVoiceLowBits));
                break;
            }

            if (!g_QuestionIntroVoiceAvailable)
            {
                spdlog::warn("MGS2: Restore Action Level Selection: {} not found in {}", kQuestionIntroVoiceName, streamList.string());
            }

            g_QuestionIntroVoiceResolved = true;
        }

        if (!g_QuestionIntroVoiceAvailable)
        {
            return;
        }

        const uintptr_t endVoiceData = Memory::ReadField<uintptr_t>(work, kQuestionVoiceEndData);
        const int32_t voiceFlags = GclGetLong(endVoiceData) & kStreamFlagMask;
        g_QuestionIntroVoicePos = (voiceFlags != 0 ? voiceFlags : kQuestionIntroVoiceDefaultFlags) | g_QuestionIntroVoiceLowBits;

        auto* voiceStartData = reinterpret_cast<uintptr_t*>(work + kQuestionVoiceStartData);
        if (Memory::IsWritable(voiceStartData, sizeof(*voiceStartData)))
        {
            *voiceStartData = reinterpret_cast<uintptr_t>(&g_QuestionIntroVoicePos);
        }
    }



    uint32_t ShiftQuestionTextY(uint64_t y)
    {
        return static_cast<uint32_t>(static_cast<int32_t>(y) - kJapaneseQuestionTextYOffset);
    }

}

bool MGS2_RestoreActionLevelSelection::IsActionLevelCaptionActive()
{
    return GM_StreamCaptionCurrentName &&
        Memory::IsReadable(reinterpret_cast<const void*>(GM_StreamCaptionCurrentName), sizeof(*GM_StreamCaptionCurrentName)) &&
        *GM_StreamCaptionCurrentName == kActionLevelCaptionName;
}

void MGS2_RestoreActionLevelSelection::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS2: Restore Action Level Selection: Disabled via config, skipping.");
        return;
    }

    if (uint8_t* initStoryChoices = Memory::PatternScan(baseModule,
        "C6 85 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B F0",
        "MGS2: Restore Action Level Selection - story choices | L2D_GetObject()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(initStoryChoices, 0x07, hook, "MGS2: Restore Action Level Selection - story choices", [](SafetyHookContext& ctx)
        {
            UpdateStoryChoices(ctx.rbp);
        });
    }

    if (uint8_t* getLocalResource = Memory::PatternScan(baseModule,
        "44 8B 0D ?? ?? ?? ?? 33 C0 45 85 C9 7E ?? 4C 8D 15",
        "MGS2: Restore Action Level Selection - local resources | GM_GetResource()"))
    {
        g_GlobalResInfo = reinterpret_cast<GclStringResource**>(
            Memory::GetRipRelativeAddress(getLocalResource + 0x2B, 0x03, 0x07));
    }

    if (uint8_t* showFirstTimeMenu = Memory::PatternScan(baseModule,
        "0F B6 91 ?? ?? ?? ?? 83 EA",
        "MGS2: Restore Action Level Selection - show first-time menu | kano\\titlescr\\newgame.c -> ShowGameMenu()"))
    {
        static SafetyHookMid startHook {};
        Memory::HookMidAtOffset(showFirstTimeMenu, 0x00, startHook, "MGS2: Restore Action Level Selection - show first-time menu", [](SafetyHookContext& ctx)
        {
            ShowFirstTimeEntry(ctx.rcx);
        });

        Memory::PatchBytes(reinterpret_cast<uintptr_t>(showFirstTimeMenu) + 0x1D, "\x90\x90\x90\x90\x90\x90\x90", 7);
    }

    if (uint8_t* questionTextBounds = Memory::PatternScan(baseModule,
        "B8 ?? ?? ?? ?? 41 0F 44 C4 83 C1",
        "MGS2: Restore Action Level Selection - question text bounds | skoba\\etc\\encute.c -> StringDisp()"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(questionTextBounds) + 0x0B, "\x40", 1);
    }

    if (uint8_t* questionTextPosition = Memory::PatternScan(baseModule,
        "?? ?? ?? B9 ?? ?? ?? ?? 44 0F B6 5E",
        "MGS2: Restore Action Level Selection - question text position | skoba\\etc\\encute.c -> StringDisp()"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(questionTextPosition) + 0x04, "\x44", 1);
    }

    if (uint8_t* japaneseQuestionTextTextureBounds = Memory::PatternScan(baseModule,
        "41 83 C0 ?? E8 ?? ?? ?? ?? FF C7",
        "MGS2: Restore Action Level Selection - Japanese question texture bounds | MENU_CreateTextTexture()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(japaneseQuestionTextTextureBounds, 0x04, hook, "MGS2: Restore Action Level Selection - Japanese question texture bounds", [](SafetyHookContext& ctx)
        {
            if (Util::IsJapanese())
            {
                ctx.r9 = static_cast<uint32_t>(static_cast<int32_t>(ctx.r9) + kJapaneseQuestionTextWidthPad);
            }
        });
    }

    if (uint8_t* japaneseQuestionTextPosition = Memory::PatternScan(baseModule,
        "F3 44 0F 2C D0 44 89 54 24 ?? E8",
        "MGS2: Restore Action Level Selection - Japanese question text position | skoba\\etc\\encute.c -> StringDisp()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(japaneseQuestionTextPosition, 0x05, hook, "MGS2: Restore Action Level Selection - Japanese question text position", [](SafetyHookContext& ctx)
        {
            if (Util::IsJapanese())
            {
                ctx.r8 = ShiftQuestionTextY(ctx.r8);
                ctx.r9 = static_cast<uint32_t>(static_cast<int32_t>(ctx.r9) + kJapaneseQuestionTextWidthPad);
                ctx.r10 = static_cast<uint32_t>(static_cast<int32_t>(ShiftQuestionTextY(ctx.r10)) + kJapaneseQuestionTextScreenHeightPad);
                Memory::AddStackInt32(ctx.rsp, 0x38, kJapaneseQuestionTextWidthPad);
            }
        });
    }

    if (uint8_t* captionCurrentName = Memory::PatternScan(baseModule,
        "?? ?? 89 05 ?? ?? ?? ?? 48 8B 43 ?? 8B 48",
        "MGS2: Restore Action Level Selection - caption name | GM_StreamCaptionCurrentName"))
    {
        GM_StreamCaptionCurrentName = reinterpret_cast<int32_t*>(
            Memory::GetRipRelativeAddress(captionCurrentName + 0x16, 0x02, 0x06));
    }

    MAKE_HOOK_MID(baseModule,
        "48 8B CB 48 89 43 ?? 66 89 83",
        "MGS2: Restore Action Level Selection - questionnaire route | kano\\titlescr\\newgame.c -> Step()", {
            if (Memory::ReadField<int32_t>(ctx.rbx, kWorkStep, -1) == 0 && IsFirstTimeQuestionPath(ctx.rbx))
            {
                ctx.rax = 1;
            }
        });

    MAKE_HOOK_MID(baseModule,
        "48 8B CB 48 89 53 ?? 48 83 C4",
        "MGS2: Restore Action Level Selection - difficulty cancel route | kano\\titlescr\\newgame.c -> Step()", {
            if (Memory::ReadField<int32_t>(ctx.rbx, kWorkStep, -1) == 2 && IsFirstTimeQuestionPath(ctx.rbx))
            {
                ctx.rdx = 1;
            }
        });

    if (uint8_t* cursorInit = Memory::PatternScan(baseModule,
        "44 0F BF 83 ?? ?? ?? ?? BF",
        "MGS2: Restore Action Level Selection - cursor init | kano\\titlescr\\newgame.c -> GameSubStep()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(cursorInit, 0x1B, hook, "MGS2: Restore Action Level Selection - cursor init", [](SafetyHookContext& ctx)
        {
            if (IsFirstTimeQuestionPath(ctx.rbx))
            {
                ctx.r8 = 0;
            }
        });
    }

    if (uint8_t* cursorInitTarget = Memory::PatternScan(baseModule,
        "48 8B 8C C3 ?? ?? ?? ?? 8B 81 ?? ?? ?? ?? 89 42 ?? 48 8B 83 ?? ?? ?? ?? 8B 48 ?? 49 63 C0",
        "MGS2: Restore Action Level Selection - cursor init target | kano\\titlescr\\newgame.c -> GameSubStep()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(cursorInitTarget, 0x24, hook, "MGS2: Restore Action Level Selection - cursor init target", [](SafetyHookContext& ctx)
        {
            AdjustTankerPlantCursorY(ctx.rbx, true);
        });
    }

    if (uint8_t* cursorBefore = Memory::PatternScan(baseModule,
        "0F B7 83 ?? ?? ?? ?? BF ?? ?? ?? ?? 8B F7",
        "MGS2: Restore Action Level Selection - cursor before | kano\\titlescr\\newgame.c -> GameSubStep()"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(cursorBefore) + 0x08, "\x04", 1);

        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(cursorBefore, 0x16, hook, "MGS2: Restore Action Level Selection - cursor before", [](SafetyHookContext& ctx)
        {
            if (IsFirstTimeQuestionPath(ctx.rbx))
            {
                ctx.rsi = 0;
            }
        });
    }

    if (uint8_t* cursorUpWrap = Memory::PatternScan(baseModule,
        "44 0F B6 83 ?? ?? ?? ?? 66 90",
        "MGS2: Restore Action Level Selection - cursor up wrap | kano\\titlescr\\newgame.c -> GameSubStep()"))
    {
        Memory::PatchBytes(reinterpret_cast<uintptr_t>(cursorUpWrap) + 0x14, "\x03", 1);
    }

    if (uint8_t* cursorMoveTarget = Memory::PatternScan(baseModule,
        "48 8B 8C C3 ?? ?? ?? ?? 8B 81 ?? ?? ?? ?? 89 83 ?? ?? ?? ?? 48 63 C6",
        "MGS2: Restore Action Level Selection - cursor move target | kano\\titlescr\\newgame.c -> GameSubStep()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(cursorMoveTarget, 0x14, hook, "MGS2: Restore Action Level Selection - cursor move target", [](SafetyHookContext& ctx)
        {
            AdjustTankerPlantCursorY(ctx.rbx, false);
        });
    }

    if (uint8_t* cursorAfter = Memory::PatternScan(baseModule,
        "66 89 8B ?? ?? ?? ?? 66 85 C0",
        "MGS2: Restore Action Level Selection - cursor after | kano\\titlescr\\newgame.c -> GameSubStep()"))
    {
        static SafetyHookMid hook {};
        Memory::HookMidAtOffset(cursorAfter, 0x0F, hook, "MGS2: Restore Action Level Selection - cursor after", [](SafetyHookContext& ctx)
        {
            if (IsFirstTimeQuestionPath(ctx.rbx))
            {
                ctx.rdi = 0;
            }
        });
    }

    MAKE_HOOK_MID(baseModule,
        "48 8B 05 ?? ?? ?? ?? 66 09 88 ?? ?? ?? ?? B8",
        "MGS2: Restore Action Level Selection - final story flag | kano\titlescr\newgame.c -> NewNewGameScr() -> Die()", {
            if (IsFirstTimeQuestionPath(ctx.rdi))
            {
                ctx.rcx = Memory::ReadField<int16_t>(ctx.rdi, kWorkQuestionCursor, -1) < 3 ? 3 : 2;
            }
        });

    MAKE_HOOK_MID(baseModule,
        "41 B9 ?? ?? ?? ?? C6 83 ?? ?? ?? ?? ?? BA",
        "MGS2: Restore Action Level Selection - questionnaire voice | skoba\\etc\\encute.c -> GetResources()", {
            ConvertQuestionTextToBpFont(ctx.rbx);
            SetQuestionIntroVoice(ctx.rbx);
        });
}
