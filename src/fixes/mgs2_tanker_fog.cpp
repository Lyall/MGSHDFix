#include "stdafx.h"

#include "mgs2_tanker_fog.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

#include <atomic>
#include <mutex>
#include <unordered_set>

namespace
{
    std::mutex gLock;
    std::unordered_set<void*> gMasks;
    std::atomic<bool> gArmed{ false };

    // Prim.fx reads TEXCOORD0 as u_v_dx_dy; dx/dy nudge the vertex in view space.
    struct FogVertex
    {
        float x, y, z;
        int16_t u, v, dx, dy;
        uint8_t colour[4];
    };
    static_assert(sizeof(FogVertex) == 24, "the game's fog vertices are 24 bytes");

    constexpr UINT kFogVertexStride = sizeof(FogVertex);

    // Where the mask lands across the curtain - evenly, so its thick middle ends up in the middle.
    constexpr int16_t kU[3] = { 2820, 3071, 3324 };
    constexpr int16_t kVTop = 2818;
    constexpr int16_t kVBottom = 2942;

    struct Band
    {
        float x0, z0;
        float x1, z1;
        float yTop;
        float yBottom;
    };

    // The sea is a fixed +-60000 square but stage fog only saturates near 82000, so its last tiles
    // keep a sixth of their water colour against a fogged void.
    // +z only: anything down the river converges into mid-frame, and -z is the Manhattan sightline.
    constexpr float kSeaEdge = 58000.0f;
    constexpr float kSeaSpan = 60000.0f;
    constexpr float kBankTop = 12000.0f;
    constexpr float kBankBottom = -6000.0f;

    constexpr Band kBands[] = {
        { -kSeaSpan, kSeaEdge, kSeaSpan, kSeaEdge, kBankTop, kBankBottom },
    };

    constexpr float kSeaCurtainSpan = 100000.0f;

    // The slanted Manhattan curtain. Folding softened the end that used to hide the skyline, so this
    // one starts at the mask's peak instead of its faded edge and falls away from there.
    constexpr float kManhattanX[2] = { -102153.0f, -67847.0f };
    constexpr float kManhattanZ[2] = { -474157.0f, -125843.0f };
    constexpr float kMatchTolerance = 2000.0f;
    constexpr int16_t kPeakU = 3072;                // U = u/504 - 5.5952, so 3072 is halfway across

    // One wall, two jobs. Seen across, it is the haze hiding the city and has to stay covered; seen
    // along, its end is a hard line on screen and has to fade. Five cameras measure 0.62 across
    // against 0.24 and below along, so the gap is wide.
    constexpr float kFaceOn = 0.4f;
    constexpr UINT kConstantBytes = 10240;
    constexpr size_t kForwardFloat = 72;            // third row of gVS_Screen, which sits at c16

    ComPtr<ID3D11Buffer> gBandVB;
    ComPtr<ID3D11Buffer> gManhattanVB;
    std::atomic<bool> gBandsDrawn{ false };

    ComPtr<ID3D11ShaderResourceView> gThinnedSRV;
    std::vector<std::vector<uint8_t>> gThinnedMips;
    D3D11_TEXTURE2D_DESC gMaskDesc{};

    SafetyHookInline gUpdateSubHook{};
    SafetyHookInline gDrawIndexedHook{};
    SafetyHookInline gMapHook{};
    SafetyHookInline gUnmapHook{};

    ID3D11Resource* gConstants = nullptr;
    void* gConstantData = nullptr;
    float gCameraForward[3] = { 0.0f, 0.0f, 1.0f };

    // w00_fog_fader_alp hash
    constexpr uint64_t kFogMaskHash = 0xb766ba4ac57cd459ull;

    bool LooksLikeFogMask(const D3D11_TEXTURE2D_DESC& desc, const void* data, const UINT rowPitch)
    {
        if (desc.Width != 64 || desc.Height != 32 || !data || rowPitch < desc.Width * 4)
        {
            return false;
        }
        if (desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM)
        {
            return false;
        }

        return Util::HashTexels(data, rowPitch, desc.Width, desc.Height) == kFogMaskHash;
    }

    // The camera only reaches us through the constants the game maps every pass.
    HRESULT STDMETHODCALLTYPE HookedMap(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub,
        D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped)
    {
        const HRESULT hr = gMapHook.stdcall<HRESULT>(ctx, res, sub, type, flags, mapped);
        if (FAILED(hr) || !res || !mapped)
        {
            return hr;
        }

        if (res == gConstants)
        {
            gConstantData = mapped->pData;
        }
        else if (!gConstants)
        {
            ComPtr<ID3D11Buffer> buffer;
            if (SUCCEEDED(res->QueryInterface(IID_PPV_ARGS(buffer.GetAddressOf()))) && buffer)
            {
                D3D11_BUFFER_DESC desc{};
                buffer->GetDesc(&desc);
                if (desc.ByteWidth == kConstantBytes && (desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER))
                {
                    gConstants = res;
                    gConstantData = mapped->pData;
                }
            }
        }
        return hr;
    }

    void STDMETHODCALLTYPE HookedUnmap(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub)
    {
        if (res && res == gConstants && gConstantData)
        {
            memcpy(gCameraForward, static_cast<const float*>(gConstantData) + kForwardFloat,
                sizeof(gCameraForward));
            gConstantData = nullptr;
        }
        gUnmapHook.stdcall<void>(ctx, res, sub);
    }

    bool LooksAcrossWall(const FogVertex (&strip)[6])
    {
        const float dx = strip[4].x - strip[0].x;
        const float dz = strip[4].z - strip[0].z;
        const float wall = std::sqrt(dx * dx + dz * dz);
        const float fx = gCameraForward[0];
        const float fz = gCameraForward[2];
        const float view = std::sqrt(fx * fx + fz * fz);
        if (wall < 1.0f || view < 1e-4f)
        {
            return false;
        }
        return std::fabs(fx / view * (dz / wall) - fz / view * (dx / wall)) > kFaceOn;
    }

    void BuildThinnedMask()
    {
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev || gThinnedMips.size() < gMaskDesc.MipLevels)
        {
            return;
        }

        std::vector<D3D11_SUBRESOURCE_DATA> levels(gMaskDesc.MipLevels);
        for (UINT mip = 0; mip < gMaskDesc.MipLevels; mip++)
        {
            levels[mip].pSysMem = gThinnedMips[mip].data();
            levels[mip].SysMemPitch = std::max(1u, gMaskDesc.Width >> mip) * 4;
        }

        D3D11_TEXTURE2D_DESC desc = gMaskDesc;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        ComPtr<ID3D11Texture2D> tex;
        if (FAILED(dev->CreateTexture2D(&desc, levels.data(), tex.GetAddressOf())))
        {
            return;
        }
        if (SUCCEEDED(dev->CreateShaderResourceView(tex.Get(), nullptr, gThinnedSRV.ReleaseAndGetAddressOf())))
        {
            spdlog::info("MGS2TankerFog: sea fog mask thinned, {}x{}, {} mips.",
                desc.Width, desc.Height, desc.MipLevels);
        }
    }

    void STDMETHODCALLTYPE HookedUpdateSubresource(ID3D11DeviceContext* ctx, ID3D11Resource* dst,
        UINT subresource, const D3D11_BOX* box, const void* data, UINT rowPitch, UINT depthPitch)
    {
        // Every mip has to be thinned too, or the curtain changes as it minifies.
        static ID3D11Resource* collecting = nullptr;
        if (dst && data && !box)
        {
            ComPtr<ID3D11Texture2D> tex;
            if (SUCCEEDED(dst->QueryInterface(IID_PPV_ARGS(tex.GetAddressOf()))) && tex)
            {
                D3D11_TEXTURE2D_DESC desc{};
                tex->GetDesc(&desc);
                if (subresource == 0)
                {
                    const std::lock_guard<std::mutex> guard(gLock);
                    collecting = LooksLikeFogMask(desc, data, rowPitch) ? dst : nullptr;
                    if (collecting)
                    {
                        gMaskDesc = desc;
                        gThinnedMips.clear();
                        gMasks.insert(dst);
                        gArmed.store(true, std::memory_order_relaxed);
                    }
                }

                if (dst == collecting && subresource == gThinnedMips.size())
                {
                    const UINT width = std::max(1u, gMaskDesc.Width >> subresource);
                    const UINT height = std::max(1u, gMaskDesc.Height >> subresource);
                    std::vector<uint8_t> thinned(static_cast<size_t>(width) * height * 4);
                    for (UINT y = 0; y < height; y++)
                    {
                        const auto* in = static_cast<const uint8_t*>(data) + static_cast<size_t>(y) * rowPitch;
                        uint8_t* out = thinned.data() + static_cast<size_t>(y) * width * 4;
                        for (UINT x = 0; x < width; x++)
                        {
                            const uint8_t* mirror = in + (width - 1 - x) * 4;
                            for (int c = 0; c < 4; c++)
                            {
                                out[x * 4 + c] = std::min(in[x * 4 + c], mirror[c]);
                            }
                        }
                    }

                    const std::lock_guard<std::mutex> guard(gLock);
                    gThinnedMips.push_back(std::move(thinned));
                    if (gThinnedMips.size() == gMaskDesc.MipLevels)
                    {
                        BuildThinnedMask();
                    }
                }
            }
        }
        gUpdateSubHook.stdcall<void>(ctx, dst, subresource, box, data, rowPitch, depthPitch);
    }

    void BuildBands()
    {
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev || gBandVB)
        {
            return;
        }

        std::vector<FogVertex> verts;
        verts.reserve(std::size(kBands) * 6);
        for (const Band& band : kBands)
        {
            for (int c = 0; c < 3; c++)
            {
                const float t = c * 0.5f;
                for (const float y : { band.yTop, band.yBottom })
                {
                    FogVertex v{};
                    v.x = band.x0 + (band.x1 - band.x0) * t;
                    v.y = y;
                    v.z = band.z0 + (band.z1 - band.z0) * t;
                    v.u = kU[c];
                    v.v = (y == band.yTop) ? kVTop : kVBottom;
                    v.dx = 4096;
                    v.dy = 0;
                    v.colour[0] = v.colour[1] = v.colour[2] = v.colour[3] = 0x80;
                    verts.push_back(v);
                }
            }
        }

        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = static_cast<UINT>(verts.size() * sizeof(FogVertex));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = verts.data();
        if (SUCCEEDED(dev->CreateBuffer(&desc, &init, gBandVB.GetAddressOf())))
        {
            spdlog::info("MGS2TankerFog: {} cross-river band(s) ready.", std::size(kBands));
        }
    }

    // Staging copy because the game's buffer is write-only. Callers cache the answer.
    bool ReadStrip(ID3D11DeviceContext* ctx, ID3D11Buffer* vb, UINT vbOffset, UINT firstVertex,
        FogVertex (&strip)[6])
    {
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev)
        {
            return false;
        }

        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = 6 * kFogVertexStride;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ComPtr<ID3D11Buffer> staging;
        if (FAILED(dev->CreateBuffer(&desc, nullptr, staging.GetAddressOf())))
        {
            return false;
        }

        D3D11_BOX box{};
        box.left = vbOffset + firstVertex * kFogVertexStride;
        box.right = box.left + desc.ByteWidth;
        box.bottom = 1;
        box.back = 1;
        ctx->CopySubresourceRegion(staging.Get(), 0, 0, 0, 0, vb, 0, &box);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        {
            return false;
        }
        memcpy(strip, mapped.pData, sizeof(strip));
        ctx->Unmap(staging.Get(), 0);
        return true;
    }

    void Bounds(const FogVertex (&strip)[6], float& lowX, float& highX, float& lowZ, float& highZ)
    {
        lowX = highX = strip[0].x;
        lowZ = highZ = strip[0].z;
        for (const FogVertex& v : strip)
        {
            lowX = std::min(lowX, v.x);
            highX = std::max(highX, v.x);
            lowZ = std::min(lowZ, v.z);
            highZ = std::max(highZ, v.z);
        }
    }

    bool IsManhattanCurtain(const FogVertex (&strip)[6])
    {
        float lowX, highX, lowZ, highZ;
        Bounds(strip, lowX, highX, lowZ, highZ);
        return std::fabs(lowX - kManhattanX[0]) < kMatchTolerance
            && std::fabs(highX - kManhattanX[1]) < kMatchTolerance
            && std::fabs(lowZ - kManhattanZ[0]) < kMatchTolerance
            && std::fabs(highZ - kManhattanZ[1]) < kMatchTolerance;
    }

    // Same vertices, but no sample below the peak - the skyline end stays covered and fades from
    // there instead of dropping to nothing.
    void BuildManhattanVB(const FogVertex (&strip)[6])
    {
        auto* dev = g_D3D11Hooks.d3dDevice.Get();
        if (!dev || gManhattanVB)
        {
            return;
        }

        FogVertex verts[6];
        memcpy(verts, strip, sizeof(verts));
        for (FogVertex& v : verts)
        {
            v.u = std::max(v.u, kPeakU);
        }

        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(verts);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = verts;
        dev->CreateBuffer(&desc, &init, gManhattanVB.GetAddressOf());
    }

    bool IsSeaCurtain(const FogVertex (&strip)[6])
    {
        float lowX, highX, lowZ, highZ;
        Bounds(strip, lowX, highX, lowZ, highZ);
        return std::max(highX - lowX, highZ - lowZ) > kSeaCurtainSpan;
    }

    bool BoundToFogMask(ID3D11DeviceContext* ctx, ComPtr<ID3D11ShaderResourceView>& srv)
    {
        ctx->PSGetShaderResources(0, 1, srv.GetAddressOf());
        if (!srv)
        {
            return false;
        }
        ComPtr<ID3D11Resource> res;
        srv->GetResource(res.GetAddressOf());
        if (!res)
        {
            return false;
        }
        const std::lock_guard<std::mutex> guard(gLock);
        return gMasks.count(res.Get()) != 0;
    }

    void STDMETHODCALLTYPE HookedDrawIndexed(ID3D11DeviceContext* ctx, UINT indexCount,
        UINT startIndex, INT baseVertex)
    {
        ComPtr<ID3D11ShaderResourceView> theirs;
        if (!gArmed.load(std::memory_order_relaxed) || indexCount != 6 || !gThinnedSRV
            || !BoundToFogMask(ctx, theirs))
        {
            gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }

        ComPtr<ID3D11Buffer> vb;
        UINT stride = 0;
        UINT vbOffset = 0;
        ctx->IAGetVertexBuffers(0, 1, vb.GetAddressOf(), &stride, &vbOffset);
        if (!vb || stride != kFogVertexStride)
        {
            gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }

        // Each curtain draws once a frame, so there is nothing to gain by remembering the answer -
        // and the offset it would have to be remembered against is one the game recycles.
        bool sea = false;
        ComPtr<ID3D11Buffer> remapped;
        FogVertex strip[6]{};
        if (ReadStrip(ctx, vb.Get(), vbOffset, static_cast<UINT>(std::max(0, baseVertex)), strip))
        {
            sea = IsSeaCurtain(strip);
            // Cutscenes only - their framing is authored, so the wall is either hiding the city or
            // showing its edge. In gameplay the player swings the camera through both and it pops.
            if (g_GameVars.InCutscene() && IsManhattanCurtain(strip) && LooksAcrossWall(strip))
            {
                const std::lock_guard<std::mutex> guard(gLock);
                BuildManhattanVB(strip);
                remapped = gManhattanVB;
            }
        }

        ID3D11ShaderResourceView* thinned = gThinnedSRV.Get();
        ctx->PSSetShaderResources(0, 1, &thinned);

        if (remapped)
        {
            ID3D11Buffer* mine = remapped.Get();
            const UINT stride24 = kFogVertexStride;
            const UINT zero = 0;
            ctx->IASetVertexBuffers(0, 1, &mine, &stride24, &zero);
            ctx->Draw(6, 0);
            ID3D11Buffer* restore = vb.Get();
            ctx->IASetVertexBuffers(0, 1, &restore, &stride, &vbOffset);
        }
        else
        {
            gDrawIndexedHook.stdcall<void>(ctx, indexCount, startIndex, baseVertex);
        }

        // Piggyback the first sea wall of the frame - its whole state is already bound.
        if (gBandVB && sea && !gBandsDrawn.load(std::memory_order_relaxed))
        {
            ID3D11Buffer* mine = gBandVB.Get();
            const UINT bandStride = kFogVertexStride;
            const UINT zero = 0;
            ctx->IASetVertexBuffers(0, 1, &mine, &bandStride, &zero);
            for (UINT i = 0; i < std::size(kBands); i++)
            {
                ctx->Draw(6, i * 6);
            }
            ID3D11Buffer* restore = vb.Get();
            ctx->IASetVertexBuffers(0, 1, &restore, &stride, &vbOffset);
            gBandsDrawn.store(true, std::memory_order_relaxed);
        }

        ID3D11ShaderResourceView* original = theirs.Get();
        ctx->PSSetShaderResources(0, 1, &original);
    }
}

void MGS2TankerFog::OnPresent()
{
    gBandsDrawn.store(false, std::memory_order_relaxed);
}

void MGS2TankerFog::OnDeviceReady()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    auto* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!ctx)
    {
        spdlog::error("MGS2TankerFog: no device context.");
        return;
    }

    void** vtable = *reinterpret_cast<void***>(ctx);
    gDrawIndexedHook = safetyhook::create_inline(vtable[12], reinterpret_cast<void*>(HookedDrawIndexed));
    gUpdateSubHook = safetyhook::create_inline(vtable[48], reinterpret_cast<void*>(HookedUpdateSubresource));
    gMapHook = safetyhook::create_inline(vtable[14], reinterpret_cast<void*>(HookedMap));
    gUnmapHook = safetyhook::create_inline(vtable[15], reinterpret_cast<void*>(HookedUnmap));
    LOG_HOOK(gDrawIndexedHook, "MGS 2: Tanker sea fog: DrawIndexed");
    LOG_HOOK(gUpdateSubHook, "MGS 2: Tanker sea fog: UpdateSubresource");
    LOG_HOOK(gMapHook, "MGS 2: Tanker sea fog: Map");
    LOG_HOOK(gUnmapHook, "MGS 2: Tanker sea fog: Unmap");
    BuildBands();
}
