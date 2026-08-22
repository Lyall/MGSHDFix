#include "stdafx.h"
#include "mgs2_railgun_beam.hpp"

#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"
#include "gamevars.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <set>
#include <mutex>
using Microsoft::WRL::ComPtr;

namespace
{
    std::mutex g_standinMutex;
    std::set<ID3D11Resource*> g_standins;                 // live 4x4 white stand-in textures
    ComPtr<ID3D11Texture2D> g_greyTex;
    ComPtr<ID3D11ShaderResourceView> g_greySRV;

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


    SafetyHookInline g_drawIndexedHook {};





    void STDMETHODCALLTYPE HookedDrawIndexed(ID3D11DeviceContext* ctx, UINT indexCount,
        UINT startIndex, INT baseVertex)
    {
        if (g_greySRV)
        {
            ComPtr<ID3D11VertexShader> vs;
            ctx->VSGetShader(vs.GetAddressOf(), nullptr, nullptr);
            const D3D11Hooks::StockVS kind = D3D11Hooks::GetStockVS(vs.Get());
            if (kind == D3D11Hooks::StockVS::Sprite || kind == D3D11Hooks::StockVS::Poly)
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
}

void MGS2RailgunBeam::OnDeviceCreated(ID3D11Device* dev)
{
    if (!(eGameType & MGS2) || !bEnabled || !dev || g_drawIndexedHook) return;

    EnsureGreyTwin(dev);

    ComPtr<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext(ctx.GetAddressOf());
    if (!ctx) return;
    void** cvt = *reinterpret_cast<void***>(ctx.Get());
    g_updateSubHook = safetyhook::create_inline(cvt[48], reinterpret_cast<void*>(HookedUpdateSubresource));
    LOG_HOOK(g_updateSubHook, "MGS 2: Untextured prim stand-in: UpdateSubresource");
    g_drawIndexedHook = safetyhook::create_inline(cvt[12], reinterpret_cast<void*>(HookedDrawIndexed));
    LOG_HOOK(g_drawIndexedHook, "MGS 2: Missing-texture effect skip: DrawIndexed");
}
