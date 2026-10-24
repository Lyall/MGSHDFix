#include "stdafx.h"
#include "mg2_portrait_overlay.hpp"
#include "mg2_bridge.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"
#include "mg2_portrait_sprites.hpp"
#include "mg2_original_sprites.hpp"
#include "mg2_sprite_patcher.hpp"
#include "stb_image.h"

// #define MG2_PORTRAIT_SCANLINE_OVERWRITES // msx portraits had their face/mouth anims draw over scanlines, which honestly looks crap/buggy nowadays.


namespace
{
    constexpr int nPortraitWidth = mg2_portrait_sprites::CellWidth;
    constexpr int nPortraitHeight = mg2_portrait_sprites::CellHeight;
    constexpr int nLeftOffsetX = -113;
    constexpr int nRightOffsetX = 15;
    constexpr int nOffsetY = -82;
    constexpr int nIntroSourceX = 104;
    constexpr int nIntroSourceY = 151;
    constexpr int nIntroFaceInsetX = 8;
    constexpr int nIntroFaceInsetY = 1;
    constexpr int nIntroFaceStride = 40;
    constexpr int nIntroVisibleHeight = nPortraitHeight - 1;
    constexpr int nIntroPartCount = 3;

    constexpr int nFrameWidth = 256;
    constexpr int nFrameHeight = 256;
    constexpr size_t nFrameBytes = 0x40000;

    // MSX2: 20 Hz; mouths update every 4 ticks, eyes every 8.
    constexpr int nTickMilliseconds = 50;
    constexpr int nMouthTickShift = 2;
    constexpr int nEyeStepTickShift = 3;

    // Reveal: two passes of 12 steps, followed by a static scanline.
    // MSX2 reveal steps take 4 then 2 frames; dialogue scanlines advance every 0.1 s.
    constexpr int nRevealSteps = 12;
    constexpr int nRevealTopRowStride = 4;
    constexpr int nRevealTurnStep = 5;
    constexpr int nRevealLastRow = 46;
    constexpr int nRevealTurnRow = 22;
    constexpr int nRevealFirstPassStepMilliseconds = 67;
    constexpr int nRevealSecondPassStepMilliseconds = 33;
    constexpr int nRevealDurationMilliseconds = nRevealSteps * (nRevealFirstPassStepMilliseconds + nRevealSecondPassStepMilliseconds);
    constexpr int nScanlineRowMilliseconds = 2 * nTickMilliseconds;
    // gap between restoring and redrawing a noise row can leave one clean frame every now and then
    constexpr int nScanlineRedrawGapMilliseconds = 13;
    constexpr int nFaceTileSize = 8;
    constexpr int nFaceTileColumns = nPortraitWidth / nFaceTileSize;
    constexpr int nNoReveal = -1;

    enum Character : int
    {
        SOLID_SNAKE,
        ROY_CAMPBELL,
        HOLLY_WHITE,
        KAZUHIRA_MILLER,
        DRAGO_PETTROVICH,
        YOSEF_NORDEN,
        GRAY_FOX,
        GRAY_FOX_WOUNDED,
        KIO_MARV,
        NATASHA_MARKOVA,
        GEORGE_KESSLER,
        BIG_BOSS,
        CHARACTER_COUNT,
        CHARACTER_NONE = -1,
    };

    enum Part : int
    {
        PART_NEUTRAL,
        PART_MOUTH_TALKING,
        PART_BLINK,
        PART_ALT_DIR,
        PART_CIGARETTE_NEUTRAL_MOUTH,
        PART_CIGARETTE_TALKING_MOUTH,
        PART_COUNT,
    };

    constexpr const char* apszCharacterIds[CHARACTER_COUNT] =
    {
        "solid_snake", "roy_campbell", "holly_white", "kazuhira_miller", "drago_pettrovich", "yosef_norden",
        "gray_fox", "gray_fox_wounded", "kio_marv", "natasha_markova", "george_kessler", "big_boss",
    };

    constexpr const char* apszPartIds[PART_COUNT] =
    {
        "neutral", "mouth_talking", "blink", "alt_dir", "cigarette_neutral_mouth", "cigarette_talking_mouth",
    };

    [[nodiscard]] constexpr bool SameString(const char* pLeft, const char* pRight)
    {
        while (*pLeft && *pLeft == *pRight)
        {
            pLeft++;
            pRight++;
        }
        return *pLeft == *pRight;
    }

    [[nodiscard]] constexpr bool SheetLayoutMatches()
    {
        if (mg2_portrait_sprites::RowCount != CHARACTER_COUNT || mg2_portrait_sprites::ColumnCount != PART_COUNT)
        {
            return false;
        }

        for (int nIndex = 0; nIndex < CHARACTER_COUNT; nIndex++)
        {
            if (!SameString(mg2_portrait_sprites::Characters[nIndex], apszCharacterIds[nIndex]))
            {
                return false;
            }
        }

        for (int nIndex = 0; nIndex < PART_COUNT; nIndex++)
        {
            if (!SameString(mg2_portrait_sprites::Columns[nIndex], apszPartIds[nIndex]))
            {
                return false;
            }
        }
        return true;
    }

    static_assert(SheetLayoutMatches(), "mg2_portrait_sprites.hpp rows / columns no longer match the Character / Part enums");

    constexpr Character aeCharacterForFrame[MG2_PORTRAIT_GUSTAVA_TRANSLATING + 1] =
    {
        /* MG2_PORTRAIT_BIG_BOSS */ BIG_BOSS,
        /* MG2_PORTRAIT_GRAY_FOX */ GRAY_FOX,
        /* MG2_PORTRAIT_GRAY_FOX_DAMAGED */ GRAY_FOX_WOUNDED,
        /* MG2_PORTRAIT_HOLLY */ HOLLY_WHITE,
        /* MG2_PORTRAIT_KASLER */ GEORGE_KESSLER,
        /* MG2_PORTRAIT_MARV */ KIO_MARV,
        /* MG2_PORTRAIT_CAMPBELL */ ROY_CAMPBELL,
        /* MG2_PORTRAIT_MADNAR */ DRAGO_PETTROVICH,
        /* MG2_PORTRAIT_MASTER_MILLER */ KAZUHIRA_MILLER,
        /* MG2_PORTRAIT_GUSTAVA_EYES_OPEN */ NATASHA_MARKOVA,
        /* MG2_PORTRAIT_GUSTAVA_EYES_CLOSED */ NATASHA_MARKOVA,
        /* MG2_PORTRAIT_JACOBSEN */ YOSEF_NORDEN,
        /* MG2_PORTRAIT_SNAKE */ SOLID_SNAKE,
        /* MG2_PORTRAIT_SNAKE_SMOKING */ SOLID_SNAKE,
        /* MG2_PORTRAIT_GUSTAVA_TRANSLATING */ NATASHA_MARKOVA,
    };

    enum EyeState : int
    {
        EYES_OPEN,
        EYES_BLINK,
        EYES_GLANCE,
    };

    // Eye schedule: 0 open, 1 blink, 2 glance.
    constexpr uint8_t anEyePattern[64] =
    {
        0x40, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0xA8, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x10, 0x00, 0x00,
        0x28, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x01, 0x00, 0x04, 0x00,
        0x00, 0x40, 0x00, 0x01, 0x01, 0xA0, 0x00, 0x04, 0x00, 0x00, 0x10, 0x00, 0x04, 0x00, 0x68, 0x01,
    };
    constexpr int nEyePatternMask = 0xFF;
    constexpr int nEyeFaceBase = 0x20;
    constexpr int nEyeFaceNibbleShift = 4;

    constexpr int anEyeFaceForCharacter[CHARACTER_COUNT] =
    {
        /* SOLID_SNAKE */ 0,
        /* ROY_CAMPBELL */ 1,
        /* HOLLY_WHITE */ 2,
        /* KAZUHIRA_MILLER */ 3,
        /* DRAGO_PETTROVICH */ 4,
        /* YOSEF_NORDEN */ 5,
        /* GRAY_FOX */ 6,
        /* GRAY_FOX_WOUNDED */ 7,
        /* KIO_MARV */ 8,
        /* NATASHA_MARKOVA */ 9,
        /* GEORGE_KESSLER */ 10,
        /* BIG_BOSS */ 11,
    };

    // MSX2 palette in a 32 x 48 noise block.
    struct NoiseColor
    {
        uint8_t nRed;
        uint8_t nGreen;
        uint8_t nBlue;
        uint16_t nWeight;
    };

    constexpr NoiseColor astNoiseColors[] =
    {
        { 0, 0, 0, 534 }, { 1, 1, 1, 72 }, { 5, 3, 2, 29 }, { 2, 1, 0, 68 },
        { 7, 6, 5, 36 }, { 3, 3, 3, 7 }, { 5, 5, 5, 35 }, { 7, 7, 7, 94 },
        { 4, 4, 4, 75 }, { 0, 2, 6, 30 }, { 7, 0, 0, 14 }, { 7, 4, 3, 45 },
        { 4, 2, 1, 81 }, { 0, 4, 2, 35 }, { 0, 2, 1, 96 }, { 0, 0, 0, 285 },
    };
    constexpr int nNoiseWeightTotal = 1536;
    constexpr int nNoiseChannelMax = 7;

    struct Image
    {
        std::vector<uint32_t> vPixels; // 0xAARRGGBB, nPortraitWidth * nPortraitHeight

        [[nodiscard]] bool IsValid() const
        {
            return !vPixels.empty();
        }
    };

    std::array<std::array<Image, PART_COUNT>, CHARACTER_COUNT> s_aastImages;
    std::array<uint32_t, CHARACTER_COUNT> s_anAnimatedTiles {};
    constexpr Character aeIntroCharacters[] = { SOLID_SNAKE, HOLLY_WHITE };
    std::array<std::array<Image, nIntroPartCount>, std::size(aeIntroCharacters)> s_aastIntroImages;
    std::array<uint32_t, nFrameWidth * nFrameHeight> s_anIntroPixels {};
    bool s_bIntroDrawn = false;
    SafetyHookMid s_hIntroDraw {};
    struct DrawnPortrait
    {
        MG2_LinkVarBuf::Sprite stSprite {};
        bool bTalking = false;
        bool bRadio = false;
    };

    DrawnPortrait s_stLeftPortrait;
    DrawnPortrait s_stRightPortrait;
    bool s_bMessageTyping = false;

    SafetyHookMid s_hMessageState {};
    SafetyHookMid s_hPortraitDraw {};
    SafetyHookMid s_hPresent {};
    bool s_bRightShown = false;
    bool s_bRightRevealComplete = false;
    Character s_eRightCharacter = CHARACTER_NONE;
    int s_nRightShownMilliseconds = 0;

    [[nodiscard]] Image CutCell(const unsigned char* pSheet, int nSheetWidth, int nColumn, int nRow)
    {
        Image stImage;
        std::vector<uint32_t> vPixels(static_cast<size_t>(nPortraitWidth) * nPortraitHeight);
        bool bAnyOpaque = false;
        for (int nPixelY = 0; nPixelY < nPortraitHeight; nPixelY++)
        {
            for (int nPixelX = 0; nPixelX < nPortraitWidth; nPixelX++)
            {
                const unsigned char* pPixel = pSheet + (static_cast<size_t>(nRow * nPortraitHeight + nPixelY) * nSheetWidth + nColumn * nPortraitWidth + nPixelX) * 4;
                vPixels[static_cast<size_t>(nPixelY) * nPortraitWidth + nPixelX] = (static_cast<uint32_t>(pPixel[3]) << 24) | (static_cast<uint32_t>(pPixel[0]) << 16) | (static_cast<uint32_t>(pPixel[1]) << 8) | pPixel[2];
                bAnyOpaque |= pPixel[3] != 0;
            }
        }

        if (bAnyOpaque)
        {
            stImage.vPixels = std::move(vPixels);
        }
        return stImage;
    }

    // MSX redrew whole 8 x 8 eye/mouth tiles, including their neutral states.
    [[nodiscard]] uint32_t GetAnimatedTiles(const std::array<Image, PART_COUNT>& astParts)
    {
        uint32_t nTiles = 0;
        for (int nPart = PART_MOUTH_TALKING; nPart < PART_COUNT; nPart++)
        {
            if (!astParts[nPart].IsValid())
            {
                continue;
            }
            for (int nPixelY = 0; nPixelY < nPortraitHeight; nPixelY++)
            {
                for (int nPixelX = 0; nPixelX < nPortraitWidth; nPixelX++)
                {
                    if ((astParts[nPart].vPixels[nPixelY * nPortraitWidth + nPixelX] >> 24) != 0)
                    {
                        const int nTile = (nPixelY / nFaceTileSize) * nFaceTileColumns + nPixelX / nFaceTileSize;
                        nTiles |= 1u << nTile;
                    }
                }
            }
        }
        return nTiles;
    }

    [[nodiscard]] size_t LoadSheet()
    {
        int nWidth = 0;
        int nHeight = 0;
        int nChannels = 0;
        unsigned char* pSheet = stbi_load_from_memory(mg2_portrait_sprites::SheetPng.data(), static_cast<int>(mg2_portrait_sprites::SheetPng.size()), &nWidth, &nHeight, &nChannels, 4);
        if (!pSheet)
        {
            spdlog::error("MG2: Portrait Overlay : Failed to decode the compiled-in portrait sheet");
            return 0;
        }

        size_t nLoaded = 0;
        if (nWidth == nPortraitWidth * PART_COUNT && nHeight == nPortraitHeight * CHARACTER_COUNT)
        {
            for (int nCharacter = 0; nCharacter < CHARACTER_COUNT; nCharacter++)
            {
                for (int nPart = 0; nPart < PART_COUNT; nPart++)
                {
                    s_aastImages[nCharacter][nPart] = CutCell(pSheet, nWidth, nPart, nCharacter);
                }
                s_anAnimatedTiles[nCharacter] = GetAnimatedTiles(s_aastImages[nCharacter]);

                if (s_aastImages[nCharacter][PART_NEUTRAL].IsValid())
                {
                    nLoaded++;
                }
                else
                {
                    spdlog::warn("MG2: Portrait Overlay : No neutral for {}, keeping the game's portrait", apszCharacterIds[nCharacter]);
                }
            }
        }
        else
        {
            spdlog::error("MG2: Portrait Overlay : The portrait sheet is {} x {}, expected {} x {}", nWidth, nHeight, nPortraitWidth * PART_COUNT, nPortraitHeight * CHARACTER_COUNT);
        }

        stbi_image_free(pSheet);
        return nLoaded;
    }

    void LoadIntroSheet()
    {
        int nWidth = 0;
        int nHeight = 0;
        int nChannels = 0;
        unsigned char* pSheet = stbi_load_from_memory(mg2_portrait_sprites::IntroSheetPng.data(), static_cast<int>(mg2_portrait_sprites::IntroSheetPng.size()), &nWidth, &nHeight, &nChannels, 4);
        if (!pSheet || nWidth != nPortraitWidth * nIntroPartCount || nHeight != nPortraitHeight * static_cast<int>(std::size(aeIntroCharacters)))
        {
            spdlog::error("MG2: Portrait Overlay : Invalid intro portrait sheet.");
            stbi_image_free(pSheet);
            return;
        }

        for (size_t nCharacter = 0; nCharacter < s_aastIntroImages.size(); nCharacter++)
        {
            for (int nPart = 0; nPart < nIntroPartCount; nPart++)
            {
                s_aastIntroImages[nCharacter][nPart] = CutCell(pSheet, nWidth, nPart, static_cast<int>(nCharacter));
            }
        }
        stbi_image_free(pSheet);
    }

    void Blit(uint32_t* pFrame, const Image& stImage, int nX, int nY)
    {
        if (!stImage.IsValid())
        {
            return;
        }

        for (int nPixelY = 0; nPixelY < nPortraitHeight; nPixelY++)
        {
            const int nDestY = nY + nPixelY;
            if (nDestY < 0 || nDestY >= nFrameHeight)
            {
                continue;
            }

            for (int nPixelX = 0; nPixelX < nPortraitWidth; nPixelX++)
            {
                const int nDestX = nX + nPixelX;
                const uint32_t nSource = stImage.vPixels[static_cast<size_t>(nPixelY) * nPortraitWidth + nPixelX];
                const uint32_t nAlpha = nSource >> 24;
                if (nDestX < 0 || nDestX >= nFrameWidth || nAlpha == 0)
                {
                    continue;
                }

                uint32_t& nDest = pFrame[nDestY * nFrameWidth + nDestX];
                if (nAlpha == 0xFF)
                {
                    nDest = nSource & 0x00FFFFFF;
                    continue;
                }

                uint32_t nBlended = 0;
                for (int nShift = 0; nShift <= 16; nShift += 8)
                {
                    const uint32_t nSourceChannel = (nSource >> nShift) & 0xFF;
                    const uint32_t nDestChannel = (nDest >> nShift) & 0xFF;
                    nBlended |= ((nSourceChannel * nAlpha + nDestChannel * (0xFF - nAlpha)) / 0xFF) << nShift;
                }
                nDest = nBlended;
            }
        }
    }

    [[nodiscard]] int GameMilliseconds()
    {
        static const std::chrono::steady_clock::time_point s_tStart = std::chrono::steady_clock::now();
        return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s_tStart).count());
    }

    [[nodiscard]] EyeState GetEyeState(int nTick, Character eCharacter)
    {
        int nIndex = nTick >> nEyeStepTickShift;
        const int nFace = anEyeFaceForCharacter[eCharacter];
        if (nFace != 0)
        {
            nIndex += nEyeFaceBase + (((nFace << nEyeFaceNibbleShift) | (nFace >> nEyeFaceNibbleShift)) & 0xFF);
        }
        nIndex &= nEyePatternMask;
        const int nShift = 6 - 2 * (nIndex & 3);
        return static_cast<EyeState>((anEyePattern[nIndex >> 2] >> nShift) & 3);
    }

    [[nodiscard]] bool IsMouthOpen(int nTick, bool bTalking)
    {
        return bTalking && ((nTick >> nMouthTickShift) & 1) != 0;
    }

    [[nodiscard]] constexpr int RevealTopRow(int nStep)
    {
        return nStep <= nRevealTurnStep ? nStep * nRevealTopRowStride : nRevealTurnRow - (nStep - (nRevealTurnStep + 1)) * nRevealTopRowStride;
    }

    [[nodiscard]] bool IsRowRevealed(int nRow, int nElapsedMilliseconds)
    {
        const int nParity = nRow & 1;
        const int nEvenRow = nRow - nParity;
        for (int nStep = 0; nStep < nRevealSteps; nStep++)
        {
            const int nTop = RevealTopRow(nStep);
            if (nTop == nEvenRow || nRevealLastRow - nTop == nEvenRow)
            {
                const int nStepStart = nParity == 0 ? nStep * nRevealFirstPassStepMilliseconds : nRevealSteps * nRevealFirstPassStepMilliseconds + nStep * nRevealSecondPassStepMilliseconds;
                return nElapsedMilliseconds >= nStepStart;
            }
        }
        return true;
    }

    void HideUnrevealedRows(uint32_t* pFrame, int nElapsedMilliseconds, int nX, int nY)
    {
        for (int nRow = 0; nRow < nPortraitHeight; nRow++)
        {
            const int nDestY = nY + nRow;
            if (nDestY < 0 || nDestY >= nFrameHeight || IsRowRevealed(nRow, nElapsedMilliseconds))
            {
                continue;
            }

            for (int nPixelX = 0; nPixelX < nPortraitWidth; nPixelX++)
            {
                const int nDestX = nX + nPixelX;
                if (nDestX >= 0 && nDestX < nFrameWidth)
                {
                    pFrame[nDestY * nFrameWidth + nDestX] = 0;
                }
            }
        }
    }

    [[nodiscard]] uint32_t NoiseHash(uint32_t nValue)
    {
        nValue ^= nValue >> 16;
        nValue *= 0x7FEB352Du;
        nValue ^= nValue >> 15;
        nValue *= 0x846CA68Bu;
        nValue ^= nValue >> 16;
        return nValue;
    }

    // Even ticks draw noise last; odd ticks let eye/mouth tiles overwrite it.
    void DrawScanline(uint32_t* pFrame, int nElapsedMilliseconds, int nX, int nY, uint32_t nAnimatedTiles)
    {
        if (nElapsedMilliseconds % nScanlineRowMilliseconds < nScanlineRedrawGapMilliseconds)
        {
            return;
        }

        const int nRow = (nElapsedMilliseconds / nScanlineRowMilliseconds) % nPortraitHeight;
#ifdef MG2_PORTRAIT_SCANLINE_OVERWRITES
        const bool bRedrawFeatures = ((nElapsedMilliseconds / nTickMilliseconds) & 1) != 0;
#endif
        const int nDestY = nY + nRow;
        if (nDestY < 0 || nDestY >= nFrameHeight)
        {
            return;
        }

        for (int nPixelX = 0; nPixelX < nPortraitWidth; nPixelX++)
        {
            const int nDestX = nX + nPixelX;
            if (nDestX < 0 || nDestX >= nFrameWidth)
            {
                continue;
            }
#ifdef MG2_PORTRAIT_SCANLINE_OVERWRITES
            const int nTile = (nRow / nFaceTileSize) * nFaceTileColumns + nPixelX / nFaceTileSize;
            if (bRedrawFeatures && (nAnimatedTiles & (1u << nTile)) != 0)
            {
                continue;
            }
#endif

            int nPick = static_cast<int>(NoiseHash(static_cast<uint32_t>(nRow * nPortraitWidth + nPixelX)) % nNoiseWeightTotal);
            const NoiseColor* pColor = &astNoiseColors[0];
            for (const NoiseColor& stColor : astNoiseColors)
            {
                pColor = &stColor;
                nPick -= stColor.nWeight;
                if (nPick < 0)
                {
                    break;
                }
            }

            pFrame[nDestY * nFrameWidth + nDestX] = (static_cast<uint32_t>(pColor->nRed * 255 / nNoiseChannelMax) << 16) | (static_cast<uint32_t>(pColor->nGreen * 255 / nNoiseChannelMax) << 8) | static_cast<uint32_t>(pColor->nBlue * 255 / nNoiseChannelMax);
        }
    }

    void DrawPortrait(uint32_t* pFrame, int nTick, const MG2_LinkVarBuf::Sprite& stSprite, int nOffsetX, Character eCharacter, bool bSmoking, bool bEyesClosed, bool bTalking, int nRevealMilliseconds)
    {
        const auto& astParts = s_aastImages[eCharacter];
        const int nDyingEyes = eCharacter == NATASHA_MARKOVA ? MG2_Bridge::GetDyingEyeState() : -1;
        const EyeState eEyes = nDyingEyes >= 0 ? static_cast<EyeState>(nDyingEyes) : GetEyeState(nTick, eCharacter);
        const bool bMouthOpen = IsMouthOpen(nTick, bTalking);

        const int nX = stSprite.nX + nOffsetX;
        const int nY = stSprite.nY + nOffsetY;
        Blit(pFrame, astParts[PART_NEUTRAL], nX, nY);

        if ((bEyesClosed && nDyingEyes < 0) || eEyes == EYES_BLINK)
        {
            Blit(pFrame, astParts[PART_BLINK], nX, nY);
        }
        else if (eEyes == EYES_GLANCE)
        {
            Blit(pFrame, astParts[PART_ALT_DIR], nX, nY);
        }

        if (bMouthOpen)
        {
            Blit(pFrame, astParts[PART_MOUTH_TALKING], nX, nY);
        }

        if (bSmoking)
        {
            Blit(pFrame, astParts[bMouthOpen ? PART_CIGARETTE_TALKING_MOUTH : PART_CIGARETTE_NEUTRAL_MOUTH], nX, nY);
        }

        if (nRevealMilliseconds != nNoReveal)
        {
            if (nRevealMilliseconds < nRevealDurationMilliseconds)
            {
                HideUnrevealedRows(pFrame, nRevealMilliseconds, nX, nY);
            }
            else
            {
                DrawScanline(pFrame, nRevealMilliseconds - nRevealDurationMilliseconds, nX, nY, s_anAnimatedTiles[eCharacter]);
            }
        }
    }

    void CaptureMessageState(const char* pText, int nLength)
    {
        s_bMessageTyping = false;
        const int nPos = MG2_LinkVarBuf::GM_MessagePos;
        if (!pText || nPos < 0 || nPos >= nLength || MG2_LinkVarBuf::GM_MessageSpeed == 0)
        {
            return;
        }

        // MessageDone misses page waits (\w / \W); stop talking there too.
        const bool bPageWait = nLength - nPos >= 2 && pText[nPos] == '\\' && (pText[nPos + 1] == 'w' || pText[nPos + 1] == 'W');
        s_bMessageTyping = !bPageWait;
    }

    void DrawIntroPortraits(int nPanelX, int nPanelY, const int* pClip)
    {
        const int nTick = GameMilliseconds() / nTickMilliseconds;
        auto* pLayer = MG2_Gamefuncs::pfnGetLayerImage(MG2_LAYER_SPRITES);
        const int nPreviousColour = pLayer->nColour;
        const int nLeft = std::max({ 0, pClip[0], pLayer->nClipLeft });
        const int nTop = std::max({ 0, pClip[1], pLayer->nClipTop });
        const int nRight = std::min({ nFrameWidth, pClip[0] + pClip[2], pLayer->nClipRight });
        const int nBottom = std::min({ nFrameHeight, pClip[1] + pClip[3], pLayer->nClipBottom });
        MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_SPRITES, 0);
        for (size_t nCharacter = 0; nCharacter < s_aastIntroImages.size(); nCharacter++)
        {
            const auto& astParts = s_aastIntroImages[nCharacter];
            if (!astParts[0].IsValid())
            {
                continue;
            }
            const EyeState eEyes = GetEyeState(nTick, aeIntroCharacters[nCharacter]);
            const int nX = nPanelX + nIntroFaceInsetX + static_cast<int>(nCharacter) * nIntroFaceStride;
            const int nY = nPanelY + nIntroFaceInsetY;
            for (int nRow = 0; nRow < nIntroVisibleHeight; nRow++)
            {
                for (int nColumn = 0; nColumn < nPortraitWidth; nColumn++)
                {
                    const int nDestX = nX + nColumn;
                    const int nDestY = nY + nRow;
                    if (nDestX < nLeft || nDestX >= nRight || nDestY < nTop || nDestY >= nBottom)
                    {
                        continue;
                    }
                    const int nSource = nRow * nPortraitWidth + nColumn;
                    uint32_t nPixel = astParts[0].vPixels[nSource];
                    if ((eEyes == EYES_BLINK || eEyes == EYES_GLANCE) && astParts[eEyes].IsValid() && (astParts[eEyes].vPixels[nSource] >> 24) != 0)
                    {
                        nPixel = astParts[eEyes].vPixels[nSource];
                    }
                    if ((nPixel >> 24) == 0)
                    {
                        continue;
                    }
                    s_anIntroPixels[nDestY * nFrameWidth + nDestX] = nPixel;
                    s_bIntroDrawn = true;
                }
                if (nY + nRow < nTop || nY + nRow >= nBottom)
                {
                    continue;
                }
                for (int nColumn = std::max(nX, nLeft); nColumn < std::min(nX + nPortraitWidth, nRight);)
                {
                    if (s_anIntroPixels[(nY + nRow) * nFrameWidth + nColumn] == 0)
                    {
                        nColumn++;
                        continue;
                    }
                    const int nStart = nColumn++;
                    while (nColumn < std::min(nX + nPortraitWidth, nRight) && s_anIntroPixels[(nY + nRow) * nFrameWidth + nColumn] != 0)
                    {
                        nColumn++;
                    }
                    // Sprite blits never write index zero. Later opaque tracks replace this marker.
                    const int anRun[] = { nStart, nY + nRow, nColumn - nStart, 1 };
                    MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_SPRITES, anRun);
                }
            }
        }
        MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_SPRITES, nPreviousColour);
    }

    void CompositeIntroPortraits(uint32_t* pFrame, const uint8_t* pIndexedEnd, const uint32_t* pPalette)
    {
        if (!s_bIntroDrawn)
        {
            return;
        }
        const uint32_t nBrightness = (pPalette[1] >> 16) & 0xFF;
        for (int nPixel = 0; nPixel < nFrameWidth * nFrameHeight; nPixel++)
        {
            if (s_anIntroPixels[nPixel] != 0 && pIndexedEnd[nPixel - nFrameWidth * nFrameHeight] == 0)
            {
                pFrame[nPixel] = (((s_anIntroPixels[nPixel] >> 16) & 0xFF) * nBrightness / 0xFF) << 16;
            }
        }
        s_anIntroPixels.fill(0);
        s_bIntroDrawn = false;
    }

    void CapturePortrait(const MG2_LinkVarBuf::Sprite& stSprite)
    {
        if (stSprite.nSheet != MG2_SPRITE_SHEET_PORTRAITS || stSprite.nAnim != MG2_PORTRAIT_ANIMATION_STATIC)
        {
            return;
        }

        // Copy visible sprites before scene teardown releases their handles.
        const bool bTalking = s_bMessageTyping && !MG2_LinkVarBuf::GM_MessageDone;
        const bool bSnakeSpeaking = MG2_LinkVarBuf::GM_SpeakerIsSnake != 0;
        if (&stSprite == MG2_LinkVarBuf::GetSprite(MG2_SPRITE_LIST_TRANSCEIVER, MG2_TRANSCEIVER_SLOT_SNAKE))
        {
            s_stLeftPortrait = { stSprite, bTalking && bSnakeSpeaking };
        }
        else if (&stSprite == MG2_LinkVarBuf::GetSprite(MG2_SPRITE_LIST_TRANSCEIVER, MG2_TRANSCEIVER_SLOT_CONTACT))
        {
            s_stRightPortrait = { stSprite, bTalking && !bSnakeSpeaking, MG2_LinkVarBuf::GM_Scene == MG2_SCENE_TRANSCEIVER };
        }
    }

    void Composite(uint32_t* pFrame)
    {
        const int nMilliseconds = GameMilliseconds();
        const int nTick = nMilliseconds / nTickMilliseconds;
        s_bRightRevealComplete = false;

        if (s_stRightPortrait.stSprite.nShown)
        {
            const MG2_LinkVarBuf::Sprite* pRight = &s_stRightPortrait.stSprite;
            const int nFrame = pRight->nTime;
            if (nFrame >= MG2_PORTRAIT_BIG_BOSS && nFrame <= MG2_PORTRAIT_JACOBSEN)
            {
                const Character eCharacter = aeCharacterForFrame[nFrame];
                if (s_aastImages[eCharacter][PART_NEUTRAL].IsValid())
                {
                    if (s_stRightPortrait.bRadio && (!s_bRightShown || eCharacter != s_eRightCharacter))
                    {
                        s_eRightCharacter = eCharacter;
                        s_nRightShownMilliseconds = nMilliseconds;
                    }
                    s_bRightShown = s_stRightPortrait.bRadio;
                    const int nRevealMilliseconds = s_stRightPortrait.bRadio ? nMilliseconds - s_nRightShownMilliseconds : nNoReveal;
                    DrawPortrait(pFrame, nTick, *pRight, nRightOffsetX, eCharacter, false, nFrame == MG2_PORTRAIT_GUSTAVA_EYES_CLOSED, s_stRightPortrait.bTalking, nRevealMilliseconds);
                    s_bRightRevealComplete = !s_stRightPortrait.bRadio || nRevealMilliseconds >= nRevealDurationMilliseconds;
                }
            }
            else
            {
                s_bRightShown = false;
            }
        }
        else
        {
            s_bRightShown = false;
        }

        if (s_stLeftPortrait.stSprite.nShown)
        {
            const MG2_LinkVarBuf::Sprite* pLeft = &s_stLeftPortrait.stSprite;
            const int nFrame = pLeft->nTime;
            if (nFrame >= MG2_PORTRAIT_SNAKE && nFrame <= MG2_PORTRAIT_GUSTAVA_TRANSLATING)
            {
                const Character eCharacter = aeCharacterForFrame[nFrame];
                if (s_aastImages[eCharacter][PART_NEUTRAL].IsValid())
                {
                    DrawPortrait(pFrame, nTick, *pLeft, nLeftOffsetX, eCharacter, nFrame == MG2_PORTRAIT_SNAKE_SMOKING, false, s_stLeftPortrait.bTalking, nNoReveal);
                }
            }
        }

        // Clear frame copies to avoid stale portraits or mouth anims.
        s_stLeftPortrait = {};
        s_stRightPortrait = {};
        s_bMessageTyping = false;
    }
}

bool MG2_PortraitOverlay::IsContactRevealPending()
{
    if (!bEnabled || !s_hMessageState || !s_hPortraitDraw || !s_hPresent || MG2_LinkVarBuf::GM_Scene != MG2_SCENE_TRANSCEIVER)
    {
        return false;
    }

    const MG2_LinkVarBuf::Sprite* pContact = MG2_LinkVarBuf::GetSprite(MG2_SPRITE_LIST_TRANSCEIVER, MG2_TRANSCEIVER_SLOT_CONTACT);
    if (!pContact || !pContact->nShown || pContact->nSheet != MG2_SPRITE_SHEET_PORTRAITS || pContact->nAnim != MG2_PORTRAIT_ANIMATION_STATIC || pContact->nTime < MG2_PORTRAIT_BIG_BOSS || pContact->nTime > MG2_PORTRAIT_JACOBSEN)
    {
        return false;
    }

    const Character eCharacter = aeCharacterForFrame[pContact->nTime];
    return s_aastImages[eCharacter][PART_NEUTRAL].IsValid() && (!s_bRightShown || s_eRightCharacter != eCharacter || !s_bRightRevealComplete);
}

void MG2_PortraitOverlay::Initialize()
{
    if (!bEnabled)
    {
        return;
    }

    if (MG2_SpritePatcher::bAllowReplacements)
    {
        for (const mg2_original_sprites::PaletteColour& stColour : mg2_original_sprites::astColours)
        {
            if (!MG2_SpritePatcher::AddPalettePatch(stColour.nEntry, stColour.nIndex, stColour.nRgb))
            {
                spdlog::error("MG2: Original Sprites : Could not register palette {} colour {}.", stColour.nEntry, stColour.nIndex);
                return;
            }
        }
        for (const mg2_original_sprites::ImagePatch& stImage : mg2_original_sprites::astImages)
        {
            if (!MG2_SpritePatcher::AddPatch(stImage.nEntry, stImage.nHash, stImage.pBlob, stImage.nSize))
            {
                spdlog::error("MG2: Original Sprites : Could not register image {}.", stImage.nEntry);
                return;
            }
        }
    }

    if (MG2_SpritePatcher::bAllowReplacements)
    {
        spdlog::info("MG2: Original Sprites : Image and palette patches registered.");
    }

    if (LoadSheet() == 0)
    {
        spdlog::error("MG2: Portrait Overlay : No usable portraits in the compiled-in sheet. Skipping.");
        return;
    }

    LoadIntroSheet();

    //DrawMessageText+234
    uint8_t* pMessageState = Memory::PatternScan(mg2Module, "48 63 3D", "MG2: Portrait Overlay : DrawMessageText()+0x234 : completion");
    //DrawSprite+EE
    uint8_t* pPortraitDraw = Memory::PatternScan(mg2Module, "44 8B 56 ?? 45 33 C0", "MG2: Portrait Overlay : DrawSprite()+0xEE : visible sprite");
    //DrawSprite+412
    uint8_t* pIntroDraw = Memory::PatternScan(mg2Module, "41 2B 7D ?? ?? ?? ?? ?? ?? ?? ?? EB", "MG2: Portrait Overlay : DrawSprite()+0x412 : intro portrait track");
    //PresentIndexedFrame+95
    uint8_t* pPresent = Memory::PatternScan(mg2Module, "48 8B 05 ?? ?? ?? ?? 48 8D 8F", "MG2: Portrait Overlay : PresentIndexedFrame()+0x95 : upload");
    if (!pMessageState || !pPortraitDraw || !pIntroDraw || !pPresent || !MG2_Gamefuncs::pfnGetLayerImage || !MG2_Gamefuncs::pfnSetDrawColor || !MG2_Gamefuncs::pfnLayerFillRect)
    {
        spdlog::error("MG2: Portrait Overlay : Missing draw or message hook address. Skipping.");
    }
    else
    {
        // r14 = localized text; [rsp+0x40] = byte length, after reveal paths
        s_hMessageState = safetyhook::create_mid(pMessageState, [](SafetyHookContext& stContext)
        {
            CaptureMessageState(reinterpret_cast<const char*>(stContext.r14), *reinterpret_cast<const int*>(stContext.rsp + 0x40));
        });
        LOG_HOOK(s_hMessageState, "MG2: Portrait Overlay : DrawMessageText()+0x234 : completion")

        // rsi = visible sprite, before animation advances.
        // [rsp+0x38] is draw mode: zero draws into Present's game layer.
        s_hPortraitDraw = safetyhook::create_mid(pPortraitDraw, [](SafetyHookContext& stContext)
        {
            if (*reinterpret_cast<const int*>(stContext.rsp + 0x38) == 0)
            {
                CapturePortrait(*reinterpret_cast<const MG2_LinkVarBuf::Sprite*>(stContext.rsi));
            }
        });
        LOG_HOOK(s_hPortraitDraw, "MG2: Portrait Overlay : DrawSprite()+0xEE : visible sprite")

        s_hIntroDraw = safetyhook::create_mid(pIntroDraw, [](SafetyHookContext& stContext)
        {
            const auto* pSprite = reinterpret_cast<const MG2_LinkVarBuf::Sprite*>(stContext.rsi);
            if (*reinterpret_cast<const int*>(stContext.rsp + 0x38) != 0 || stContext.r13 != reinterpret_cast<uintptr_t>(&pSprite->nFirstTrackFrame) || pSprite->nSheet != MG2_SPRITE_SHEET_INTRO || pSprite->nAnim != MG2_INTRO_ANIMATION_MOVIE || pSprite->nFirstTrackFrame != MG2_INTRO_FRAME_RED_PORTRAITS)
            {
                return;
            }
            const int* pSource = reinterpret_cast<const int*>(stContext.rsp + 0x68);
            const int nX = static_cast<int>(stContext.rbx);
            const int nY = *reinterpret_cast<const int*>(stContext.rsp + 0x20);
            const int anClip[] = { nX, nY, pSource[2], pSource[3] };
            DrawIntroPortraits(nX - (pSource[0] - nIntroSourceX), nY - (pSource[1] - nIntroSourceY), anClip);
        });
        LOG_HOOK(s_hIntroDraw, "MG2: Portrait Overlay : DrawSprite()+0x412 : intro portrait track")

        // rdi = end of the 256 x 256 texture
        s_hPresent = safetyhook::create_mid(pPresent, [](SafetyHookContext& stContext)
        {
            uint32_t* pFrame = reinterpret_cast<uint32_t*>(stContext.rdi - nFrameBytes);
            CompositeIntroPortraits(pFrame, reinterpret_cast<const uint8_t*>(stContext.rsi), reinterpret_cast<const uint32_t*>(stContext.rdx));
            Composite(pFrame);
        });
        LOG_HOOK(s_hPresent, "MG2: Portrait Overlay : PresentIndexedFrame()+0x95 : upload")

        if (!s_hMessageState || !s_hPortraitDraw || !s_hIntroDraw || !s_hPresent)
        {
            s_hIntroDraw.reset();
            s_hPresent.reset();
            s_hPortraitDraw.reset();
            s_hMessageState.reset();
            spdlog::error("MG2: Portrait Overlay : Incomplete hook setup. Disabled all portrait hooks.");
        }
        else
        {
            spdlog::info("MG2: Portrait Overlay : Draw and message hooks initialized.");
        }
    }
}
