#include "stdafx.h"



#include "common.hpp"
#include "d3d11_api.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "mgs2_status_flags.hpp"
#include "mgs2_underwater_filter.hpp"
using namespace MGS2_StatusFlags;

namespace
{
    SafetyHookInline NewScrWater_hook {};
    SafetyHookInline ScrWaterAct_hook {};
    SafetyHookInline DmapackCreate_hook {};
    SafetyHookInline DmapackSetCallback_hook {};
    SafetyHookInline DmapackQueue_hook {};
    SafetyHookInline OMSetRenderTargets_hook {};
    SafetyHookInline PSSetShaderResources_hook {};
    SafetyHookInline Draw_hook {};
    std::array<SafetyHookMid, 8> ScrWaterDmapackCreateHooks {};
    int ScrWaterDmapackCreateHookCount = 0;
    std::mutex gHookMutex;

    constexpr int kDmapackNormal = 0x0001;
    constexpr int kDmapackMenu = 0x0002;
    constexpr int kDmapackInvisible0 = 0x0010;
    constexpr int kDmapackInvisible1 = 0x0020;
    constexpr int kDmapackInvisible2 = 0x0040;
    constexpr int kDmapackInvisible3 = 0x0080;
    constexpr int kDmapackInvisibleMenu = 0x0100;
    constexpr int kDmapackInvisibleAll = 0x01F0;
    constexpr int kDmapackPhaseAfter = 0x04;
    constexpr int kScrWaterPriority = 120;
    constexpr int kMenuPrimTrailPriority = kScrWaterPriority - 1;

    constexpr uint8_t kCmdAlpha = 20;
    constexpr uint8_t kCmdUseFrameTex = 26;
    constexpr uint8_t kCmdBackupFrame = 22;
    constexpr uint8_t kCmdEnd = 30;

    constexpr ptrdiff_t kActorActOffset = 0x08;
    constexpr ptrdiff_t kScrWaterDmapackOffset = 0x1938;
    constexpr ptrdiff_t kScrWaterDmapackOffsetAlt1 = 0x1948;
    constexpr ptrdiff_t kScrWaterDmapackOffsetAlt2 = 0x1950;
    constexpr ptrdiff_t kScrWaterStepOffsetFromDmapack = sizeof(void*) + sizeof(float) + sizeof(int16_t);
    constexpr ptrdiff_t kRuntimeDmapackPacketOffset = 0x38;
    constexpr ptrdiff_t kWorkPacketMemOffsetFromDmapack = 0x38;
    constexpr ptrdiff_t kDmapackBuildParamOffset = 0x18;
    constexpr ptrdiff_t kDmapackBuildCallbackOffset = 0x20;
    constexpr ptrdiff_t kDmapackAutopacketOffset = 0x38;
    constexpr ptrdiff_t kDmapackAutopacketSizeOffset = 0x40;
    constexpr ptrdiff_t kDmapackFileOffset = 0x48;
    constexpr ptrdiff_t kMenu2PrimFlagOffset = 0x60;
    constexpr ptrdiff_t kMenu2PrimNPrimsOffset = 0x64;

    constexpr int kPrim2TypeMask = 0x0000000F;
    constexpr int kPrim2Poly = 0x00000002;
    constexpr int kPrim2Shade = 0x00010000;
    constexpr int kPrim2Alpha = 0x00080000;

    struct RuntimeDmapack
    {
        int flag;
        int16_t phase;
        int16_t priority;
        void* packet[2];
        void* buildAutoPacketCallbackParam;
        void* buildAutoPacketCallback;
        void* BP_callbackParam;
        void* BP_renderCallback;
        void* autopacket;
        int autopacketSize;
        const char* file;
        uint32_t labelMask;
        int pad[2];
    };

    struct LateHudDmapack
    {
        RuntimeDmapack dmapack {};
        RuntimeDmapack* source = nullptr;
        void* callback = nullptr;
        void* param = nullptr;
        int chanl = -1;
        bool queued = false;
    };

    struct MenuPrimMirror
    {
        RuntimeDmapack* dmapack = nullptr;
        RuntimeDmapack* source = nullptr;
        bool queued = false;
    };

    struct KiraCallbackParam
    {
        void* dmapack;
        void* work;
        void* param0;
        int chanl;
    };

    std::array<LateHudDmapack, 5> gLateHudDmapacks {};
    std::array<MenuPrimMirror, 64> gMenuPrimMirrors {};
    int gLateHudSlot = 0;
    bool gLateHudMirrorGuard = false;
    bool gMenuPrimSourceGuard = false;
    ComPtr<ID3D11Resource> gD3D11CurrentRt0Resource;
    ComPtr<ID3D11Resource> gD3D11CurrentPs0Resource;
    ComPtr<ID3D11ShaderResourceView> gD3D11CurrentPs0View;
    void* gD3D11CurrentRt0 = nullptr;
    void* gD3D11CurrentPs0 = nullptr;
    ComPtr<ID3D11Texture2D> gWaterPreviousFrameTexture;
    ComPtr<ID3D11Texture2D> gWaterTempFrameTexture;
    ComPtr<ID3D11Texture2D> gOwnedWaterFeedbackTexture;
    ComPtr<ID3D11ShaderResourceView> gOwnedWaterFeedbackSrv;
    D3D11_TEXTURE2D_DESC gOwnedWaterFeedbackDesc {};
    bool gOwnedWaterFeedbackHasFrame = false;
    bool gWaterFeedbackSourceSeen = false;
    bool gWaterFeedbackSubstitutedThisFrame = false;
    bool gScrWaterActive = false;
    bool gScrWaterSawUnderwaterStep = false;
    void* gScrWaterWork = nullptr;
    using DmapackQueueFn = int(__fastcall*)(RuntimeDmapack*);
    DmapackQueueFn gDmapackQueue = nullptr;

    struct DmapackParam
    {
        uint8_t cmd;
        uint8_t pad[3];
        int param;
    };

    struct DmapackAlpha
    {
        uint8_t cmd;
        uint8_t pad[7];
        uint32_t alpha0;
        uint32_t alpha1;
    };

    struct WorkInfo
    {
        RuntimeDmapack* dmapack = nullptr;
        void* packetMem = nullptr;
        ptrdiff_t dmapackFieldOffset = -1;
    };

    uint8_t PacketCommand(void* packetMem);
    WorkInfo FindWorkInfo(void* work);
    void TryHookDmapackCreate(void* target);
    void TryHookKiraDmapackLifecycle(void* returnAddress);
    LateHudDmapack* AcquireLateHudDmapack(RuntimeDmapack* dmapack);
    void RegisterMenuPrimSource(RuntimeDmapack* dmapack);
    void RegisterMenuPrimCandidate(RuntimeDmapack* dmapack, void* callback, void* param);
    void RegisterMenuAutopacketCandidate(RuntimeDmapack* dmapack);
    int CallQueueDmapack(RuntimeDmapack* dmapack);
    void UpdateLateHudVisibility();
    void UpdateMenuPrimMirrors(RuntimeDmapack* waterDmapack);
    void RememberWaterFeedbackSource(UINT vertexCount);
    bool TryBindOwnedWaterFeedback(ID3D11DeviceContext* context, UINT vertexCount, ComPtr<ID3D11ShaderResourceView>& originalPs0View);
    void RestorePixelShaderSlot0(ID3D11DeviceContext* context, ID3D11ShaderResourceView* view);
    bool EnsureOwnedWaterFeedbackTexture(const D3D11_TEXTURE2D_DESC& sourceDesc);
    bool IsWaterVisible();
    bool IsScrWaterActive();
    int ScrWaterStep(void* work, ptrdiff_t dmapackFieldOffset);
    bool LooksLikeDmapackPacket(void* packetMem);
    bool IsCodecMenuOpen();
    bool IsPlayerInWater();
    bool IsUnderwaterCodecSequence();
    bool IsDemo();
    bool UseCurrentFrameBackup();

    using Memory::IsReadable;
    using Memory::ReadableBytes;

    bool LooksLikeScrWaterDmapack(const RuntimeDmapack* dmapack)
    {
        if (!IsReadable(dmapack, sizeof(RuntimeDmapack)))
        {
            return false;
        }

        if ((dmapack->flag & kDmapackNormal) == 0 || dmapack->phase != kDmapackPhaseAfter)
        {
            return false;
        }

        if (dmapack->priority < 0 || dmapack->priority > 255)
        {
            return false;
        }

        return true;
    }

    template<size_t N>
    bool ContainsBytes(const uint8_t* begin, const uint8_t* end, const uint8_t (&pattern)[N])
    {
        if (!begin || !end || begin >= end || N == 0)
        {
            return false;
        }

        for (const uint8_t* cursor = begin; cursor + N <= end; ++cursor)
        {
            if (std::memcmp(cursor, pattern, N) == 0)
            {
                return true;
            }
        }

        return false;
    }

    void* ReadWorkPointer(uint8_t* bytes, ptrdiff_t offset)
    {
        if (!IsReadable(bytes + offset, sizeof(void*)))
        {
            return nullptr;
        }

        return *reinterpret_cast<void**>(bytes + offset);
    }

    uint8_t PacketCommand(void* packetMem)
    {
        if (!IsReadable(packetMem, sizeof(DmapackParam)))
        {
            return 0xFF;
        }

        return *static_cast<uint8_t*>(packetMem);
    }

    void* ReadDmapackPointer(RuntimeDmapack* dmapack, ptrdiff_t offset)
    {
        if (!IsReadable(reinterpret_cast<uint8_t*>(dmapack) + offset, sizeof(void*)))
        {
            return nullptr;
        }

        return *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(dmapack) + offset);
    }

    int ReadDmapackInt(RuntimeDmapack* dmapack, ptrdiff_t offset)
    {
        if (!IsReadable(reinterpret_cast<uint8_t*>(dmapack) + offset, sizeof(int)))
        {
            return 0;
        }

        return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(dmapack) + offset);
    }

    bool IsStaticGameAddress(const void* ptr)
    {
        if (!ptr)
        {
            return false;
        }

        MEMORY_BASIC_INFORMATION mbi {};
        return VirtualQuery(ptr, &mbi, sizeof(mbi)) &&
            mbi.AllocationBase == baseModule &&
            mbi.State == MEM_COMMIT &&
            !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
    }

    int CallQueueDmapack(RuntimeDmapack* dmapack)
    {
        if (!dmapack)
        {
            return -1;
        }

        if (DmapackQueue_hook)
        {
            return DmapackQueue_hook.fastcall<int>(dmapack);
        }

        if (gDmapackQueue)
        {
            return gDmapackQueue(dmapack);
        }

        return -1;
    }

    bool IsOwnedMenuMirror(RuntimeDmapack* dmapack)
    {
        if (!dmapack)
        {
            return false;
        }

        for (const MenuPrimMirror& mirror : gMenuPrimMirrors)
        {
            if (mirror.dmapack == dmapack)
            {
                return true;
            }
        }

        for (const LateHudDmapack& late : gLateHudDmapacks)
        {
            if (&late.dmapack == dmapack)
            {
                return true;
            }
        }

        return false;
    }

    bool IsMenu2PrimGaugeDmapack(RuntimeDmapack* dmapack)
    {
        if (!IsReadable(dmapack, kMenu2PrimNPrimsOffset + sizeof(int)) ||
            (dmapack->flag & kDmapackMenu) == 0 ||
            dmapack->phase != kDmapackPhaseAfter ||
            dmapack->priority <= kScrWaterPriority)
        {
            return false;
        }

        void* callback = ReadDmapackPointer(dmapack, kDmapackBuildCallbackOffset);
        void* param = ReadDmapackPointer(dmapack, kDmapackBuildParamOffset);
        if (param != dmapack || !Memory::IsExecutable(callback))
        {
            return false;
        }

        const int primFlag = ReadDmapackInt(dmapack, kMenu2PrimFlagOffset);
        const int nPrims = ReadDmapackInt(dmapack, kMenu2PrimNPrimsOffset);
        if ((primFlag & kPrim2TypeMask) != kPrim2Poly ||
            (primFlag & (kPrim2Shade | kPrim2Alpha)) != (kPrim2Shade | kPrim2Alpha))
        {
            return false;
        }

        return nPrims == 8 || nPrims == 11;
    }

    bool IsMenuTextDmapack(RuntimeDmapack* dmapack)
    {
        if (!IsReadable(dmapack, sizeof(RuntimeDmapack)) ||
            !IsStaticGameAddress(dmapack) ||
            (dmapack->flag & kDmapackMenu) == 0 ||
            dmapack->phase != kDmapackPhaseAfter ||
            dmapack->priority <= kScrWaterPriority)
        {
            return false;
        }

        if (ReadDmapackPointer(dmapack, kDmapackBuildCallbackOffset) != nullptr)
        {
            return false;
        }

        return true;
    }

    bool IsMenuAutopacketTrailDmapack(RuntimeDmapack* dmapack)
    {
        if (!IsReadable(dmapack, sizeof(RuntimeDmapack)) ||
            IsOwnedMenuMirror(dmapack) ||
            (dmapack->flag & kDmapackMenu) == 0 ||
            dmapack->phase != kDmapackPhaseAfter ||
            dmapack->priority <= kScrWaterPriority ||
            ReadDmapackPointer(dmapack, kDmapackBuildCallbackOffset) != nullptr)
        {
            return false;
        }

        void* autopacket = ReadDmapackPointer(dmapack, kDmapackAutopacketOffset);
        const int autopacketSize = ReadDmapackInt(dmapack, kDmapackAutopacketSizeOffset);
        if (!autopacket || autopacketSize < 0 || autopacketSize > 0x20000)
        {
            return false;
        }

        return LooksLikeDmapackPacket(autopacket);
    }

    bool IsEarlyMenuMirrorSource(RuntimeDmapack* dmapack)
    {
        return IsMenu2PrimGaugeDmapack(dmapack) || IsMenuAutopacketTrailDmapack(dmapack) || IsMenuTextDmapack(dmapack);
    }

    bool IsMenuMirrorCandidate(RuntimeDmapack* dmapack)
    {
        return IsEarlyMenuMirrorSource(dmapack) && !IsOwnedMenuMirror(dmapack);
    }

    bool IsMenuPrimCallbackCandidate(RuntimeDmapack* dmapack, void* callback, void* param)
    {
        if (!IsReadable(dmapack, sizeof(RuntimeDmapack)) ||
            param != dmapack ||
            !Memory::IsExecutable(callback) ||
            (dmapack->flag & kDmapackMenu) == 0 ||
            dmapack->phase != kDmapackPhaseAfter ||
            dmapack->priority <= kScrWaterPriority)
        {
            return false;
        }

        return !IsOwnedMenuMirror(dmapack);
    }

    void CaptureD3D11View(ID3D11View* view, ComPtr<ID3D11Resource>& resource, void*& resourcePtr)
    {
        resource.Reset();
        resourcePtr = nullptr;

        if (!view)
        {
            return;
        }

        view->GetResource(resource.GetAddressOf());
        resourcePtr = resource.Get();
    }

    bool GetTextureDesc(ID3D11Resource* resource, D3D11_TEXTURE2D_DESC& desc, ComPtr<ID3D11Texture2D>& texture)
    {
        texture.Reset();
        if (!resource)
        {
            return false;
        }

        if (FAILED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(texture.GetAddressOf()))) || !texture)
        {
            return false;
        }

        texture->GetDesc(&desc);
        return true;
    }

    void RememberWaterFeedbackSource(UINT vertexCount)
    {
        if (vertexCount != 4 ||
            !IsScrWaterActive() ||
            IsDemo() ||
            !gD3D11CurrentRt0 ||
            !gD3D11CurrentPs0 ||
            gD3D11CurrentRt0 == gD3D11CurrentPs0)
        {
            return;
        }

        D3D11_TEXTURE2D_DESC rtDesc {};
        D3D11_TEXTURE2D_DESC psDesc {};
        ComPtr<ID3D11Texture2D> rtTexture;
        ComPtr<ID3D11Texture2D> psTexture;
        if (!GetTextureDesc(static_cast<ID3D11Resource*>(gD3D11CurrentRt0), rtDesc, rtTexture) ||
            !GetTextureDesc(static_cast<ID3D11Resource*>(gD3D11CurrentPs0), psDesc, psTexture))
        {
            return;
        }

        const bool fullSizeCopy =
            rtDesc.Width >= 1280 &&
            rtDesc.Height >= 720 &&
            rtDesc.Width == psDesc.Width &&
            rtDesc.Height == psDesc.Height &&
            rtDesc.Format == psDesc.Format &&
            rtDesc.SampleDesc.Count == 1 &&
            psDesc.SampleDesc.Count == 1 &&
            (rtDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0 &&
            (rtDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0 &&
            (psDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0 &&
            (psDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0;

        if (!fullSizeCopy)
        {
            return;
        }

        if (gWaterPreviousFrameTexture.Get() != psTexture.Get() || gWaterTempFrameTexture.Get() != rtTexture.Get())
        {
            gWaterPreviousFrameTexture = psTexture;
            gWaterTempFrameTexture = rtTexture;
        }

        gWaterFeedbackSourceSeen = true;
    }

    bool EnsureOwnedWaterFeedbackTexture(const D3D11_TEXTURE2D_DESC& sourceDesc)
    {
        if (!g_D3D11Hooks.d3dDevice ||
            sourceDesc.Width == 0 ||
            sourceDesc.Height == 0 ||
            sourceDesc.SampleDesc.Count != 1)
        {
            return false;
        }

        const bool matches =
            gOwnedWaterFeedbackTexture &&
            gOwnedWaterFeedbackSrv &&
            gOwnedWaterFeedbackDesc.Width == sourceDesc.Width &&
            gOwnedWaterFeedbackDesc.Height == sourceDesc.Height &&
            gOwnedWaterFeedbackDesc.Format == sourceDesc.Format &&
            gOwnedWaterFeedbackDesc.SampleDesc.Count == sourceDesc.SampleDesc.Count;

        if (matches)
        {
            return true;
        }

        gOwnedWaterFeedbackTexture.Reset();
        gOwnedWaterFeedbackSrv.Reset();
        gOwnedWaterFeedbackHasFrame = false;

        D3D11_TEXTURE2D_DESC desc = sourceDesc;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT hr = g_D3D11Hooks.d3dDevice->CreateTexture2D(&desc, nullptr, gOwnedWaterFeedbackTexture.GetAddressOf());
        if (FAILED(hr) || !gOwnedWaterFeedbackTexture)
        {
            spdlog::warn("MGS 2: Underwater Filter Fix: failed to create owned water feedback texture, HRESULT=0x{:08X}.", static_cast<unsigned>(hr));
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = g_D3D11Hooks.d3dDevice->CreateShaderResourceView(gOwnedWaterFeedbackTexture.Get(), &srvDesc, gOwnedWaterFeedbackSrv.GetAddressOf());
        if (FAILED(hr) || !gOwnedWaterFeedbackSrv)
        {
            gOwnedWaterFeedbackTexture.Reset();
            spdlog::warn("MGS 2: Underwater Filter Fix: failed to create owned water feedback SRV, HRESULT=0x{:08X}.", static_cast<unsigned>(hr));
            return false;
        }

        gOwnedWaterFeedbackDesc = desc;
        return true;
    }

    void RestorePixelShaderSlot0(ID3D11DeviceContext* context, ID3D11ShaderResourceView* view)
    {
        ID3D11ShaderResourceView* restoreView[1] = { view };
        if (PSSetShaderResources_hook)
        {
            PSSetShaderResources_hook.call<void>(context, 0, 1, restoreView);
        }
        else
        {
            context->PSSetShaderResources(0, 1, restoreView);
        }
        gD3D11CurrentPs0View = view;
        CaptureD3D11View(
            view ? static_cast<ID3D11View*>(view) : nullptr,
            gD3D11CurrentPs0Resource,
            gD3D11CurrentPs0);
    }

    bool TryBindOwnedWaterFeedback(ID3D11DeviceContext* context, UINT vertexCount, ComPtr<ID3D11ShaderResourceView>& originalPs0View)
    {
        if (vertexCount != 4 ||
            !context ||
            !gOwnedWaterFeedbackSrv ||
            !gOwnedWaterFeedbackHasFrame ||
            gWaterFeedbackSubstitutedThisFrame ||
            !IsScrWaterActive() ||
            IsDemo() ||
            !gWaterPreviousFrameTexture ||
            !gD3D11CurrentPs0Resource ||
            !gD3D11CurrentRt0Resource)
        {
            return false;
        }

        D3D11_TEXTURE2D_DESC rtDesc {};
        D3D11_TEXTURE2D_DESC psDesc {};
        ComPtr<ID3D11Texture2D> rtTexture;
        ComPtr<ID3D11Texture2D> psTexture;
        if (!GetTextureDesc(gD3D11CurrentRt0Resource.Get(), rtDesc, rtTexture) ||
            !GetTextureDesc(gD3D11CurrentPs0Resource.Get(), psDesc, psTexture))
        {
            return false;
        }

        const bool isWaterFeedbackDraw =
            psTexture.Get() == gWaterPreviousFrameTexture.Get() &&
            rtTexture.Get() != psTexture.Get() &&
            rtDesc.SampleDesc.Count == 1 &&
            psDesc.SampleDesc.Count == 1 &&
            (rtDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0 &&
            (rtDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0 &&
            (psDesc.BindFlags & D3D11_BIND_RENDER_TARGET) != 0 &&
            (psDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0 &&
            rtDesc.Width == gOwnedWaterFeedbackDesc.Width &&
            rtDesc.Height == gOwnedWaterFeedbackDesc.Height &&
            rtDesc.Format == gOwnedWaterFeedbackDesc.Format &&
            psDesc.Width == gOwnedWaterFeedbackDesc.Width &&
            psDesc.Height == gOwnedWaterFeedbackDesc.Height &&
            psDesc.Format == gOwnedWaterFeedbackDesc.Format;

        if (!isWaterFeedbackDraw)
        {
            return false;
        }

        originalPs0View = gD3D11CurrentPs0View;
        ID3D11ShaderResourceView* substituteView[1] = { gOwnedWaterFeedbackSrv.Get() };
        PSSetShaderResources_hook.call<void>(context, 0, 1, substituteView);
        gWaterFeedbackSubstitutedThisFrame = true;

        gD3D11CurrentPs0View = gOwnedWaterFeedbackSrv;
        gD3D11CurrentPs0Resource = gOwnedWaterFeedbackTexture;
        gD3D11CurrentPs0 = gOwnedWaterFeedbackTexture.Get();

        return true;
    }

    void __stdcall OMSetRenderTargets_Hook(
        ID3D11DeviceContext* context,
        UINT numViews,
        ID3D11RenderTargetView* const* renderTargetViews,
        ID3D11DepthStencilView* depthStencilView)
    {
        if (IsScrWaterActive())
        {
            CaptureD3D11View(
                numViews && renderTargetViews ? static_cast<ID3D11View*>(renderTargetViews[0]) : nullptr,
                gD3D11CurrentRt0Resource,
                gD3D11CurrentRt0);
        }

        OMSetRenderTargets_hook.call<void>(context, numViews, renderTargetViews, depthStencilView);
    }

    void __stdcall PSSetShaderResources_Hook(
        ID3D11DeviceContext* context,
        UINT startSlot,
        UINT numViews,
        ID3D11ShaderResourceView* const* shaderResourceViews)
    {
        if (IsScrWaterActive() && startSlot == 0 && numViews > 0)
        {
            ID3D11ShaderResourceView* srv = shaderResourceViews ? shaderResourceViews[0] : nullptr;
            gD3D11CurrentPs0View = srv;
            CaptureD3D11View(
                srv ? static_cast<ID3D11View*>(srv) : nullptr,
                gD3D11CurrentPs0Resource,
                gD3D11CurrentPs0);
        }

        PSSetShaderResources_hook.call<void>(context, startSlot, numViews, shaderResourceViews);
    }

    void __stdcall Draw_Hook(ID3D11DeviceContext* context, UINT vertexCount, UINT startVertexLocation)
    {
        RememberWaterFeedbackSource(vertexCount);
        ComPtr<ID3D11ShaderResourceView> originalPs0View;
        const bool substituted = TryBindOwnedWaterFeedback(context, vertexCount, originalPs0View);
        Draw_hook.call<void>(context, vertexCount, startVertexLocation);
        if (substituted)
        {
            RestorePixelShaderSlot0(context, originalPs0View.Get());
        }
    }

    

    void SyncMenuPrimMirror(MenuPrimMirror& mirror)
    {
        RuntimeDmapack* source = mirror.source;
        RuntimeDmapack* dmapack = mirror.dmapack;
        if (!dmapack)
        {
            return;
        }

        const bool isGauge = IsMenu2PrimGaugeDmapack(source);
        const bool isMenuAutopacket = IsMenuAutopacketTrailDmapack(source);
        const bool isText = IsMenuTextDmapack(source);
        if ((!isGauge && !isMenuAutopacket && !isText) ||
            !IsScrWaterActive() ||
            IsDemo())
        {
            dmapack->flag |= kDmapackInvisibleAll;
            return;
        }

        void* autopacket = ReadDmapackPointer(source, kDmapackAutopacketOffset);
        if (isText && !autopacket)
        {
            dmapack->flag |= kDmapackInvisibleAll;
            return;
        }

        dmapack->flag = source->flag;
        dmapack->phase = kDmapackPhaseAfter;
        dmapack->priority = kMenuPrimTrailPriority;
        dmapack->packet[0] = source->packet[0];
        dmapack->packet[1] = source->packet[1];
        dmapack->buildAutoPacketCallbackParam = ReadDmapackPointer(source, kDmapackBuildParamOffset);
        dmapack->buildAutoPacketCallback = ReadDmapackPointer(source, kDmapackBuildCallbackOffset);
        dmapack->BP_callbackParam = nullptr;
        dmapack->BP_renderCallback = nullptr;
        dmapack->autopacket = autopacket;
        dmapack->autopacketSize = ReadDmapackInt(source, kDmapackAutopacketSizeOffset);
        dmapack->file = "mgs2_underwater_filter_menuprim";
        dmapack->labelMask = 0;
        dmapack->pad[0] = 0;
        dmapack->pad[1] = 0;
    }

    MenuPrimMirror* FindMenuPrimMirror(RuntimeDmapack* source)
    {
        for (MenuPrimMirror& mirror : gMenuPrimMirrors)
        {
            if (mirror.source == source)
            {
                return &mirror;
            }
        }

        return nullptr;
    }

    MenuPrimMirror* AcquireMenuPrimMirror(RuntimeDmapack* source)
    {
        if (MenuPrimMirror* mirror = FindMenuPrimMirror(source))
        {
            return mirror;
        }

        for (MenuPrimMirror& mirror : gMenuPrimMirrors)
        {
            if (!mirror.source || !IsReadable(mirror.source, sizeof(RuntimeDmapack)))
            {
                mirror.source = source;
                if (mirror.dmapack)
                {
                    mirror.dmapack->flag |= kDmapackInvisibleAll;
                }
                return &mirror;
            }
        }

        return nullptr;
    }

    void RegisterMenuPrimSource(RuntimeDmapack* dmapack)
    {
        if (gMenuPrimSourceGuard || !IsMenuMirrorCandidate(dmapack))
        {
            return;
        }

        if (FindMenuPrimMirror(dmapack))
        {
            return;
        }

        MenuPrimMirror* mirror = AcquireMenuPrimMirror(dmapack);
        if (!mirror)
        {
            return;
        }

    }

    void RegisterMenuPrimCandidate(RuntimeDmapack* dmapack, void* callback, void* param)
    {
        if (gMenuPrimSourceGuard || !IsMenuPrimCallbackCandidate(dmapack, callback, param))
        {
            return;
        }

        if (FindMenuPrimMirror(dmapack))
        {
            return;
        }

        MenuPrimMirror* mirror = AcquireMenuPrimMirror(dmapack);
        if (!mirror)
        {
            return;
        }

    }

    void RegisterMenuAutopacketCandidate(RuntimeDmapack* dmapack)
    {
        if (gMenuPrimSourceGuard || !IsMenuAutopacketTrailDmapack(dmapack))
        {
            return;
        }

        if (FindMenuPrimMirror(dmapack))
        {
            return;
        }

        MenuPrimMirror* mirror = AcquireMenuPrimMirror(dmapack);
        if (!mirror)
        {
            return;
        }

    }

    bool EnsureMenuPrimMirror(MenuPrimMirror& mirror)
    {
        if (mirror.dmapack)
        {
            return true;
        }

        if (!DmapackCreate_hook)
        {
            return false;
        }

        gLateHudMirrorGuard = true;
        mirror.dmapack = DmapackCreate_hook.fastcall<RuntimeDmapack*>(
            kDmapackMenu | kDmapackInvisibleAll,
            kDmapackPhaseAfter,
            kMenuPrimTrailPriority,
            "mgs2_underwater_filter_menuprim");
        gLateHudMirrorGuard = false;

        if (!mirror.dmapack)
        {
            return false;
        }

        SyncMenuPrimMirror(mirror);
        return true;
    }

    bool QueueMenuPrimMirror(MenuPrimMirror& mirror)
    {
        if (mirror.queued || !EnsureMenuPrimMirror(mirror))
        {
            return mirror.queued;
        }

        gLateHudMirrorGuard = true;
        const int result = CallQueueDmapack(mirror.dmapack);
        gLateHudMirrorGuard = false;
        if (result < 0)
        {
            return false;
        }

        mirror.queued = true;
        return true;
    }

    void UpdateMenuPrimMirrors(RuntimeDmapack* waterDmapack)
    {
        (void)waterDmapack;

        for (MenuPrimMirror& mirror : gMenuPrimMirrors)
        {
            if (!mirror.source)
            {
                continue;
            }

            if (!IsReadable(mirror.source, sizeof(RuntimeDmapack)))
            {
                if (mirror.dmapack)
                {
                    mirror.dmapack->flag |= kDmapackInvisibleAll;
                }
                continue;
            }

            if (!IsEarlyMenuMirrorSource(mirror.source))
            {
                if (mirror.dmapack)
                {
                    mirror.dmapack->flag |= kDmapackInvisibleAll;
                }
                continue;
            }

            if (!EnsureMenuPrimMirror(mirror))
            {
                continue;
            }

            SyncMenuPrimMirror(mirror);
            QueueMenuPrimMirror(mirror);
        }
    }

    bool LooksLikeSetDmapackCallback(void* target)
    {
        if (!IsReadable(target, 0x40))
        {
            return false;
        }

        const auto* bytes = static_cast<const uint8_t*>(target);
        bool writesCallback = false;
        bool writesParam = false;
        for (int i = 0; i < 0x3c; ++i)
        {
            if (bytes[i] == 0x48 && bytes[i + 1] == 0x89 && bytes[i + 2] == 0x51 && bytes[i + 3] == 0x20)
            {
                writesCallback = true;
            }
            if (bytes[i] == 0x4c && bytes[i + 1] == 0x89 && bytes[i + 2] == 0x41 && bytes[i + 3] == 0x18)
            {
                writesParam = true;
            }
        }

        return writesCallback && writesParam;
    }

    int VisibleLateHudFlag(const LateHudDmapack& late)
    {
        (void)late;
        return kDmapackNormal | kDmapackInvisible1 | kDmapackInvisible2 | kDmapackInvisible3;
    }

    bool IsLateHudMirrorChannel(const LateHudDmapack& late)
    {
        return late.chanl >= 0 && late.chanl <= 4;
    }

    bool IsLateHudDmapackCandidate(RuntimeDmapack* dmapack, void* callback, const KiraCallbackParam* param, int chanl)
    {
        return IsReadable(dmapack, sizeof(RuntimeDmapack)) &&
            IsReadable(param, sizeof(KiraCallbackParam)) &&
            param->dmapack == dmapack &&
            chanl >= 0 &&
            chanl <= 4 &&
            Memory::IsExecutable(callback) &&
            dmapack->phase == kDmapackPhaseAfter &&
            (dmapack->flag & (kDmapackNormal | kDmapackMenu)) != 0 &&
            !IsOwnedMenuMirror(dmapack);
    }

    void SyncLateHudDmapack(LateHudDmapack& late)
    {
        RuntimeDmapack* target = &late.dmapack;
        if (!late.source || !IsReadable(late.source, 0x50))
        {
            target->flag |= kDmapackInvisibleAll;
            return;
        }

        void* buildParam = late.param ? late.param : ReadDmapackPointer(late.source, kDmapackBuildParamOffset);
        void* buildCallback = late.callback ? late.callback : ReadDmapackPointer(late.source, kDmapackBuildCallbackOffset);
        const bool ready = buildParam && buildCallback;
        const bool visible = ready && IsLateHudMirrorChannel(late) && IsScrWaterActive() && !IsDemo();
        target->flag = visible ? VisibleLateHudFlag(late) : (late.source->flag | kDmapackInvisibleAll);
        target->phase = kDmapackPhaseAfter;
        target->priority = kMenuPrimTrailPriority;
        target->packet[0] = late.source->packet[0];
        target->packet[1] = late.source->packet[1];
        target->buildAutoPacketCallbackParam = buildParam;
        target->buildAutoPacketCallback = buildCallback;
        target->BP_callbackParam = nullptr;
        target->BP_renderCallback = nullptr;
        target->autopacket = nullptr;
        target->autopacketSize = 0;
        target->file = static_cast<const char*>(ReadDmapackPointer(late.source, kDmapackFileOffset));
        target->labelMask = 0;
        target->pad[0] = 0;
        target->pad[1] = 0;
    }

    LateHudDmapack* FindLateHudBySource(RuntimeDmapack* source)
    {
        for (LateHudDmapack& late : gLateHudDmapacks)
        {
            if (late.source == source)
            {
                return &late;
            }
        }

        return nullptr;
    }

    LateHudDmapack* AcquireLateHudDmapack(RuntimeDmapack* source)
    {
        if (LateHudDmapack* late = FindLateHudBySource(source))
        {
            return late;
        }

        LateHudDmapack& late = gLateHudDmapacks[gLateHudSlot++ % static_cast<int>(gLateHudDmapacks.size())];
        late.source = source;
        late.callback = nullptr;
        late.param = nullptr;
        late.chanl = -1;
        late.queued = false;
        return &late;
    }

    void UpdateLateHudVisibility()
    {
        for (LateHudDmapack& late : gLateHudDmapacks)
        {
            if (late.source)
            {
                SyncLateHudDmapack(late);
            }
        }
    }

    void __fastcall DmapackSetCallback_Hook(RuntimeDmapack* dmapack, void* callback, void* param)
    {
        DmapackSetCallback_hook.fastcall<void>(dmapack, callback, param);
        RegisterMenuPrimCandidate(dmapack, callback, param);
        RegisterMenuPrimSource(dmapack);

        if (gLateHudMirrorGuard)
        {
            return;
        }

        const auto* kiraParam = static_cast<const KiraCallbackParam*>(param);
        const int chanl = IsReadable(kiraParam, sizeof(KiraCallbackParam)) ? kiraParam->chanl : -1;
        LateHudDmapack* late = FindLateHudBySource(dmapack);
        if (!late && IsLateHudDmapackCandidate(dmapack, callback, kiraParam, chanl))
        {
            late = AcquireLateHudDmapack(dmapack);
        }

        if (!late || !late->source)
        {
            return;
        }

        late->callback = callback;
        late->param = param;
        late->chanl = chanl;

        SyncLateHudDmapack(*late);
    }

    int __fastcall DmapackQueue_Hook(RuntimeDmapack* dmapack)
    {
        const int result = DmapackQueue_hook.fastcall<int>(dmapack);

        if (gLateHudMirrorGuard)
        {
            return result;
        }

        RegisterMenuPrimSource(dmapack);
        RegisterMenuAutopacketCandidate(dmapack);
        return result;
    }

    void TryHookKiraDmapackLifecycle(void* returnAddress)
    {
        if (!returnAddress || DmapackSetCallback_hook)
        {
            return;
        }

        auto* bytes = static_cast<uint8_t*>(returnAddress);
        if (!IsReadable(bytes, 0x300))
        {
            return;
        }

        void* setCallbackTarget = nullptr;
        void* queueTarget = nullptr;

        for (size_t offset = 0; offset + 5 <= 0x300; ++offset)
        {
            if (bytes[offset] != 0xE8)
            {
                continue;
            }

            void* target = reinterpret_cast<void*>(Memory::GetRipRelativeAddress(bytes + offset, 1, 5));
            if (!setCallbackTarget)
            {
                if (LooksLikeSetDmapackCallback(target))
                {
                    setCallbackTarget = target;
                }
                continue;
            }

            if (!queueTarget && target != setCallbackTarget)
            {
                queueTarget = target;
                break;
            }
        }

        std::lock_guard lock(gHookMutex);
        if (setCallbackTarget && !DmapackSetCallback_hook)
        {
            DmapackSetCallback_hook = safetyhook::create_inline(setCallbackTarget, reinterpret_cast<void*>(DmapackSetCallback_Hook));
            LOG_HOOK(DmapackSetCallback_hook, "MGS 2: Underwater Filter Fix: DG_SetDmapackCallback");
            spdlog::info("MGS 2: Underwater Filter Fix: DG_SetDmapackCallback hook target {}.", fmt::ptr(setCallbackTarget));
        }

        if (queueTarget && !gDmapackQueue)
        {
            gDmapackQueue = reinterpret_cast<DmapackQueueFn>(queueTarget);
            spdlog::info("MGS 2: Underwater Filter Fix: DG_QueueDmapack target {}.", fmt::ptr(queueTarget));
        }

        if (queueTarget && !DmapackQueue_hook)
        {
            DmapackQueue_hook = safetyhook::create_inline(queueTarget, reinterpret_cast<void*>(DmapackQueue_Hook));
            LOG_HOOK(DmapackQueue_hook, "MGS 2: Underwater Filter Fix: DG_QueueDmapack");
        }

    }

    bool LooksLikeDmapackPacket(void* packetMem)
    {
        const uint8_t cmd = PacketCommand(packetMem);
        return cmd == kCmdBackupFrame || cmd == kCmdEnd || cmd == kCmdUseFrameTex || cmd == kCmdAlpha;
    }

    void* GetDmapackPacket(RuntimeDmapack* dmapack)
    {
        if (!dmapack)
        {
            return nullptr;
        }

        void* runtimePacket = nullptr;
        runtimePacket = ReadDmapackPointer(dmapack, kRuntimeDmapackPacketOffset);

        if (LooksLikeDmapackPacket(runtimePacket))
        {
            return runtimePacket;
        }

        if (LooksLikeDmapackPacket(ReadDmapackPointer(dmapack, kDmapackAutopacketOffset)))
        {
            return ReadDmapackPointer(dmapack, kDmapackAutopacketOffset);
        }

        return nullptr;
    }

    void* FindPacketMem(uint8_t* bytes, ptrdiff_t dmapackOffset)
    {
        void* packetMem = ReadWorkPointer(bytes, dmapackOffset + kWorkPacketMemOffsetFromDmapack);
        if (LooksLikeDmapackPacket(packetMem))
        {
            return packetMem;
        }

        for (ptrdiff_t delta = 0x20; delta <= 0x180; delta += sizeof(void*))
        {
            void* candidate = ReadWorkPointer(bytes, dmapackOffset + delta);
            if (PacketCommand(candidate) == kCmdBackupFrame)
            {
                return candidate;
            }
        }

        for (ptrdiff_t delta = 0x20; delta <= 0x180; delta += sizeof(void*))
        {
            void* candidate = ReadWorkPointer(bytes, dmapackOffset + delta);
            if (PacketCommand(candidate) == kCmdEnd)
            {
                return candidate;
            }
        }

        return nullptr;
    }

    void SetWorkInfo(WorkInfo& info, uint8_t* bytes, ptrdiff_t offset, RuntimeDmapack* dmapack)
    {
        info.dmapack = dmapack;
        info.dmapackFieldOffset = offset;
        info.packetMem = ReadWorkPointer(bytes, offset + kWorkPacketMemOffsetFromDmapack);
        if (!LooksLikeDmapackPacket(info.packetMem))
        {
            info.packetMem = GetDmapackPacket(dmapack);
        }
        if (!info.packetMem)
        {
            info.packetMem = FindPacketMem(bytes, offset);
        }
    }

    WorkInfo FindWorkInfo(void* work)
    {
        WorkInfo info {};
        if (!IsReadable(work, sizeof(void*)))
        {
            return info;
        }

        auto* bytes = static_cast<uint8_t*>(work);
        for (const ptrdiff_t offset : { kScrWaterDmapackOffset, kScrWaterDmapackOffsetAlt1, kScrWaterDmapackOffsetAlt2 })
        {
            if (!IsReadable(bytes + offset, sizeof(void*)))
            {
                continue;
            }

            auto* candidate = *reinterpret_cast<RuntimeDmapack**>(bytes + offset);
            if (LooksLikeScrWaterDmapack(candidate))
            {
                SetWorkInfo(info, bytes, offset, candidate);
                return info;
            }
        }

        for (ptrdiff_t offset = 0x20; offset < 0x1A80; offset += sizeof(void*))
        {
            if (!IsReadable(bytes + offset, sizeof(void*)))
            {
                continue;
            }

            auto* candidate = *reinterpret_cast<RuntimeDmapack**>(bytes + offset);
            if (LooksLikeScrWaterDmapack(candidate))
            {
                SetWorkInfo(info, bytes, offset, candidate);
                return info;
            }
        }

        return info;
    }

    bool IsWaterVisible()
    {
        const int gameStatus = g_GameVars.Get_GM_GameStatus();
        if ((gameStatus & STATE_DISP_GAMEOVER) != 0)
        {
            return false;
        }

        return true;
    }

    int ScrWaterStep(void* work, ptrdiff_t dmapackFieldOffset)
    {
        if (!work || dmapackFieldOffset < 0)
        {
            return -1;
        }

        auto* step = reinterpret_cast<int16_t*>(static_cast<uint8_t*>(work) + dmapackFieldOffset + kScrWaterStepOffsetFromDmapack);
        if (!IsReadable(step, sizeof(*step)))
        {
            return -1;
        }

        return *step;
    }

    bool IsScrWaterActive()
    {
        return gScrWaterActive && IsWaterVisible();
    }

    bool IsCodecMenuOpen()
    {
        const int menuStatus = g_GameVars.GM_MenuStatus();
        return (menuStatus & MENU_RADIO_ON) != 0 ||
            (menuStatus & (MENU_RADAR_ON | MENU_GAGE_ON)) == (MENU_RADAR_ON | MENU_GAGE_ON);
    }

    bool IsPlayerInWater()
    {
        return (g_GameVars.Get_PL_Status() & PLAYER_IN_THE_WATER) != 0;
    }

    bool IsUnderwaterCodecSequence()
    {
        return IsPlayerInWater() &&
            IsCodecMenuOpen();
    }

    bool IsDemo()
    {
        return (g_GameVars.Get_GM_GameStatus() & STATE_PLAY_DEMO) != 0 && !IsUnderwaterCodecSequence();
    }

    bool UseCurrentFrameBackup()
    {
        return IsDemo();
    }

    void PatchTuning(void* work, ptrdiff_t dmapackFieldOffset)
    {
        auto* bytes = static_cast<uint8_t*>(work);
        auto* diffColor = reinterpret_cast<float*>(bytes + dmapackFieldOffset + 0x20);
        auto* warp = reinterpret_cast<float*>(bytes + dmapackFieldOffset + 0x30);

        if (IsReadable(diffColor, sizeof(float) * 4) &&
            std::abs(diffColor[0]) < 0.01f &&
            std::abs(diffColor[1]) < 0.01f &&
            diffColor[2] > 20.0f && diffColor[2] < 40.0f)
        {
            diffColor[2] = 4.0f;
        }

        if (IsReadable(warp, sizeof(float)) && *warp > 24.0f && *warp < 40.0f)
        {
            *warp = 80.0f;
        }
    }

    void PatchPacket(void* packetMem)
    {
        auto* packet = static_cast<uint8_t*>(packetMem);
        const size_t packetSize = std::min<size_t>(ReadableBytes(packet), 0x2000);
        if (!IsReadable(packet, packetSize))
        {
            return;
        }

        for (size_t offset = 0; offset + sizeof(DmapackParam) <= packetSize; offset += 4)
        {
            auto* backupFrame = reinterpret_cast<DmapackParam*>(packet + offset);
            if (backupFrame->cmd == kCmdEnd)
            {
                break;
            }

            if (backupFrame->cmd == kCmdBackupFrame)
            {
                backupFrame->param = UseCurrentFrameBackup() ? 0 : 1;
                break;
            }
        }

        for (size_t offset = 0; offset + sizeof(DmapackAlpha) <= packetSize; offset += 4)
        {
            auto* alpha = reinterpret_cast<DmapackAlpha*>(packet + offset);
            if (alpha->cmd == kCmdEnd)
            {
                break;
            }

            if (alpha->cmd == kCmdAlpha && alpha->alpha0 == 100 && alpha->alpha1 == 72)
            {
                alpha->alpha1 = 50;
                break;
            }
        }
    }

    RuntimeDmapack* __fastcall DmapackCreate_Hook(int flag, int phase, int priority, const char* file)
    {
        RuntimeDmapack* dmapack = DmapackCreate_hook.fastcall<RuntimeDmapack*>(flag, phase, priority, file);
        if (dmapack)
        {
            TryHookKiraDmapackLifecycle(_ReturnAddress());
            RegisterMenuPrimSource(dmapack);
            RegisterMenuAutopacketCandidate(dmapack);
        }

        return dmapack;
    }

    void TryHookDmapackCreate(void* target)
    {
        if (!target || DmapackCreate_hook)
        {
            return;
        }

        std::lock_guard lock(gHookMutex);
        if (DmapackCreate_hook)
        {
            return;
        }

        DmapackCreate_hook = safetyhook::create_inline(target, reinterpret_cast<void*>(DmapackCreate_Hook));
        LOG_HOOK(DmapackCreate_hook, "MGS 2: Underwater Filter Fix: DG_MakeDmapack2_D");
        spdlog::info("MGS 2: Underwater Filter Fix: DG_MakeDmapack2_D hook target {}.", fmt::ptr(target));
    }

    void __fastcall ScrWaterAct_Hook(void* work)
    {
        WorkInfo info = FindWorkInfo(work);
        void* packetBegin = info.packetMem;

        ScrWaterAct_hook.fastcall<void>(work);
        PatchPacket(packetBegin);
        g_MGS2UnderwaterFilterFix.PatchWork(work);
    }

    void TryHookScrWaterAct(void* work)
    {
        if (!work || ScrWaterAct_hook)
        {
            return;
        }

        std::lock_guard lock(gHookMutex);
        if (ScrWaterAct_hook)
        {
            return;
        }

        if (!IsReadable(static_cast<uint8_t*>(work) + kActorActOffset, sizeof(void*)))
        {
            spdlog::warn("MGS 2: Underwater Filter Fix: actor memory was not readable; Act hook skipped.");
            return;
        }

        void* act = *reinterpret_cast<void**>(static_cast<uint8_t*>(work) + kActorActOffset);
        if (!act)
        {
            spdlog::warn("MGS 2: Underwater Filter Fix: actor Act pointer was null; Act hook skipped.");
            return;
        }

        ScrWaterAct_hook = safetyhook::create_inline(act, reinterpret_cast<void*>(ScrWaterAct_Hook));
        LOG_HOOK(ScrWaterAct_hook, "MGS 2: Underwater Filter Fix: ScrWater Act");
        spdlog::info("MGS 2: Underwater Filter Fix: ScrWater Act hook target {}.", fmt::ptr(act));
    }

    void* __fastcall NewScrWater_Hook(int name, int map)
    {
        void* work = NewScrWater_hook.fastcall<void*>(name, map);
        TryHookScrWaterAct(work);
        g_MGS2UnderwaterFilterFix.PatchWork(work);
        return work;
    }

    void PatchScrWaterDmapackCreateArgs(SafetyHookContext& ctx)
    {
        const int flag = static_cast<int>(ctx.rcx);
        const int phase = static_cast<int>(ctx.rdx);
        const int priority = static_cast<int>(ctx.r8);
        constexpr int originalScrWaterFlag = kDmapackNormal | kDmapackInvisible0 | kDmapackInvisible1;
        if ((flag & originalScrWaterFlag) != originalScrWaterFlag ||
            phase != kDmapackPhaseAfter ||
            priority != 144)
        {
            return;
        }

        ctx.rcx = static_cast<uint64_t>(flag | kDmapackMenu | kDmapackInvisibleMenu);
        ctx.r8 = kScrWaterPriority;

        static bool logged = false;
        if (!logged)
        {
            logged = true;
            spdlog::info(
                "MGS 2: Underwater Filter Fix: ScrWater dmapack create args patched, flag 0x{:X}->0x{:X}, priority {}->{}.",
                flag,
                static_cast<int>(ctx.rcx),
                priority,
                kScrWaterPriority);
        }
    }

    bool LooksLikeScrWaterDmapackCreateCall(uint8_t* functionBegin, uint8_t* call)
    {
        constexpr uint8_t movR8d144[] = { 0x41, 0xB8, 0x90, 0x00, 0x00, 0x00 };
        constexpr uint8_t movEdx4[] = { 0xBA, 0x04, 0x00, 0x00, 0x00 };
        constexpr uint8_t movEcx49[] = { 0xB9, 0x31, 0x00, 0x00, 0x00 };

        const size_t lookBack = std::min<size_t>(static_cast<size_t>(call - functionBegin), 0x60);
        const uint8_t* begin = call - lookBack;
        return ContainsBytes(begin, call, movR8d144) &&
            ContainsBytes(begin, call, movEdx4) &&
            ContainsBytes(begin, call, movEcx49);
    }

    void HookScrWaterDmapackCreateCalls(void* newScrWater)
    {
        if (!newScrWater)
        {
            return;
        }

        auto* bytes = static_cast<uint8_t*>(newScrWater);
        constexpr size_t scanSize = 0x8000;
        if (!IsReadable(bytes, scanSize))
        {
            spdlog::warn("MGS 2: Underwater Filter Fix: NewScrWater body was not readable; dmapack create hook skipped.");
            return;
        }

        for (size_t offset = 0; offset + 5 <= scanSize && ScrWaterDmapackCreateHookCount < static_cast<int>(ScrWaterDmapackCreateHooks.size()); ++offset)
        {
            uint8_t* call = bytes + offset;
            if (*call != 0xE8 || !LooksLikeScrWaterDmapackCreateCall(bytes, call))
            {
                continue;
            }

            SafetyHookMid& hook = ScrWaterDmapackCreateHooks[ScrWaterDmapackCreateHookCount++];
            hook = safetyhook::create_mid(call, [](SafetyHookContext& ctx) {
                PatchScrWaterDmapackCreateArgs(ctx);
            });

            LOG_HOOK(hook, "MGS 2: Underwater Filter Fix: ScrWater dmapack create")
            spdlog::info(
                "MGS 2: Underwater Filter Fix: ScrWater dmapack create hook target {}+0x{:X}.",
                sExeName,
                reinterpret_cast<uintptr_t>(call) - reinterpret_cast<uintptr_t>(baseModule));
        }

        if (ScrWaterDmapackCreateHookCount == 0)
        {
            spdlog::warn("MGS 2: Underwater Filter Fix: no ScrWater dmapack create call was found; queued priority remains unchanged.");
        }
    }
}

void MGS2UnderwaterFilterFix::InstallD3D11StateHooks()
{
    if (!g_D3D11Hooks.d3dDeviceContext)
    {
        return;
    }

    spdlog::info("MGS2 Underwater Filter Fix: Installing D3D11 state hooks.");

    if (!PSSetShaderResources_hook)
    {
        void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
        PSSetShaderResources_hook = safetyhook::create_inline(vtable[8], reinterpret_cast<void*>(PSSetShaderResources_Hook));
        LOG_HOOK(PSSetShaderResources_hook, "MGS 2: Underwater Filter Fix: ID3D11DeviceContext::PSSetShaderResources");
    }

    if (!Draw_hook)
    {
        void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
        Draw_hook = safetyhook::create_inline(vtable[13], reinterpret_cast<void*>(Draw_Hook));
        LOG_HOOK(Draw_hook, "MGS 2: Underwater Filter Fix: ID3D11DeviceContext::Draw");
    }

    if (!OMSetRenderTargets_hook)
    {
        void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
        OMSetRenderTargets_hook = safetyhook::create_inline(vtable[33], reinterpret_cast<void*>(OMSetRenderTargets_Hook));
        LOG_HOOK(OMSetRenderTargets_hook, "MGS 2: Underwater Filter Fix: ID3D11DeviceContext::OMSetRenderTargets");
    }
}

void MGS2UnderwaterFilterFix::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Underwater Filter Fix: Config disabled. Skipping");
        return;
    }

    if (uint8_t* NewScrWater_scan = Memory::PatternScan( baseModule, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 45 33 C9 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 ?? ?? ?? ?? 48 8B D8", "MGS 2: Underwater Filter Fix: shibata\\effect\\scr_water.c -> NewScrWater()"))
    {
        NewScrWater_hook = safetyhook::create_inline(NewScrWater_scan, reinterpret_cast<void*>(NewScrWater_Hook));
        LOG_HOOK(NewScrWater_hook, "MGS 2: Underwater Filter Fix: NewScrWater");
        spdlog::info("MGS 2: Underwater Filter Fix: NewScrWater hook target {}.", fmt::ptr(NewScrWater_scan));
        HookScrWaterDmapackCreateCalls(NewScrWater_scan);
    }

}

void MGS2UnderwaterFilterFix::PatchWork(void* work) const
{
    if (!bEnabled || !work)
    {
        return;
    }

    WorkInfo info = FindWorkInfo(work);
    RuntimeDmapack* dmapack = info.dmapack;
    if (!dmapack)
    {
        return;
    }

    PatchTuning(work, info.dmapackFieldOffset);
    if (gScrWaterWork != work)
    {
        gScrWaterWork = work;
        gScrWaterSawUnderwaterStep = false;
    }

    const int step = ScrWaterStep(work, info.dmapackFieldOffset);
    if (step >= 1 && step <= 2)
    {
        gScrWaterSawUnderwaterStep = true;
    }

    const bool codecFallback = step == 0 && !gScrWaterSawUnderwaterStep && IsUnderwaterCodecSequence();
    const bool active = ((step >= 1 && step <= 2) || codecFallback) && IsWaterVisible();
    gScrWaterActive = active;

    dmapack->flag |= kDmapackMenu;
    dmapack->priority = kScrWaterPriority;
    if (active)
    {
        dmapack->flag &= ~(kDmapackInvisible0 | kDmapackInvisibleMenu);
    }
    else
    {
        dmapack->flag |= kDmapackInvisible0 | kDmapackInvisibleMenu;
    }

    PatchPacket(info.packetMem);
}

void MGS2UnderwaterFilterFix::BeforePresent()
{
    if (!bEnabled)
    {
        return;
    }

    if (IsDemo() || !IsScrWaterActive())
    {
        gWaterFeedbackSubstitutedThisFrame = false;
        return;
    }

    if (!gWaterFeedbackSourceSeen ||
        !g_D3D11Hooks.swapChain ||
        !g_D3D11Hooks.d3dDeviceContext)
    {
        return;
    }

    ComPtr<ID3D11Texture2D> backBuffer;
    if (FAILED(g_D3D11Hooks.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()))) || !backBuffer)
    {
        return;
    }

    D3D11_TEXTURE2D_DESC backBufferDesc {};
    backBuffer->GetDesc(&backBufferDesc);
    if (!EnsureOwnedWaterFeedbackTexture(backBufferDesc))
    {
        return;
    }

    if (gD3D11CurrentPs0View.Get() == gOwnedWaterFeedbackSrv.Get())
    {
        ID3D11ShaderResourceView* nullView[1] = { nullptr };
        if (PSSetShaderResources_hook)
        {
            PSSetShaderResources_hook.call<void>(g_D3D11Hooks.d3dDeviceContext.Get(), 0, 1, nullView);
        }
        else
        {
            g_D3D11Hooks.d3dDeviceContext->PSSetShaderResources(0, 1, nullView);
        }
        gD3D11CurrentPs0View.Reset();
        gD3D11CurrentPs0Resource.Reset();
        gD3D11CurrentPs0 = nullptr;
    }

    g_D3D11Hooks.d3dDeviceContext->CopyResource(gOwnedWaterFeedbackTexture.Get(), backBuffer.Get());
    gOwnedWaterFeedbackHasFrame = true;
    gWaterFeedbackSubstitutedThisFrame = false;

}
