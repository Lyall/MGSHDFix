#include "stdafx.h"
#include "mgs2_scanline_scale.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

//mgs2x\source\user\takabe\effect1\raster.c -> NewRasterEffect() | (mgs2x\source\user\skoba\weapon\equip_layout.c -> NewDEMO_Equip() / mgs2x\source\user\skoba\weapon\vtr_layout.c -> NewVtrSight() / mgs2x\source\user\skoba\weapon\ray_layout.c -> NewRaySight())

// The raster effect lays 112 lines down the screen, 3 units tall with a 1 unit gap. Scale that to
// a vertical resolution it doesn't divide into and lines land on a mix of pixel counts, so round
// the pitch up to whole pixels and let the extra run off the bottom.
//
// Has to be the final vertices - the packet stores whole PS2 units and truncates anything finer.

namespace
{
    SafetyHookInline gIASetVB{};
    SafetyHookInline gMap{};
    SafetyHookInline gUnmap{};
    ID3D11DeviceContext* gImmediate = nullptr;

    ID3D11Buffer* gVB = nullptr;
    UINT gStride = 0;
    UINT gOffset = 0;

    std::mutex gLock;
    std::unordered_map<ID3D11Resource*, uint8_t*> gMapped;
    std::atomic<int> gMappedCount{ 0 };

    float gScale = 1.0f;

    struct Vertex { float x, y, z, w, r, g, b, a; };

    void Recalculate()
    {
        float h = f_PS2_Height;
        if (IDXGISwapChain* sc = g_D3D11Hooks.swapChain.Get())
        {
            DXGI_SWAP_CHAIN_DESC d{};
            if (SUCCEEDED(sc->GetDesc(&d)) && d.BufferDesc.Height > 0)
            {
                h = static_cast<float>(d.BufferDesc.Height);
            }
        }
        const float scale = h / f_PS2_Height;
        gScale = (scale > 0.0f) ? ceilf(scale) / scale : 1.0f;
    }

    bool Near(float v, float want)
    {
        return fabsf(v - want) < 0.001f;
    }

    // Konami's line: full width, 3 units tall, and its own dark green. The letterbox bars and
    // the screen fill share everything else about the draw, so the shape is what tells them apart.
    bool IsScanline(const Vertex* v)
    {
        float lo = v[0].x, hi = v[0].x, top = v[0].y, bot = v[0].y;
        for (int i = 1; i < 4; i++)
        {
            lo = std::min(lo, v[i].x); hi = std::max(hi, v[i].x);
            top = std::min(top, v[i].y); bot = std::max(bot, v[i].y);
        }
        if (!Near(lo, 0.0f) || !Near(hi, f_PS2_Width) || !Near(bot - top, 3.0f))
        {
            return false;
        }
        for (int i = 0; i < 4; i++)
        {
            if (!Near(v[i].r, 0.0f) || !Near(v[i].g, 32.0f / 255.0f) ||
                !Near(v[i].b, 0.0f) || !Near(v[i].a, 54.0f / 255.0f))
            {
                return false;
            }
        }
        return true;
    }

    void __stdcall HookedIASetVertexBuffers(ID3D11DeviceContext* ctx, UINT slot, UINT num,
        ID3D11Buffer* const* buffers, const UINT* strides, const UINT* offsets)
    {
        if (ctx == gImmediate && slot == 0 && num > 0)
        {
            gVB = buffers ? buffers[0] : nullptr;
            gStride = strides ? strides[0] : 0;
            gOffset = offsets ? offsets[0] : 0;
        }
        gIASetVB.call<void>(ctx, slot, num, buffers, strides, offsets);
    }

    HRESULT __stdcall HookedMap(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub,
        D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* out)
    {
        const HRESULT hr = gMap.call<HRESULT>(ctx, res, sub, type, flags, out);
        if (SUCCEEDED(hr) && out && out->pData && sub == 0 && res == gVB && gStride == 32)
        {
            std::lock_guard<std::mutex> lk(gLock);
            gMapped[res] = static_cast<uint8_t*>(out->pData);
            gMappedCount.store(static_cast<int>(gMapped.size()), std::memory_order_release);
        }
        return hr;
    }

    void __stdcall HookedUnmap(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub)
    {
        // Runs on every unmap in the game, so nothing tracked means nothing to look at.
        if (gMappedCount.load(std::memory_order_acquire) != 0)
        {
            std::lock_guard<std::mutex> lk(gLock);
            auto it = gMapped.find(res);
            if (it != gMapped.end())
            {
                if (gStride == 32)
                {
                    Vertex* v = reinterpret_cast<Vertex*>(it->second + gOffset);
                    if (IsScanline(v))
                    {
                        // a scaled line no longer measures 3 units, so this cannot stack
                        for (int i = 0; i < 4; i++) { v[i].y *= gScale; }
                    }
                }
                gMapped.erase(it);
                gMappedCount.store(static_cast<int>(gMapped.size()), std::memory_order_release);
            }
        }
        gUnmap.call<void>(ctx, res, sub);
    }
}

void MGS2ScanlineScale::OnDeviceReady()
{
    if (!(eGameType & MGS2) || !bEnabled || gMap)
    {
        return;
    }
    ID3D11DeviceContext* dc = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!dc)
    {
        return;
    }
    gImmediate = dc;
    Recalculate();

    void** vt = *reinterpret_cast<void***>(dc);
    gMap = safetyhook::create_inline(vt[14], reinterpret_cast<void*>(HookedMap));
    LOG_HOOK(gMap, "MGS 2: Scanline Scale: ID3D11DeviceContext::Map")
    gUnmap = safetyhook::create_inline(vt[15], reinterpret_cast<void*>(HookedUnmap));
    LOG_HOOK(gUnmap, "MGS 2: Scanline Scale: ID3D11DeviceContext::Unmap")
    gIASetVB = safetyhook::create_inline(vt[18], reinterpret_cast<void*>(HookedIASetVertexBuffers));
    LOG_HOOK(gIASetVB, "MGS 2: Scanline Scale: ID3D11DeviceContext::IASetVertexBuffers")
}

void MGS2ScanlineScale::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    if (!bEnabled)
    {
        spdlog::info("MGS 2: Scanline Scale: Disabled via config, skipping.");
    }
}
