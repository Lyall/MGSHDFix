#include "stdafx.h"
#include "mg1_opening.hpp"
#include "mg1_opening_sprites.hpp"

#include "common.hpp"
#include "d3d11_api.hpp"
#include "gamevars.hpp"
#include "mg1_linkvarbuf.hpp"

using namespace MG1_LinkVarBuf;

namespace
{
    struct Glitch
    {
        int nAnimation;
        int nKeyframe;
        int nSourceX;
        int nX;
        int nY;
        int nWidth;
        int nHeight;
    };

    constexpr Glitch astGlitches[] =
    {
        { 0, 36, 0, -8, 82, 16, 14 },
        { 0, 43, 16, -8, 82, 16, 9 },
        { 0, 92, 32, -86, 63, 16, 15 },
        { 1, 31, 48, -7, 29, 15, 16 },
    };
    constexpr int anBounceY[] = { -19, -24, -29, -31, -33, -32 };

    SafetyHookMid s_hDraw;
    SafetyHookMid s_hDuration;
    SafetyHookMid s_hPresent;
    SafetyHookInline s_hUpload;
    SafetyHookInline s_hSetResources;
    std::mutex s_stMutex;
    std::atomic<bool> s_bRestorePending = false;
    bool s_bInitialized = false;
    bool s_bReplacementBound = false;
    bool s_bRegionPending = false;
    int s_nLastAnimation = -1;
    int s_nLastKeyframe = -1;
    Sprite* s_pLastSprite = nullptr;
    int s_nX = 0;
    int s_nY = 0;
    int s_nWidth = 0;
    int s_nHeight = 0;
    std::array<uint8_t, 32 * 64> s_anRegion;
    std::vector<uint32_t> s_vGlitchPixels;
    uint32_t* s_pUploadPixels = nullptr;
    ComPtr<ID3D11ShaderResourceView> s_pOriginalView;
    ComPtr<ID3D11ShaderResourceView> s_pGlitchView;

    bool IsOpening(const Sprite* pSprite)
    {
        return s_bInitialized && sceneState->GM_CurrentScene == MG1_SCENE_OPENING && pSprite->nSheet == MG1_SPRITE_SHEET_OPENING;
    }

    void DrawSprite(SafetyHookContext& ctx)
    {
        auto* pSprite = reinterpret_cast<Sprite*>(ctx.rbx);
        if (!IsOpening(pSprite))
        {
            return;
        }
        const int nKeyframe = pSprite->anTrackFrames[0];
        if (pSprite->nAnimation == 1 && nKeyframe >= 42 && nKeyframe < 48)
        {
            ctx.rcx = static_cast<uint32_t>(pSprite->nY + anBounceY[nKeyframe - 42]);
        }
        if (s_pLastSprite == pSprite && s_nLastAnimation == pSprite->nAnimation && s_nLastKeyframe == nKeyframe)
        {
            return;
        }
        s_pLastSprite = pSprite;
        s_nLastAnimation = pSprite->nAnimation;
        s_nLastKeyframe = nKeyframe;
        const auto pGlitch = std::find_if(std::begin(astGlitches), std::end(astGlitches), [&](const Glitch& stGlitch)
        {
            return stGlitch.nAnimation == pSprite->nAnimation && stGlitch.nKeyframe == nKeyframe;
        });
        if (pGlitch == std::end(astGlitches) || !g_D3D11Hooks.swapChain || pSprite->nPaletteIndex < 0 || pSprite->nPaletteIndex > 1)
        {
            return;
        }
        const int nMode = *reinterpret_cast<int*>(ctx.rbp - 0x29);
        const int nLayer = nMode == 0 ? MG1_LAYER_COMPOSITE : static_cast<int>(GM_CurrentDrawLayer);
        if (nLayer < 0 || nLayer > MG1_LAYER_COMPOSITE)
        {
            return;
        }
        const DrawLayer& stLayer = pDrawLayers[nLayer];
        if (!stLayer.hPixels)
        {
            return;
        }
        const auto* pPixels = *reinterpret_cast<const uint8_t* const*>(stLayer.hPixels);
        const int nNormalX = static_cast<int>(ctx.r11);
        const int nNormalY = static_cast<int>(ctx.rcx);
        const int nNormalWidth = static_cast<int>(ctx.rdx);
        const int nNormalHeight = static_cast<int>(ctx.r8);
        const int nGlitchX = pSprite->nX + pGlitch->nX;
        const int nGlitchY = pSprite->nY + pGlitch->nY;
        const int nLeft = std::max(0, std::min(nNormalX, nGlitchX));
        const int nTop = std::max(0, std::min(nNormalY, nGlitchY));
        const int nRight = std::min({ nFrameBufferWidth, stLayer.nWidth, std::max(nNormalX + nNormalWidth, nGlitchX + pGlitch->nWidth) });
        const int nBottom = std::min({ nFrameBufferHeight, stLayer.nHeight, std::max(nNormalY + nNormalHeight, nGlitchY + pGlitch->nHeight) });
        const int nWidth = nRight - nLeft;
        const int nHeight = nBottom - nTop;
        if (!pPixels || nWidth <= 0 || nHeight <= 0 || nWidth * nHeight > static_cast<int>(s_anRegion.size()))
        {
            return;
        }
        std::lock_guard<std::mutex> stLock(s_stMutex);
        s_nX = nLeft;
        s_nY = nTop;
        s_nWidth = nWidth;
        s_nHeight = nHeight;
        for (int nY = 0; nY < nHeight; nY++)
        {
            std::copy_n(pPixels + (nTop + nY) * stLayer.nWidth + nLeft, nWidth, s_anRegion.data() + nY * nWidth);
        }
        const auto* pGlitchPixels = pSprite->nPaletteIndex == 0 ? MG1_OpeningSprites::anPixels0 : MG1_OpeningSprites::anPixels1;
        for (int nY = 0; nY < pGlitch->nHeight; nY++)
        {
            for (int nX = 0; nX < pGlitch->nWidth; nX++)
            {
                const int nXOut = nGlitchX + nX - nLeft;
                const int nYOut = nGlitchY + nY - nTop;
                const uint8_t nColour = pGlitchPixels[nY * MG1_OpeningSprites::nWidth + pGlitch->nSourceX + nX];
                if (nColour && nXOut >= 0 && nXOut < nWidth && nYOut >= 0 && nYOut < nHeight)
                {
                    s_anRegion[nYOut * nWidth + nXOut] = nColour;
                }
            }
        }
        s_bRegionPending = true;
    }

    void SetDuration(SafetyHookContext& ctx)
    {
        const auto* pSprite = reinterpret_cast<const Sprite*>(ctx.rbx);
        if (!IsOpening(pSprite) || pSprite->nAnimation != 0)
        {
            return;
        }
        const int nKeyframe = static_cast<int>(ctx.r11);
        if (nKeyframe == 42 && static_cast<int>(ctx.rcx) == 4)
        {
            ctx.rcx = 7;
        }
        else if (nKeyframe == 92 && static_cast<int>(ctx.rcx) == 14)
        {
            ctx.rcx = 32;
        }
    }

    void PrepareUpload(SafetyHookContext& ctx)
    {
        std::lock_guard<std::mutex> stLock(s_stMutex);
        s_pUploadPixels = nullptr;
        if (!s_bRegionPending)
        {
            return;
        }
        s_bRegionPending = false;
        if (sceneState->GM_CurrentScene != MG1_SCENE_OPENING)
        {
            return;
        }
        s_pUploadPixels = reinterpret_cast<uint32_t*>(ctx.rdi - nFrameBufferWidth * nFrameBufferHeight * sizeof(uint32_t));
    }

    void __stdcall SetResources_hook(ID3D11DeviceContext* pContext, UINT nStart, UINT nCount, ID3D11ShaderResourceView* const* ppViews)
    {
        if (!s_bRestorePending.load(std::memory_order_acquire) || !ppViews || nCount > D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
        {
            s_hSetResources.call<void>(pContext, nStart, nCount, ppViews);
            return;
        }
        std::lock_guard<std::mutex> stLock(s_stMutex);
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> apViews;
        for (UINT nIndex = 0; nIndex < nCount; nIndex++)
        {
            const bool bReplace = s_pOriginalView && ppViews[nIndex] == s_pOriginalView.Get();
            apViews[nIndex] = bReplace ? s_pGlitchView.Get() : ppViews[nIndex];
            s_bReplacementBound |= bReplace;
        }
        s_hSetResources.call<void>(pContext, nStart, nCount, apViews.data());
    }

    int64_t __fastcall UploadTexture_hook(void* pTexture, uint32_t* pPixels)
    {
        const int64_t nResult = s_hUpload.call<int64_t>(pTexture, pPixels);
        std::lock_guard<std::mutex> stLock(s_stMutex);
        if (!s_pUploadPixels || pPixels != s_pUploadPixels)
        {
            return nResult;
        }
        s_pUploadPixels = nullptr;
        auto* pView = static_cast<MGHostTexture*>(pTexture)->pShaderResourceView;
        if (!pView || !g_D3D11Hooks.d3dDevice || !g_D3D11Hooks.d3dDeviceContext)
        {
            return nResult;
        }
        ComPtr<ID3D11Resource> pResource;
        ComPtr<ID3D11Texture2D> pTexture2D;
        pView->GetResource(&pResource);
        if (FAILED(pResource.As(&pTexture2D)))
        {
            return nResult;
        }
        D3D11_TEXTURE2D_DESC stDesc {};
        pTexture2D->GetDesc(&stDesc);
        if (stDesc.Width != nFrameBufferWidth || stDesc.Height == 0 || stDesc.Height > nFrameBufferHeight || stDesc.SampleDesc.Count != 1
            || (stDesc.Format != DXGI_FORMAT_R8G8B8A8_UNORM && stDesc.Format != DXGI_FORMAT_B8G8R8A8_UNORM))
        {
            spdlog::warn("MG1: Opening : Unsupported frame texture: {}x{}, format {}.", stDesc.Width, stDesc.Height, static_cast<int>(stDesc.Format));
            return nResult;
        }
        s_vGlitchPixels.assign(pPixels, pPixels + static_cast<size_t>(stDesc.Width) * stDesc.Height);
        for (int nY = 0; nY < s_nHeight; nY++)
        {
            const int nOutputY = s_nY + nY + *pOutputTopRows;
            if (nOutputY < 0 || nOutputY >= static_cast<int>(stDesc.Height))
            {
                continue;
            }
            for (int nX = 0; nX < s_nWidth; nX++)
            {
                uint32_t nColour = pOutputPalette[s_anRegion[nY * s_nWidth + nX]];
                if (nColour != 0 && (nColour & 0xFF000000) == 0)
                {
                    nColour |= 0xFF000000;
                }
                s_vGlitchPixels[nOutputY * nFrameBufferWidth + s_nX + nX] = nColour;
            }
        }
        stDesc.MipLevels = 1;
        stDesc.ArraySize = 1;
        stDesc.Usage = D3D11_USAGE_IMMUTABLE;
        stDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        stDesc.CPUAccessFlags = 0;
        stDesc.MiscFlags = 0;
        D3D11_SHADER_RESOURCE_VIEW_DESC stViewDesc {};
        pView->GetDesc(&stViewDesc);
        stViewDesc.Texture2D.MostDetailedMip = 0;
        stViewDesc.Texture2D.MipLevels = 1;
        const D3D11_SUBRESOURCE_DATA stData { s_vGlitchPixels.data(), nFrameBufferWidth * sizeof(uint32_t), 0 };
        ComPtr<ID3D11Texture2D> pGlitchTexture;
        ComPtr<ID3D11ShaderResourceView> pGlitchView;
        HRESULT hrResult = g_D3D11Hooks.d3dDevice->CreateTexture2D(&stDesc, &stData, &pGlitchTexture);
        if (SUCCEEDED(hrResult))
        {
            hrResult = g_D3D11Hooks.d3dDevice->CreateShaderResourceView(pGlitchTexture.Get(), &stViewDesc, &pGlitchView);
        }
        if (FAILED(hrResult))
        {
            spdlog::warn("MG1: Opening : Glitch texture creation failed: {:#x}.", static_cast<uint32_t>(hrResult));
            return nResult;
        }
        s_bReplacementBound = false;
        s_pOriginalView = pView;
        s_pGlitchView = pGlitchView;
        s_bRestorePending.store(true, std::memory_order_release);
        return nResult;
    }
}

void MG1_Opening::OnPresent()
{
    if (!s_bRestorePending.load(std::memory_order_acquire))
    {
        return;
    }
    std::lock_guard<std::mutex> stLock(s_stMutex);
    if (!s_bReplacementBound && sceneState->GM_CurrentScene == MG1_SCENE_OPENING)
    {
        return;
    }
    s_bRestorePending.store(false, std::memory_order_release);
    auto* pContext = g_D3D11Hooks.d3dDeviceContext.Get();
    if (pContext)
    {
        std::array<ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT> apViews {};
        pContext->PSGetShaderResources(0, static_cast<UINT>(apViews.size()), apViews.data());
        for (UINT nIndex = 0; nIndex < apViews.size(); nIndex++)
        {
            if (apViews[nIndex] == s_pGlitchView.Get())
            {
                auto* pOriginal = s_pOriginalView.Get();
                s_hSetResources.call<void>(pContext, nIndex, 1, &pOriginal);
            }
            if (apViews[nIndex])
            {
                apViews[nIndex]->Release();
            }
        }
    }
    s_pOriginalView.Reset();
    s_pGlitchView.Reset();
}

void MG1_Opening::Initialize()
{
    if (!bEnabled)
    {
        return;
    }
    //sub_180017720+36A
    uint8_t* pDraw = Memory::PatternScan(mg1Module, "83 7D ?? 00 41 0F B6 44 31", "MG1: Opening : DrawSprite()+0x36A : opening artwork");
    //sub_180017720+4CD
    uint8_t* pDuration = Memory::PatternScan(mg1Module, "44 3B F1 7C ?? 41 FF C3 45 3B DA", "MG1: Opening : DrawSprite()+0x4CD : idle_swimming");
    //PresentIndexedFrame+95
    uint8_t* pPresent = Memory::PatternScan(mg1Module, "48 8B 05 ?? ?? ?? ?? 48 8D 8F", "MG1: Opening : PresentIndexedFrame()+0x95 : upload");
    if (!pDraw || !pDuration || !pPresent || !pDrawLayers || !pOutputPalette || !pOutputTopRows || !sceneState || !pScriptGlobals || !MG1_Gamefuncs::pfnUploadTexture || !g_D3D11Hooks.d3dDeviceContext)
    {
        spdlog::error("MG1: Opening : Missing animation or drawing addresses. Skipping.");
    }
    else
    {
        void** ppTable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
        s_hSetResources = safetyhook::create_inline(ppTable[8], SetResources_hook);
        LOG_HOOK(s_hSetResources, "MG1: Opening : ID3D11DeviceContext::PSSetShaderResources()")
        s_hUpload = safetyhook::create_inline(MG1_Gamefuncs::pfnUploadTexture, UploadTexture_hook);
        LOG_HOOK(s_hUpload, "MG1: Opening : upload texture")
        s_hDraw = safetyhook::create_mid(pDraw, DrawSprite);
        LOG_HOOK(s_hDraw, "MG1: Opening : DrawSprite()+0x36A")
        s_hDuration = safetyhook::create_mid(pDuration, SetDuration);
        LOG_HOOK(s_hDuration, "MG1: Opening : DrawSprite()+0x4CD")
        s_hPresent = safetyhook::create_mid(pPresent, PrepareUpload);
        LOG_HOOK(s_hPresent, "MG1: Opening : PresentIndexedFrame()+0x95")
        if (!s_hSetResources || !s_hUpload || !s_hDraw || !s_hDuration || !s_hPresent)
        {
            s_hPresent.reset();
            s_hDuration.reset();
            s_hDraw.reset();
            s_hUpload.reset();
            s_hSetResources.reset();
            spdlog::error("MG1: Opening : Incomplete hook setup. Disabled opening restoration.");
            return;
        }
        s_bInitialized = true;
        spdlog::info("MG1: Opening : Initialized.");
    }
}
