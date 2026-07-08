#include "stdafx.h"
#include "mgs2_railgun_beam.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "gamevars.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <set>
#include <mutex>
using Microsoft::WRL::ComPtr;

namespace
{
    // Stock sprite vertex shader (Prim.fx SPRITE=1), identified by DXBC digest.
    constexpr uint8_t kSpriteVSDigest[16] = {
        0xB1, 0x03, 0x51, 0x1F, 0x89, 0xD7, 0x41, 0xB7, 0xAF, 0x31, 0x0C, 0xD9, 0x77, 0x2C, 0x24, 0x14 };
    constexpr size_t kSpriteVSSize = 4544;

    // Untextured prims bind an all-white stand-in and go through the doubling shader path; a 0x80 grey twin, substituted per draw for the effect shaders only, restores GS brightness.
    constexpr uint8_t kPrimVSDigest[16] = {
        0x92, 0x12, 0x98, 0xF1, 0x43, 0xBD, 0xE0, 0xFC, 0xBC, 0x66, 0xB2, 0xD2, 0x83, 0xD7, 0x14, 0x39 };
    constexpr size_t kPrimVSSize = 4504;

    std::mutex g_standinMutex;
    std::set<ID3D11Resource*> g_standins;                 // live 4x4 white stand-in textures
    ID3D11VertexShader* g_effectVS[16] = {};              // created instances of the effect shaders
    int g_effectVSCount = 0;
    ComPtr<ID3D11Texture2D> g_greyTex;
    ComPtr<ID3D11ShaderResourceView> g_greySRV;

    void RememberEffectVS(ID3D11VertexShader* vs)
    {
        if (vs && g_effectVSCount < 16) g_effectVS[g_effectVSCount++] = vs;
    }

    bool IsEffectVS(ID3D11VertexShader* vs)
    {
        for (int i = 0; i < g_effectVSCount; ++i)
            if (g_effectVS[i] == vs) return true;
        return false;
    }

    bool EnsureGreyTwin(ID3D11Device* dev)
    {
        if (g_greySRV) return true;
        static const uint32_t kGrey[16] = {
            0x80808080,0x80808080,0x80808080,0x80808080, 0x80808080,0x80808080,0x80808080,0x80808080,
            0x80808080,0x80808080,0x80808080,0x80808080, 0x80808080,0x80808080,0x80808080,0x80808080 };
        D3D11_TEXTURE2D_DESC td {};
        td.Width = td.Height = 4; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM; td.SampleDesc = { 1, 0 };
        td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sd { kGrey, 16, 64 };
        if (FAILED(dev->CreateTexture2D(&td, &sd, g_greyTex.ReleaseAndGetAddressOf()))) return false;
        return SUCCEEDED(dev->CreateShaderResourceView(g_greyTex.Get(), nullptr, g_greySRV.ReleaseAndGetAddressOf()));
    }


    SafetyHookInline g_createDeviceHook {};
    SafetyHookInline g_createDeviceSwapHook {};
    SafetyHookInline g_createVSHook {};
    SafetyHookInline g_drawIndexedHook {};





    void STDMETHODCALLTYPE HookedDrawIndexed(ID3D11DeviceContext* ctx, UINT indexCount,
        UINT startIndex, INT baseVertex)
    {
        if (g_greySRV && g_effectVSCount)
        {
            ComPtr<ID3D11VertexShader> vs;
            ctx->VSGetShader(vs.GetAddressOf(), nullptr, nullptr);
            if (vs && IsEffectVS(vs.Get()))
            {
                ComPtr<ID3D11ShaderResourceView> srv;
                ctx->PSGetShaderResources(0, 1, srv.GetAddressOf());
                if (srv)
                {
                    ComPtr<ID3D11Resource> res;
                    srv->GetResource(res.GetAddressOf());
                    bool standin = false;
                    if (res)
                    {
                        std::lock_guard<std::mutex> lock(g_standinMutex);
                        standin = g_standins.count(res.Get()) != 0;
                    }
                    if (standin)
                    {
                        ID3D11ShaderResourceView* grey = g_greySRV.Get();
                        ctx->PSSetShaderResources(0, 1, &grey);
                        g_drawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
                        ID3D11ShaderResourceView* orig = srv.Get();
                        ctx->PSSetShaderResources(0, 1, &orig);
                        return;
                    }
                }
            }
        }
        g_drawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
    }


    SafetyHookInline g_updateSubHook {};

    void STDMETHODCALLTYPE HookedUpdateSubresource(ID3D11DeviceContext* ctx, ID3D11Resource* dst,
        UINT subresource, const D3D11_BOX* box, const void* data, UINT rowPitch, UINT depthPitch)
    {
        if (dst && data)
        {
            ComPtr<ID3D11Texture2D> tex;
            if (SUCCEEDED(dst->QueryInterface(IID_PPV_ARGS(tex.GetAddressOf()))) && tex)
            {
                D3D11_TEXTURE2D_DESC td; tex->GetDesc(&td);
                if (td.Width == td.Height && td.Width <= 64 && subresource == 0 &&
                    (td.Format == DXGI_FORMAT_B8G8R8A8_UNORM || td.Format == DXGI_FORMAT_R8G8B8A8_UNORM))
                {
                    // Stand-ins are all-white squares at several sizes.
                    bool allWhite = true;
                    for (UINT y = 0; allWhite && y < td.Height; ++y)
                    {
                        const uint32_t* row = reinterpret_cast<const uint32_t*>(
                            static_cast<const uint8_t*>(data) + static_cast<size_t>(y) * rowPitch);
                        for (UINT x = 0; x < td.Width; ++x)
                            if (row[x] != 0xFFFFFFFF) { allWhite = false; break; }
                    }
                    std::lock_guard<std::mutex> lock(g_standinMutex);
                    if (allWhite && g_standins.size() < 64) g_standins.insert(dst);
                    else if (!allWhite) g_standins.erase(dst);   // real content arrived - untrack
                }
            }
        }
        g_updateSubHook.stdcall<void>(ctx, dst, subresource, box, data, rowPitch, depthPitch);
    }

    HRESULT STDMETHODCALLTYPE HookedCreateVertexShader(ID3D11Device* dev, const void* bytecode,
        SIZE_T length, ID3D11ClassLinkage* linkage, ID3D11VertexShader** out)
    {
        const HRESULT hr = g_createVSHook.stdcall<HRESULT>(dev, bytecode, length, linkage, out);
        if (SUCCEEDED(hr) && bytecode && out && *out &&
            ((length == kSpriteVSSize && memcmp(static_cast<const uint8_t*>(bytecode) + 4, kSpriteVSDigest, 16) == 0) ||
             (length == kPrimVSSize && memcmp(static_cast<const uint8_t*>(bytecode) + 4, kPrimVSDigest, 16) == 0)))
        {
            RememberEffectVS(*out);
        }
        return hr;
    }


    void HookDevice(ID3D11Device* dev)
    {
        if (g_createVSHook || !dev) return;
        void** vtable = *reinterpret_cast<void***>(dev);
        g_createVSHook = safetyhook::create_inline(vtable[12], reinterpret_cast<void*>(HookedCreateVertexShader));
        LOG_HOOK(g_createVSHook, "MGS 2: Untextured prim brightness: CreateVertexShader");

        EnsureGreyTwin(dev);

        ComPtr<ID3D11DeviceContext> immCtx;
        dev->GetImmediateContext(immCtx.GetAddressOf());
        if (immCtx && !g_updateSubHook)
        {
            void** icvt = *reinterpret_cast<void***>(immCtx.Get());
            g_updateSubHook = safetyhook::create_inline(icvt[48], reinterpret_cast<void*>(HookedUpdateSubresource));
            LOG_HOOK(g_updateSubHook, "MGS 2: Untextured prim stand-in: UpdateSubresource");
        }

        ComPtr<ID3D11DeviceContext> ctx;
        dev->GetImmediateContext(ctx.GetAddressOf());
        if (ctx && !g_drawIndexedHook)
        {
            void** cvt = *reinterpret_cast<void***>(ctx.Get());
            g_drawIndexedHook = safetyhook::create_inline(cvt[12], reinterpret_cast<void*>(HookedDrawIndexed));
            LOG_HOOK(g_drawIndexedHook, "MGS 2: Missing-texture effect skip: DrawIndexed");
        }
    }

    HRESULT WINAPI HookedD3D11CreateDevice(IDXGIAdapter* adapter, D3D_DRIVER_TYPE driverType,
        HMODULE software, UINT flags, const D3D_FEATURE_LEVEL* levels, UINT numLevels, UINT sdkVersion,
        ID3D11Device** dev, D3D_FEATURE_LEVEL* level, ID3D11DeviceContext** ctx)
    {
        const HRESULT hr = g_createDeviceHook.stdcall<HRESULT>(adapter, driverType, software, flags,
            levels, numLevels, sdkVersion, dev, level, ctx);
        if (SUCCEEDED(hr) && dev && *dev) HookDevice(*dev);
        return hr;
    }

    HRESULT WINAPI HookedD3D11CreateDeviceAndSwapChain(IDXGIAdapter* adapter, D3D_DRIVER_TYPE driverType,
        HMODULE software, UINT flags, const D3D_FEATURE_LEVEL* levels, UINT numLevels, UINT sdkVersion,
        const DXGI_SWAP_CHAIN_DESC* scDesc, IDXGISwapChain** swap, ID3D11Device** dev,
        D3D_FEATURE_LEVEL* level, ID3D11DeviceContext** ctx)
    {
        const HRESULT hr = g_createDeviceSwapHook.stdcall<HRESULT>(adapter, driverType, software, flags,
            levels, numLevels, sdkVersion, scDesc, swap, dev, level, ctx);
        if (SUCCEEDED(hr) && dev && *dev) HookDevice(*dev);
        return hr;
    }
}

void MGS2RailgunBeam::InitializeEarly()
{
    if (!(eGameType & MGS2) || !bEnabled) return;

    HMODULE d3d11 = LoadLibraryW(L"d3d11.dll");
    if (!d3d11)
    {
        spdlog::error("MGS 2: Untextured prim brightness: d3d11.dll not found.");
        return;
    }
    if (auto* p = GetProcAddress(d3d11, "D3D11CreateDevice"))
    {
        g_createDeviceHook = safetyhook::create_inline(p, reinterpret_cast<void*>(HookedD3D11CreateDevice));
        LOG_HOOK(g_createDeviceHook, "MGS 2: Untextured prim brightness: D3D11CreateDevice");
    }
    if (auto* p = GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain"))
    {
        g_createDeviceSwapHook = safetyhook::create_inline(p, reinterpret_cast<void*>(HookedD3D11CreateDeviceAndSwapChain));
        LOG_HOOK(g_createDeviceSwapHook, "MGS 2: Untextured prim brightness: D3D11CreateDeviceAndSwapChain");
    }
}
