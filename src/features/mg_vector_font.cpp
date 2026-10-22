#include "stdafx.h"
#include "mg_vector_font.hpp"
#include "mg_vector_font_renderer.hpp"

#include "common.hpp"
#include "d3d11_api.hpp"
#include "gamevars.hpp"
#include "mg1_linkvarbuf.hpp"
#include "mg2_linkvarbuf.hpp"
#include "mg_font_bundle.hpp"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace
{
    struct Settings
    {
        const char* pszGame;
        int* pTextFade;
        MG_Gamefuncs::SetFont_t pfnSetFont;
        MG_Gamefuncs::DrawText_t pfnDrawText;
        MG_Gamefuncs::ComposeTextLayer_t pfnComposeTextLayer;
        MG_Gamefuncs::MeasureTextFont_t pfnMeasureTextFont;
        MG_Gamefuncs::UploadTexture_t pfnUploadTexture;
    };

    constexpr std::array<int, 4> anFontHeights = { 20, 12, 18, 24 };
    constexpr int nLayerWidth = MG_VectorFontRenderer::nLayerWidth;
    constexpr int nLayerHeight = MG_VectorFontRenderer::nLayerHeight;
    constexpr size_t nLayerPixels = nLayerWidth * nLayerHeight;
    constexpr size_t nMaximumTextBytes = 4096;
    constexpr size_t nMaximumCachedGlyphs = 4096;

    struct Letter
    {
        int nCodepoint;
        int nX;
        int nAdvance;
    };

    struct GameRenderer
    {
        Settings stSettings {};
        SafetyHookInline hSetFont;
        SafetyHookInline hDrawText;
        SafetyHookInline hCompose;
        std::mutex stFontMutex;
        std::array<std::vector<Letter>, MGFont::nCount> avLetters;
        uint64_t nFontRevision = 0;
        uint64_t nWarmedRevision = 0;
        uint64_t nWarmedGeneration = 0;
    };

    std::array<GameRenderer, 2> s_astGames;
    thread_local GameRenderer* s_pGame = nullptr;
    SafetyHookInline s_hUpload;
    SafetyHookInline s_hSetResources;
    std::vector<uint8_t> s_vFontData;
    stbtt_fontinfo s_stFont {};

    struct Glyph
    {
        MG_VectorFontRenderer::GlyphLocation stLocation;
        bool bReady = true;
        int nWidth = 0;
        int nHeight = 0;
        int nX = 0;
        int nY = 0;
    };

    struct TextCommand
    {
        uint32_t nColor;
        int nX;
        int nY;
        int nFont;
        bool bDraw;
        std::string sText;
        int nNarrationOpacity = -1;
        bool bOutline = false;

        bool operator==(const TextCommand&) const = default;
    };

    std::mutex s_stTextureMutex;
    std::atomic<ID3D11ShaderResourceView*> s_pActiveOriginal = nullptr;
    ComPtr<ID3D11ShaderResourceView> s_pOriginalView;
    ComPtr<ID3D11ShaderResourceView> s_pReplacementView;
    MG_VectorFontRenderer::Renderer s_stRenderer;
    std::vector<MG_VectorFontRenderer::Vertex> s_vVertices;
    std::vector<MG_VectorFontRenderer::Batch> s_vBatches;
    ID3D11Device* s_pFailedDevice = nullptr;
    std::unordered_map<uint64_t, Glyph> s_stGlyphs;
    uint32_t* s_pFrame = nullptr;
    int s_nScale = 0;
    int s_nFade = 255;
    thread_local bool s_bComposing = false;
    bool s_bRasterFailed = false;
    std::vector<uint32_t> s_vBasePixels;
    std::vector<TextCommand> s_vCommands;
    std::vector<TextCommand> s_vCachedCommands;
    GameRenderer* s_pCachedGame = nullptr;
    int s_nCachedScale = 0;
    bool s_bCachedRasterFailed = false;
    uint64_t s_nRevision = 0;
    uint64_t s_nGlyphGeneration = 0;
    std::atomic<bool> s_bWarmPending = false;

    SafetyHookInline s_hDraw, s_hDrawIndexed;
    std::atomic<ID3D11ShaderResourceView*> s_pNarrationView = nullptr;
    ComPtr<ID3D11BlendState> s_pNarrationBlend;

    struct NarrationBlend
    {
        ID3D11DeviceContext* pContext;
        ComPtr<ID3D11BlendState> pOriginal;
        float afFactor[4] {};
        UINT nMask = 0;
        bool bChanged = false;

        explicit NarrationBlend(ID3D11DeviceContext* pDrawContext) : pContext(pDrawContext)
        {
            auto* pNarration = s_pNarrationView.load(std::memory_order_acquire);
            if (!pNarration)
            {
                return;
            }
            ComPtr<ID3D11ShaderResourceView> pView;
            pContext->PSGetShaderResources(0, 1, &pView);
            if (pView.Get() != pNarration)
            {
                return;
            }
            pContext->OMGetBlendState(&pOriginal, afFactor, &nMask);
            pContext->OMSetBlendState(s_pNarrationBlend.Get(), afFactor, nMask);
            bChanged = true;
        }

        ~NarrationBlend()
        {
            if (bChanged)
            {
                pContext->OMSetBlendState(pOriginal.Get(), afFactor, nMask);
            }
        }
    };

    void __stdcall Draw_hook(ID3D11DeviceContext* pContext, UINT nCount, UINT nStart)
    {
        NarrationBlend stBlend(pContext);
        s_hDraw.call<void>(pContext, nCount, nStart);
    }

    void __stdcall DrawIndexed_hook(ID3D11DeviceContext* pContext, UINT nCount, UINT nStart, INT nBase)
    {
        NarrationBlend stBlend(pContext);
        s_hDrawIndexed.call<void>(pContext, nCount, nStart, nBase);
    }

    void WarmGlyphCache(GameRenderer& stGame);

    void __stdcall SetResources_hook(ID3D11DeviceContext* pContext, UINT nStart, UINT nCount, ID3D11ShaderResourceView* const* ppViews)
    {
        if (!ppViews || nCount > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
        {
            s_hSetResources.call<void>(pContext, nStart, nCount, ppViews);
            return;
        }
        ID3D11ShaderResourceView* pOriginal = s_pActiveOriginal.load(std::memory_order_acquire);
        if (!pOriginal || std::find(ppViews, ppViews + nCount, pOriginal) == ppViews + nCount)
        {
            s_hSetResources.call<void>(pContext, nStart, nCount, ppViews);
            return;
        }
        ComPtr<ID3D11ShaderResourceView> pReplacement;
        {
            std::lock_guard<std::mutex> stLock(s_stTextureMutex);
            if (s_pOriginalView.Get() == pOriginal)
            {
                pReplacement = s_pReplacementView;
            }
        }
        if (!pReplacement)
        {
            s_hSetResources.call<void>(pContext, nStart, nCount, ppViews);
            return;
        }
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> apViews;
        for (UINT nIndex = 0; nIndex < nCount; nIndex++)
        {
            apViews[nIndex] = ppViews[nIndex] == pOriginal ? pReplacement.Get() : ppViews[nIndex];
        }
        s_hSetResources.call<void>(pContext, nStart, nCount, apViews.data());
    }

    bool PrepareRenderer()
    {
        if (!g_D3D11Hooks.d3dDevice || !g_D3D11Hooks.d3dDeviceContext || !g_D3D11Hooks.swapChain)
        {
            return false;
        }
        if (!s_hSetResources)
        {
            void** ppTable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
            s_hSetResources = safetyhook::create_inline(ppTable[8], SetResources_hook);
            s_hDraw = safetyhook::create_inline(ppTable[13], Draw_hook);
            s_hDrawIndexed = safetyhook::create_inline(ppTable[12], DrawIndexed_hook);
            LOG_HOOK(s_hSetResources, "MG1/MG2: Vector Font : ID3D11DeviceContext::PSSetShaderResources()")
            LOG_HOOK(s_hDraw, "MG1/MG2: Vector Font : ID3D11DeviceContext::Draw()")
            LOG_HOOK(s_hDrawIndexed, "MG1/MG2: Vector Font : ID3D11DeviceContext::DrawIndexed()")
            if (!s_hSetResources || !s_hDraw || !s_hDrawIndexed)
            {
                s_hSetResources.reset();
                s_hDraw.reset();
                s_hDrawIndexed.reset();
                return false;
            }
        }
        DXGI_SWAP_CHAIN_DESC stDesc {};
        if (FAILED(g_D3D11Hooks.swapChain->GetDesc(&stDesc)))
        {
            return false;
        }
        const int nScale = std::clamp(static_cast<int>(std::ceil((std::max)(stDesc.BufferDesc.Width * 0.75f / nLayerWidth, float(stDesc.BufferDesc.Height) / nLayerHeight))), 2, 8);
        auto* pDevice = g_D3D11Hooks.d3dDevice.Get();
        if (s_pFailedDevice == pDevice || !g_D3D11Hooks.D3DCompileFunc)
        {
            return false;
        }
        const bool bNewDevice = !s_stRenderer.IsDevice(pDevice);
        if (!s_stRenderer.Initialize(pDevice, g_D3D11Hooks.d3dDeviceContext.Get(), g_D3D11Hooks.D3DCompileFunc))
        {
            s_pFailedDevice = pDevice;
            spdlog::error("MG1/MG2: Vector Font : GPU renderer unavailable. Using native text.");
            return false;
        }
        if (bNewDevice || !s_pNarrationBlend)
        {
            D3D11_BLEND_DESC stBlend {};
            auto& stTarget = stBlend.RenderTarget[0];
            stTarget.BlendEnable = TRUE;
            stTarget.SrcBlend = D3D11_BLEND_SRC_ALPHA;
            stTarget.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            stTarget.BlendOp = stTarget.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            stTarget.SrcBlendAlpha = D3D11_BLEND_ONE;
            stTarget.DestBlendAlpha = D3D11_BLEND_ZERO;
            stTarget.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            s_pNarrationBlend.Reset();
            if (FAILED(pDevice->CreateBlendState(&stBlend, &s_pNarrationBlend)))
            {
                return false;
            }
        }
        if (nScale != s_nScale || bNewDevice || s_stGlyphs.size() >= nMaximumCachedGlyphs)
        {
            s_nScale = nScale;
            s_stGlyphs.clear();
            s_stRenderer.ResetGlyphs();
            s_pCachedGame = nullptr;
            ++s_nGlyphGeneration;
            s_bWarmPending.store(true, std::memory_order_release);
        }
        if (bNewDevice)
        {
            spdlog::info("MG1/MG2: Vector Font : GPU renderer initialized.");
        }
        return true;
    }

    bool DecodeLetters(char* pszText, int nFont, int nX, std::vector<Letter>& vLetters)
    {
        const size_t nLength = strnlen(pszText, nMaximumTextBytes);
        if (nLength == nMaximumTextBytes)
        {
            return false;
        }
        for (size_t nIndex = 0; nIndex < nLength;)
        {
            const auto nChar = static_cast<uint8_t>(pszText[nIndex]);
            if (nChar == '|')
            {
                if (nIndex + 1 == nLength)
                {
                    break;
                }
                nX += 24; // pass controller icons natively.
                nIndex += 2;
                continue;
            }
            const bool bWide = nFont == 0 && nChar >= 0x80 && static_cast<uint8_t>(nChar + 0x60) > 0x3F;
            if (nFont != 0 && nChar >= 0x80 && static_cast<uint8_t>(nChar + 0x60) > 0x3F)
            {
                return false; // these slots use a different byte-to-glyph mapping.
            }
            const int nBytes = bWide ? 2 : 1;
            if (nIndex + nBytes > nLength)
            {
                return false;
            }
            wchar_t awCodepoint[2] {};
            if (MultiByteToWideChar(bWide ? 932 : 1252, MB_ERR_INVALID_CHARS, pszText + nIndex, nBytes, awCodepoint, 2) != 1
                || stbtt_FindGlyphIndex(&s_stFont, awCodepoint[0]) == 0)
            {
                return false;
            }
            const int nAdvance = bWide ? 24 : static_cast<int>(s_pGame->stSettings.pfnMeasureTextFont(pszText, static_cast<int>(nIndex), nBytes, nFont));
            if (nAdvance < 0 || nAdvance > 64)
            {
                return false;
            }
            vLetters.push_back({ awCodepoint[0], nX, nAdvance });
            nX += nAdvance;
            nIndex += nBytes;
        }
        return true;
    }

    const Glyph& GetGlyph(const Letter& stLetter, int nFont, bool bOutline = false)
    {
        const uint64_t nKey = uint64_t(stLetter.nCodepoint) | (uint64_t(stLetter.nAdvance) << 32) | (uint64_t(nFont) << 40) | (uint64_t(bOutline) << 48);
        const auto stFound = s_stGlyphs.find(nKey);
        if (stFound != s_stGlyphs.end())
        {
            s_bRasterFailed |= !stFound->second.bReady;
            return stFound->second;
        }
        Glyph stGlyph;
        int nAdvance = 0;
        int nBearing = 0;
        int nAscent = 0;
        int nDescent = 0;
        int nLineGap = 0;
        stbtt_GetCodepointHMetrics(&s_stFont, stLetter.nCodepoint, &nAdvance, &nBearing);
        stbtt_GetFontVMetrics(&s_stFont, &nAscent, &nDescent, &nLineGap);
        const float fScaleY = stbtt_ScaleForPixelHeight(&s_stFont, float(anFontHeights[nFont] * s_nScale));
        const float fScaleX = nAdvance > 0 ? float(stLetter.nAdvance * s_nScale) / nAdvance : fScaleY;
        unsigned char* pBitmap = stbtt_GetCodepointBitmap(&s_stFont, fScaleX, fScaleY, stLetter.nCodepoint,
            &stGlyph.nWidth, &stGlyph.nHeight, &stGlyph.nX, &stGlyph.nY);
        stGlyph.nY += static_cast<int>(std::round(nAscent * fScaleY));
        if (pBitmap)
        {
            std::vector<uint8_t> vOutline;
            const uint8_t* pCoverage = pBitmap;
            if (bOutline)
            {
                const int nRadius = s_nScale;
                const int nWidth = stGlyph.nWidth + 2 * nRadius;
                const int nHeight = stGlyph.nHeight + 2 * nRadius;
                vOutline.resize(static_cast<size_t>(nWidth) * nHeight);
                for (int nY = 0; nY < stGlyph.nHeight; ++nY)
                {
                    for (int nX = 0; nX < stGlyph.nWidth; ++nX)
                    {
                        const uint8_t nCoverage = pBitmap[nY * stGlyph.nWidth + nX];
                        for (int nDY = -nRadius; nDY <= nRadius; ++nDY)
                        {
                            for (int nDX = -nRadius; nDX <= nRadius; ++nDX)
                            {
                                if (nDX * nDX + nDY * nDY <= nRadius * nRadius)
                                {
                                    auto& nPixel = vOutline[(nY + nRadius + nDY) * nWidth + nX + nRadius + nDX];
                                    nPixel = std::max(nPixel, nCoverage);
                                }
                            }
                        }
                    }
                }
                stGlyph.nX -= nRadius;
                stGlyph.nY -= nRadius;
                stGlyph.nWidth = nWidth;
                stGlyph.nHeight = nHeight;
                pCoverage = vOutline.data();
            }
            if (!s_stRenderer.AddGlyph(pCoverage, stGlyph.nWidth, stGlyph.nHeight, stGlyph.stLocation))
            {
                stGlyph.bReady = false;
            }
            stbtt_FreeBitmap(pBitmap, nullptr);
        }
        else if (stGlyph.nWidth > 0 && stGlyph.nHeight > 0)
        {
            stGlyph.bReady = false;
        }
        s_bRasterFailed |= !stGlyph.bReady;
        return s_stGlyphs.emplace(nKey, std::move(stGlyph)).first->second;
    }

    const uint8_t* GetFontData(uintptr_t hFont, size_t nSize)
    {
        auto** ppResource = reinterpret_cast<MGFont::Resource**>(hFont);
        if (!Memory::IsReadable(ppResource, sizeof(*ppResource)) || !Memory::IsReadable(*ppResource, sizeof(**ppResource)))
        {
            return nullptr;
        }
        const uint8_t* pData = (*ppResource)->pData;
        return Memory::IsReadable(pData, nSize) ? pData : nullptr;
    }

    template <size_t nGame>
    void __fastcall SetFont_hook(int nFont, uintptr_t hGlyphs, uintptr_t hKanji, int nHeight)
    {
        GameRenderer& stGame = s_astGames[nGame];
        stGame.hSetFont.call<void>(nFont, hGlyphs, hKanji, nHeight);
        if (nFont < 0 || nFont >= MGFont::nCount)
        {
            return;
        }
        std::vector<Letter> vLetters;
        const uint8_t* pGlyphs = GetFontData(hGlyphs, MGFont::nGlyphDataOffset);
        if (pGlyphs)
        {
            for (int nChar = MGFont::nFirstCharacter; nChar < MGFont::nFirstCharacter + MGFont::nCharacterCount; ++nChar)
            {
                if (nChar == '|' || (nChar >= 0x80 && static_cast<uint8_t>(nChar + 0x60) > 0x3F))
                {
                    continue;
                }
                uint32_t nRecord = 0;
                memcpy(&nRecord, pGlyphs + MGFont::nGlyphTableOffset + (nChar - MGFont::nFirstCharacter) * MGFont::nGlyphRecordSize, sizeof(nRecord));
                if ((nRecord >> MGFont::nWidthShift) == MGFont::nUnusedGlyph)
                {
                    continue;
                }
                const char chText = static_cast<char>(nChar);
                wchar_t wCodepoint = 0;
                if (MultiByteToWideChar(1252, MB_ERR_INVALID_CHARS, &chText, 1, &wCodepoint, 1) == 1 && stbtt_FindGlyphIndex(&s_stFont, wCodepoint) != 0)
                {
                    vLetters.push_back({ wCodepoint, 0, int((nRecord >> MGFont::nWidthShift) & MGFont::nWidthMask) + 1 });
                }
            }
        }
        const uint8_t* pKanji = nFont == 0 ? GetFontData(hKanji, MGFont::nKanjiDataOffset) : nullptr;
        if (pKanji)
        {
            for (int nCode = MGFont::nKanjiFirstCode; nCode <= 0xFFFF; ++nCode)
            {
                const auto nLead = static_cast<uint8_t>(nCode >> 8);
                if (static_cast<uint8_t>(nLead + 0x60) <= 0x3F)
                {
                    continue;
                }
                const int nOffset = MGFont::nKanjiTableOffset + (nCode - MGFont::nKanjiFirstCode) * MGFont::nKanjiRecordSize;
                if (pKanji[nOffset] == 0 && pKanji[nOffset + 1] == 0)
                {
                    continue;
                }
                const char achText[] = { static_cast<char>(nLead), static_cast<char>(nCode) };
                wchar_t wCodepoint = 0;
                if (MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, achText, 2, &wCodepoint, 1) == 1 && stbtt_FindGlyphIndex(&s_stFont, wCodepoint) != 0)
                {
                    vLetters.push_back({ wCodepoint, 0, MGFont::nKanjiWidth });
                }
            }
        }
        {
            std::lock_guard<std::mutex> stLock(stGame.stFontMutex);
            stGame.avLetters[nFont] = std::move(vLetters);
            ++stGame.nFontRevision;
        }
        s_bWarmPending.store(true, std::memory_order_release);
    }

    void WarmGlyphCache(GameRenderer& stGame)
    {
        std::lock_guard<std::mutex> stLock(stGame.stFontMutex);
        if (stGame.nFontRevision == 0 || (stGame.nWarmedRevision == stGame.nFontRevision && stGame.nWarmedGeneration == s_nGlyphGeneration))
        {
            return;
        }
        const auto stStart = std::chrono::steady_clock::now();
        const size_t nPreviousCount = s_stGlyphs.size();
        const bool bPreviousFailure = s_bRasterFailed;
        bool bComplete = true;
        for (int nFont = 0; nFont < MGFont::nCount && bComplete; ++nFont)
        {
            for (const Letter& stLetter : stGame.avLetters[nFont])
            {
                if (s_stGlyphs.size() >= nMaximumCachedGlyphs - 128)
                {
                    bComplete = false;
                    break;
                }
                if (!GetGlyph(stLetter, nFont).bReady)
                {
                    bComplete = false;
                    break;
                }
            }
        }
        s_bRasterFailed = bPreviousFailure;
        stGame.nWarmedRevision = stGame.nFontRevision;
        stGame.nWarmedGeneration = s_nGlyphGeneration;
        const double fMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - stStart).count();
        spdlog::info("{}: Vector Font : Prewarmed {} glyphs at {}x in {:.1f} ms.", stGame.stSettings.pszGame, s_stGlyphs.size() - nPreviousCount, s_nScale, fMilliseconds);
        if (!bComplete)
        {
            spdlog::warn("{}: Vector Font : Prewarm stopped at the cache limit or an atlas allocation failure.", stGame.stSettings.pszGame);
        }
    }

    void DrawLetter(const Letter& stLetter, int nFont, int nY, uint32_t nColor, bool bOutline = false, float fOpacity = 1.0f)
    {
        const Glyph& stGlyph = GetGlyph(stLetter, nFont, bOutline);
        if (stGlyph.nWidth <= 0 || stGlyph.nHeight <= 0 || s_bRasterFailed)
        {
            return;
        }
        const float fX = float(stLetter.nX) * s_nScale + stGlyph.nX;
        const float fY = float(nY) * s_nScale + stGlyph.nY;
        const float fU = float(stGlyph.stLocation.nX);
        const float fV = float(stGlyph.stLocation.nY);
        const float fWidth = float(stGlyph.nWidth);
        const float fHeight = float(stGlyph.nHeight);
        if (s_vBatches.empty() || s_vBatches.back().nPage != stGlyph.stLocation.nPage)
        {
            s_vBatches.push_back({ stGlyph.stLocation.nPage, static_cast<UINT>(s_vVertices.size()), 0 });
        }
        s_vVertices.insert(s_vVertices.end(), {
            { fX, fY, fU, fV, nColor, fOpacity },
            { fX + fWidth, fY, fU + fWidth, fV, nColor, fOpacity },
            { fX, fY + fHeight, fU, fV + fHeight, nColor, fOpacity },
            { fX, fY + fHeight, fU, fV + fHeight, nColor, fOpacity },
            { fX + fWidth, fY, fU + fWidth, fV, nColor, fOpacity },
            { fX + fWidth, fY + fHeight, fU + fWidth, fV + fHeight, nColor, fOpacity }
        });
        s_vBatches.back().nCount += 6;
    }

    template <size_t nGame>
    void __fastcall DrawText_hook(uint32_t* pFrame, int* pText)
    {
        GameRenderer& stGame = s_astGames[nGame];
        if (!s_bComposing || s_pGame != &stGame)
        {
            stGame.hDrawText.call<void>(pFrame, pText);
            return;
        }
        if (!s_pFrame)
        {
            s_pFrame = pFrame;
            s_vBasePixels.assign(pFrame, pFrame + nLayerPixels);
        }
        const char* pszText = reinterpret_cast<const char*>(pText + 4);
        const size_t nLength = strnlen(pszText, nMaximumTextBytes);
        TextCommand stCommand { static_cast<uint32_t>(pText[0]), pText[1], pText[2], pText[3], false, {} };
        if constexpr (nGame == 1)
        {
            if (MG2_LinkVarBuf::GM_Scene == MG2_SCENE_INTRO && MG2_LinkVarBuf::GM_IntroPhase == MG2_INTRO_PHASE_STORY && stCommand.nFont == 0)
            {
                stCommand.nNarrationOpacity = static_cast<int>(stCommand.nColor & 0xFF) * s_nFade / 255;
                stCommand.bOutline = MG_VectorFont::bOpeningNarrationOutline;
            }
        }
        if (nLength == nMaximumTextBytes)
        {
            s_bRasterFailed = true;
        }
        else
        {
            stCommand.sText.assign(pszText, nLength);
        }
        stGame.hDrawText.call<void>(pFrame, pText);
        stCommand.bDraw = pText[1] != stCommand.nX;
        s_vCommands.push_back(std::move(stCommand));
    }

    bool PrepareTextLayer()
    {
        if (s_bRasterFailed)
        {
            return false;
        }
        if (s_pCachedGame == s_pGame && s_nCachedScale == s_nScale && s_vCachedCommands == s_vCommands)
        {
            return !s_bCachedRasterFailed;
        }
        s_pCachedGame = s_pGame;
        s_nCachedScale = s_nScale;
        s_vCachedCommands = s_vCommands;
        s_bCachedRasterFailed = false;
        ++s_nRevision;
        s_vVertices.clear();
        s_vBatches.clear();
        std::vector<Letter> vLetters;
        for (auto& stCommand : s_vCommands)
        {
            vLetters.clear();
            if (stCommand.nFont < 0 || stCommand.nFont >= 4 || !DecodeLetters(stCommand.sText.data(), stCommand.nFont, stCommand.nX, vLetters))
            {
                s_bCachedRasterFailed = true;
                return false;
            }
            if (stCommand.bDraw)
            {
                if (stCommand.bOutline && stCommand.nNarrationOpacity > 0)
                {
                    for (const Letter& stLetter : vLetters)
                    {
                        DrawLetter(stLetter, stCommand.nFont, stCommand.nY, 0, true, float(stCommand.nNarrationOpacity) / 255.0f);
                    }
                }
                for (const Letter& stLetter : vLetters)
                {
                    if (stCommand.nNarrationOpacity >= 0)
                    {
                        DrawLetter(stLetter, stCommand.nFont, stCommand.nY, 0xFFFFFF, false, float(stCommand.nNarrationOpacity) / 255.0f);
                    }
                    else
                    {
                        DrawLetter(stLetter, stCommand.nFont, stCommand.nY, stCommand.nColor);
                    }
                }
            }
        }
        s_bCachedRasterFailed = s_bRasterFailed;
        return !s_bCachedRasterFailed;
    }

    void UploadReplacement(ID3D11ShaderResourceView* pOriginal)
    {
        ComPtr<ID3D11Resource> pResource;
        ComPtr<ID3D11Texture2D> pOriginalTexture;
        pOriginal->GetResource(&pResource);
        if (FAILED(pResource.As(&pOriginalTexture)))
        {
            return;
        }
        D3D11_TEXTURE2D_DESC stDesc {};
        pOriginalTexture->GetDesc(&stDesc);
        if (stDesc.Width != nLayerWidth || stDesc.Height != nLayerHeight || stDesc.SampleDesc.Count != 1
            || (stDesc.Format != DXGI_FORMAT_R8G8B8A8_UNORM && stDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM))
        {
            return;
        }
        const bool bNarration = !s_vCommands.empty() && std::all_of(s_vCommands.begin(), s_vCommands.end(), [](const TextCommand& stCommand) { return stCommand.nNarrationOpacity >= 0; });
        ID3D11ShaderResourceView* pReplacement = s_stRenderer.Render(s_vBasePixels, s_vVertices, s_vBatches, s_nRevision, s_nScale, bNarration ? 255 : s_nFade, stDesc.Format);
        if (!pReplacement)
        {
            return;
        }
        std::lock_guard<std::mutex> stLock(s_stTextureMutex);
        s_pOriginalView = pOriginal;
        s_pReplacementView = pReplacement;
        s_pNarrationView.store(bNarration ? pReplacement : nullptr, std::memory_order_release);
        s_pActiveOriginal.store(pOriginal, std::memory_order_release);
    }

    int64_t __fastcall UploadTexture_hook(void* pTexture, uint32_t* pPixels)
    {
        const int64_t nResult = s_hUpload.call<int64_t>(pTexture, pPixels);
        if (s_bComposing && s_pFrame == pPixels && PrepareTextLayer())
        {
            auto* pHostTexture = static_cast<MGHostTexture*>(pTexture);
            if (pHostTexture->pShaderResourceView)
            {
                UploadReplacement(pHostTexture->pShaderResourceView);
            }
        }
        return nResult;
    }

    template <size_t nGame>
    void __fastcall Compose_hook(int nDrawText)
    {
        GameRenderer& stGame = s_astGames[nGame];
        s_pGame = &stGame;
        s_pNarrationView.store(nullptr, std::memory_order_release);
        s_pActiveOriginal.store(nullptr, std::memory_order_release);
        s_pFrame = nullptr;
        s_bRasterFailed = false;
        s_vCommands.clear();
        s_nFade = std::clamp(*stGame.stSettings.pTextFade, 0, 255);
        const bool bRendererReady = MG_VectorFont::bEnabled && PrepareRenderer();
        if (bRendererReady)
        {
            WarmGlyphCache(stGame);
        }
        s_bComposing = nDrawText != 0 && bRendererReady;
        stGame.hCompose.call<void>(nDrawText);
        s_bComposing = false;
        s_pFrame = nullptr;
        s_pGame = nullptr;
    }

    bool LoadFont()
    {
        const auto stPath = sExePath / "launcher_Data/StreamingAssets/aa/StandaloneWindows64/defaultlocalgroup_assets_launcher/fonts/mg-rodinpron-m.otf.bundle";
        std::ifstream stFile(stPath, std::ios::binary | std::ios::ate);
        if (!stFile)
        {
            return false;
        }
        const auto nSize = stFile.tellg();
        if (nSize <= 0 || static_cast<uint64_t>(nSize) > MG_FontData::nMaximumBytes)
        {
            return false;
        }
        std::vector<uint8_t> vBundle(static_cast<size_t>(nSize));
        stFile.seekg(0);
        if (!stFile.read(reinterpret_cast<char*>(vBundle.data()), nSize) || !MG_FontData::Unpack(vBundle, s_vFontData))
        {
            return false;
        }
        return stbtt_InitFont(&s_stFont, s_vFontData.data(), 0) != 0;
    }

    void Initialize(size_t nGame, const Settings& stSettings)
    {
        if (nGame >= s_astGames.size() || !MG_VectorFont::bEnabled || s_astGames[nGame].hCompose)
        {
            return;
        }
        if (s_vFontData.empty() && !LoadFont())
        {
            s_vFontData.clear();
            spdlog::error("{}: Vector Font : Unable to parse font file from the unity launcher's files. Aborting setup!", stSettings.pszGame);
            return;
        }
        if (!stSettings.pfnSetFont || !stSettings.pfnDrawText || !stSettings.pfnComposeTextLayer || !stSettings.pfnMeasureTextFont || !stSettings.pfnUploadTexture || !stSettings.pTextFade)
        {
            spdlog::error("{}: Vector Font : Couldn't find text renderer. Aborting setup.", stSettings.pszGame);
        }
        else
        {
            GameRenderer& stGame = s_astGames[nGame];
            stGame.stSettings = stSettings;
            if (!s_hUpload)
            {
                s_hUpload = safetyhook::create_inline(stSettings.pfnUploadTexture, UploadTexture_hook);
                LOG_HOOK(s_hUpload, "MG1/MG2: Vector Font : sub_140025F80() : upload")
            }
            stGame.hSetFont = safetyhook::create_inline(stSettings.pfnSetFont, nGame == 0 ? SetFont_hook<0> : SetFont_hook<1>);
            LOG_HOOK(stGame.hSetFont, std::string(stSettings.pszGame) + ": Vector Font : register font")
            stGame.hDrawText = safetyhook::create_inline(stSettings.pfnDrawText, nGame == 0 ? DrawText_hook<0> : DrawText_hook<1>);
            LOG_HOOK(stGame.hDrawText, std::string(stSettings.pszGame) + ": Vector Font : draw text")
            stGame.hCompose = safetyhook::create_inline(stSettings.pfnComposeTextLayer, nGame == 0 ? Compose_hook<0> : Compose_hook<1>);
            LOG_HOOK(stGame.hCompose, std::string(stSettings.pszGame) + ": Vector Font : compose")
            if (!stGame.hSetFont || !stGame.hDrawText || !s_hUpload || !stGame.hCompose)
            {
                stGame.hSetFont.reset();
                stGame.hCompose.reset();
                stGame.hDrawText.reset();
                if (!s_astGames[0].hCompose && !s_astGames[1].hCompose)
                {
                    s_hUpload.reset();
                }
                spdlog::error("{}: Vector Font : Missing hook! Setup aborted.", stSettings.pszGame);
            }
            else
            {
                spdlog::info("{}: Vector Font : Text hooks initialized.", stSettings.pszGame);
            }
        }
    }
}

void MG_VectorFont::OnPresent()
{
    if (!bEnabled || !s_bWarmPending.exchange(false, std::memory_order_acq_rel))
    {
        return;
    }
    if (!PrepareRenderer())
    {
        s_bWarmPending.store(true, std::memory_order_release);
        return;
    }
    for (GameRenderer& stGame : s_astGames)
    {
        WarmGlyphCache(stGame);
    }
}

void MG_VectorFont::InitializeMG1()
{
    Initialize(0, { "MG1", MG1_LinkVarBuf::pTextFade, MG1_Gamefuncs::pfnSetFont, MG1_Gamefuncs::pfnDrawTextSlot, MG1_Gamefuncs::pfnComposeTextLayer, MG1_Gamefuncs::pfnMeasureTextFont, MG1_Gamefuncs::pfnUploadTexture });
}

void MG_VectorFont::InitializeMG2()
{
    Initialize(1, { "MG2", MG2_LinkVarBuf::pTextFade, MG2_Gamefuncs::pfnSetFont, MG2_Gamefuncs::pfnDrawTextItem, MG2_Gamefuncs::pfnComposeTextLayer, MG2_Gamefuncs::pfnMeasureTextFont, MG2_Gamefuncs::pfnUploadTexture });
}
