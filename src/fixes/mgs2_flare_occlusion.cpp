#include "stdafx.h"
#include "mgs2_flare_occlusion.hpp"

#include "common.hpp"
#include "d3d11_api.hpp"
#include "scene_depth.hpp"
#include "game_funcs.hpp"
#include "gamevars.hpp"
#include "logging.hpp"

namespace
{
    constexpr float kQuadHalf    = 26400.0f;
    constexpr UINT  kPatch       = 8;
    constexpr float kDepthMargin = 0.002f;

    std::atomic<float>     g_scrX { 0.0f };
    std::atomic<float>     g_scrY { 0.0f };
    std::atomic<float>     g_sunDepth { 0.0f };
    std::atomic<float>     g_cornerOfs { 0.5f };
    std::atomic<int>       g_depthBlockedMask { 0 };
    std::atomic<ULONGLONG> g_lastActMs { 0 };
    std::atomic<float>     g_visibility { 1.0f };

    ComPtr<ID3D11Texture2D> g_staging[2];
    DXGI_FORMAT g_stFmt = DXGI_FORMAT_UNKNOWN;
    int  g_flip = 0;
    bool g_primed = false;

    bool g_rayFaulted = false;

    int GuardedCheck(int id, const float* from, const float* to, int chk, int seg, int flr)
    {
        __try
        {
            return MGS2_GameFuncs::HZX_OnlineHazardCheck(id, from, to, chk, seg, flr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_rayFaulted = true;
            return 0;
        }
    }

    bool RayHitsWorld(const float* from, const float* to)
    {
        int r = GuardedCheck(0, from, to, 0x0A, 0x40, 0x40);
        int* currentGroup = g_GameVars.HZX_CurrentGroupID();
        const int saved = *currentGroup;
        *currentGroup = 0;
        r |= GuardedCheck(saved, from, to, 0x8F, 0, 0);
        *currentGroup = saved;
        if (g_rayFaulted)
        {
            spdlog::warn("MGS 2: Flare Occlusion: collision check failed; using depth only.");
            return true;
        }
        return (r & 3) != 0;
    }

    void OnEndOf3D(ID3D11RenderTargetView*, ID3D11ShaderResourceView* depth)
    {
        const ULONGLONG last = g_lastActMs.load(std::memory_order_relaxed);
        if (!last || GetTickCount64() - last > 250)
        {
            g_visibility.store(1.0f, std::memory_order_relaxed);
            g_depthBlockedMask.store(0, std::memory_order_relaxed);
            g_primed = false;
            return;
        }
        if (!depth) return;
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!dev || !ctx) return;

        ComPtr<ID3D11Resource> res;
        depth->GetResource(res.GetAddressOf());
        ComPtr<ID3D11Texture2D> tex;
        if (!res || FAILED(res.As(&tex)) || !tex) return;
        D3D11_TEXTURE2D_DESC td {};
        tex->GetDesc(&td);
        if (td.Width < kPatch * 8 || td.Height < kPatch * 8 || td.SampleDesc.Count != 1) return;

        if (!g_staging[0] || g_stFmt != td.Format)
        {
            for (auto& s : g_staging) s.Reset();
            D3D11_TEXTURE2D_DESC sd {};
            sd.Width = kPatch * 4; sd.Height = kPatch; sd.MipLevels = 1; sd.ArraySize = 1;
            sd.Format = td.Format; sd.SampleDesc = { 1, 0 };
            sd.Usage = D3D11_USAGE_STAGING; sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            if (FAILED(dev->CreateTexture2D(&sd, nullptr, g_staging[0].GetAddressOf())) ||
                FAILED(dev->CreateTexture2D(&sd, nullptr, g_staging[1].GetAddressOf())))
            {
                for (auto& s : g_staging) s.Reset();
                return;
            }
            g_stFmt = td.Format;
            g_primed = false;
            g_flip = 0;
        }

        const float u = (g_scrX.load(std::memory_order_relaxed) + 1.0f) * 0.5f;
        const float v = (1.0f - g_scrY.load(std::memory_order_relaxed)) * 0.5f;
        const float du = g_cornerOfs.load(std::memory_order_relaxed) * 0.5f;
        const float dv = du * ((float)td.Width / (float)td.Height);
        const float cu[4] = { u - du, u + du, u - du, u + du };
        const float cv[4] = { v - dv, v - dv, v + dv, v + dv };
        for (int i = 0; i < 4; ++i)
        {
            LONG x0 = std::clamp<LONG>((LONG)(cu[i] * td.Width)  - (LONG)kPatch / 2, 0, (LONG)td.Width  - (LONG)kPatch);
            LONG y0 = std::clamp<LONG>((LONG)(cv[i] * td.Height) - (LONG)kPatch / 2, 0, (LONG)td.Height - (LONG)kPatch);
            const D3D11_BOX box { (UINT)x0, (UINT)y0, 0, (UINT)x0 + kPatch, (UINT)y0 + kPatch, 1 };
            ctx->CopySubresourceRegion(g_staging[g_flip].Get(), 0, (UINT)i * kPatch, 0, 0, tex.Get(), 0, &box);
        }

        if (g_primed)
        {
            D3D11_MAPPED_SUBRESOURCE m {};
            if (SUCCEEDED(ctx->Map(g_staging[g_flip ^ 1].Get(), 0, D3D11_MAP_READ, 0, &m)))
            {
                const float dThresh = g_sunDepth.load(std::memory_order_relaxed) + kDepthMargin;
                const bool  isFloat = (g_stFmt == DXGI_FORMAT_R32_TYPELESS || g_stFmt == DXGI_FORMAT_R32_FLOAT ||
                                       g_stFmt == DXGI_FORMAT_D32_FLOAT);
                int mask = 0;
                for (int i = 0; i < 4; ++i)
                {
                    int nearCnt = 0;
                    for (UINT y = 0; y < kPatch; ++y)
                    {
                        const uint8_t* row = static_cast<const uint8_t*>(m.pData) + (size_t)y * m.RowPitch;
                        for (UINT x = i * kPatch; x < (i + 1) * kPatch; ++x)
                        {
                            const float dz = isFloat
                                ? reinterpret_cast<const float*>(row)[x]
                                : (float)(reinterpret_cast<const uint32_t*>(row)[x] & 0xFFFFFF) / 16777215.0f;
                            if (dz > dThresh) ++nearCnt;
                        }
                    }
                    if (nearCnt * 2 > (int)(kPatch * kPatch)) mask |= 1 << i;
                }
                ctx->Unmap(g_staging[g_flip ^ 1].Get(), 0);
                g_depthBlockedMask.store(mask, std::memory_order_relaxed);
            }
        }
        g_primed = true;
        g_flip ^= 1;
    }
}

void MGS2FlareOcclusion::SetSunState(const float* sunWorldPos, float scrX, float scrY, float clipZ, float clipW)
{
    g_lastActMs.store(GetTickCount64(), std::memory_order_relaxed);
    g_scrX.store(scrX, std::memory_order_relaxed);
    g_scrY.store(scrY, std::memory_order_relaxed);
    if (clipW > 0.001f)
    {
        g_sunDepth.store(std::clamp((clipZ / clipW + 1.0f) * 0.5f, 0.0f, 1.0f), std::memory_order_relaxed);
        g_cornerOfs.store(std::clamp(kQuadHalf / clipW, 0.005f, 0.45f), std::memory_order_relaxed);
    }

    const int mask = g_depthBlockedMask.load(std::memory_order_relaxed);
    int sum = 0;
    const auto* chanl = g_GameVars.DG_Chanl(0);
    const bool canRay = MGS2_GameFuncs::HZX_OnlineHazardCheck && g_GameVars.HZX_CurrentGroupID() && !g_rayFaulted && chanl && sunWorldPos;
    for (int i = 0; i < 4; ++i)
    {
        bool blocked = (mask & (1 << i)) != 0;
        if (blocked && canRay)
        {
            const float* right = chanl->eye.m[0];
            const float* up    = chanl->eye.m[1];
            const float* cam   = chanl->eye.m[3];
            const float sx = (i & 1) ? 1.0f : -1.0f;
            const float sy = (i & 2) ? 1.0f : -1.0f;
            const float corner[4] = {
                sunWorldPos[0] + (right[0] * sx + up[0] * sy) * kQuadHalf,
                sunWorldPos[1] + (right[1] * sx + up[1] * sy) * kQuadHalf,
                sunWorldPos[2] + (right[2] * sx + up[2] * sy) * kQuadHalf, 1.0f };
            const float camPos[4] = { cam[0], cam[1], cam[2], 1.0f };
            blocked = RayHitsWorld(corner, camPos);
        }
        sum += blocked ? -1 : 1;
    }

    const float ratio = (float)(4 + sum) / 8.0f;
    g_visibility.store(ratio, std::memory_order_relaxed);
}

float MGS2FlareOcclusion::GetVisibility()
{
    const ULONGLONG last = g_lastActMs.load(std::memory_order_relaxed);
    if (!last || GetTickCount64() - last > 250) return 1.0f;
    return g_visibility.load(std::memory_order_relaxed);
}

void MGS2FlareOcclusion::Initialize()
{
    if (!(eGameType & MGS2)) return;

    SceneDepth::SetEndOf3DCallback(&OnEndOf3D, SceneDepth::PRIORITY_DEFAULT);

    if (!MGS2_GameFuncs::HZX_OnlineHazardCheck || !g_GameVars.HZX_CurrentGroupID())
        spdlog::warn("MGS 2: Flare Occlusion: collision check unavailable; using depth only.");
}
