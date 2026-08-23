#include "stdafx.h"
#include "mgs3_glow_overbright.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

#include <d3d11.h>

// The PS2 blended in whole numbers and threw the fraction away. BP keeps it, in float,
// so light the hardware rounded down to nothing still shows up here. Do the same sums the
// same way, in a replacement pixel shader.

namespace
{
    const char* kQuantPS =
    "// The GS's own integer maths, in place of Prim.fx's float version. Returns the colour with\n"
    "// the alpha already multiplied in, so the caller sets the source blend factor to ONE.\n"
    "\n"
    "Texture2D    tTextureSampler : register(t0);\n"
    "SamplerState sTextureSampler : register(s0);\n"
    "\n"
    "cbuffer Globals : register(b0)\n"
    "{\n"
    "    float4 pad0[16];              // 0   .. 255\n"
    "    int    gPs_AlphaTestEnable;   // 256\n"
    "    float3 pad1;\n"
    "    int    gPs_AlphaTestType;     // 272\n"
    "    float3 pad2;\n"
    "    float  gPs_AlphaTestRefValue; // 288\n"
    "    float3 pad3;\n"
    "    float  gPs_AlphaTestThreshold;// 304\n"
    "    float3 pad4;\n"
    "    int    gPs_MultiSampleMaskEnable; // 320\n"
    "    float3 pad5;\n"
    "    int    gPs_MultiSampleMask;   // 336\n"
    "    float3 pad6;\n"
    "    float4 pad7[17];              // 352 .. 623\n"
    "    float4 gPS_FogColor;          // 624\n"
    "};\n"
    "\n"
    "struct PIn\n"
    "{\n"
    "    float4 vpos : SV_POSITION;\n"
    "    float4 uv   : TEXCOORD0;      // .xy uv, .w fog ratio\n"
    "    float4 col  : TEXCOORD1;      // = 2 * (GS vertex colour / 255)\n"
    "};\n"
    "\n"
    "float4 main(PIn i) : SV_TARGET\n"
    "{\n"
    "    float4 t = tTextureSampler.Sample(sTextureSampler, i.uv.xy);\n"
    "\n"
    "    // recover the GS's own 8-bit integers. t is UNORM8 so *255 is exact; i.col arrived as a\n"
    "    // packed byte /255 then doubled in the VS, so *127.5 undoes both.\n"
    "    int3 Ct = (int3)(t.rgb * 255.0f + 0.5f);\n"
    "    int  At = (int) (t.a   * 255.0f + 0.5f);\n"
    "    int3 Cv = (int3)(i.col.rgb * 127.5f + 0.5f);\n"
    "    int  Av = (int) (i.col.w   * 127.5f + 0.5f);\n"
    "\n"
    "    // TFX=0 MODULATE, COLCLAMP=1\n"
    "    int3 Cs = min(255, (Ct * Cv) >> 7);\n"
    "    // GS blend alpha\n"
    "    int  As = (At * Av) >> 7;\n"
    "\n"
    "    // fog, in the same 0..255 domain the GS works in (FOGCOL is a byte triple)\n"
    "    float3 fogged = lerp((float3)Cs, gPS_FogColor.rgb * 255.0f, i.uv.w);\n"
    "    int3   Csf    = (int3)(fogged + 0.5f);\n"
    "\n"
    "    // the blend's own >>7\n"
    "    int3 addend = (Csf * As) >> 7;\n"
    "\n"
    "    float4 o;\n"
    "    o.rgb = (float3)addend * (1.0f / 255.0f);\n"
    "    o.a   = saturate(t.a * i.col.w) * 2.0f;      // identical to BP's r1.w\n"
    "\n"
    "    // BP's runtime alpha-test emulation, on the same value it used\n"
    "    if (gPs_AlphaTestEnable)\n"
    "    {\n"
    "        bool keep = true;\n"
    "        float a = o.a;\n"
    "        switch (gPs_AlphaTestType)\n"
    "        {\n"
    "            case 0: keep = false; break;\n"
    "            case 1: keep = (a <  gPs_AlphaTestRefValue - gPs_AlphaTestThreshold); break;\n"
    "            case 2: keep = (abs(a - gPs_AlphaTestRefValue) <  gPs_AlphaTestThreshold); break;\n"
    "            case 3: keep = (a <  gPs_AlphaTestRefValue + gPs_AlphaTestThreshold); break;\n"
    "            case 4: keep = !(a <  gPs_AlphaTestRefValue + gPs_AlphaTestThreshold); break;\n"
    "            case 5: keep = !(abs(a - gPs_AlphaTestRefValue) <  gPs_AlphaTestThreshold); break;\n"
    "            case 6: keep = !(a <  gPs_AlphaTestRefValue - gPs_AlphaTestThreshold); break;\n"
    "            default: keep = true; break;\n"
    "        }\n"
    "        if (!keep) { discard; }\n"
    "    }\n"
    "    return o;\n"
    "}\n"
        ;

    ID3D11DeviceContext* gImmediate = nullptr;   // vtable hooks catch every context

    SafetyHookInline gDrawIndexed{};
    SafetyHookInline gPSSetSRV{};
    SafetyHookInline gIASetVB{};

    ID3D11Buffer* gVB[3] = {};
    UINT gVBStride[3] = {};
    UINT gVBOffset[3] = {};

    ComPtr<ID3D11Buffer> gWhite;              // one float4 = (1,1,1,1), bound with stride 0

    std::atomic<bool> gPendingDecal{ false };
    bool gCurDecal = false;
    bool gReentrant = false;

    bool gCurSrcIsRT = false;                 // post passes sample a render target, sheets do not
    ComPtr<ID3D11Texture2D> gCurCand;         // slot 0 holds a 64x64 sheet, not yet looked at
    D3D11_TEXTURE2D_DESC gCurCandDesc{};

    // TFX=1 means the texel goes down as it is, no vertex colour. Read it where BP reads it,
    // from TEX0 at +0x40, and check the field still looks like a TEX0.
    bool IsAuthoredDecal(const uint8_t* rec)
    {
        const uint64_t tex0 = *reinterpret_cast<const uint64_t*>(rec + 0x40);
        const unsigned psm = static_cast<unsigned>((tex0 >> 20) & 0x3F);
        const unsigned tw  = static_cast<unsigned>((tex0 >> 26) & 0xF);
        const unsigned th  = static_cast<unsigned>((tex0 >> 30) & 0xF);
        const bool isTex0 = tw >= 2 && tw <= 10 && th >= 2 && th <= 10 &&
            (psm == 0 || psm == 1 || psm == 2 || psm == 0x13 || psm == 0x14 ||
             psm == 0x1B || psm == 0x24 || psm == 0x2C);
        return isTex0 && ((tex0 >> 35) & 3) == 1;
    }

    // Our colour arrives with the alpha already multiplied in, so the blend needs a source
    // factor of ONE. The alpha side is left exactly as BP had it.
    SafetyHookInline gCreatePS{};
    SafetyHookInline gCreateDevice{};
    SafetyHookInline gCreateDeviceSwap{};
    SafetyHookInline gPSSetShader{};
    ComPtr<ID3D11PixelShader> gQuantPS;
    // Built on the loader thread, read on the render thread - store the pointer before the count.
    std::mutex gShaderAdd;
    ID3D11PixelShader* gPrimPS[1024] = {};    // pixel shaders carrying the alpha double
    std::atomic<int> gPrimPSCount{ 0 };
    ID3D11PixelShader* gCurPS = nullptr;
    ID3D11VertexShader* gPrimVS[1024] = {};   // and the prim vertex shaders that feed them
    std::atomic<int> gPrimVSCount{ 0 };
    ID3D11VertexShader* gCurVS = nullptr;
    SafetyHookInline gCreateVS{};
    SafetyHookInline gVSSetShader{};
    // Same as the sheet cache: hold the key, or a recycled address returns the wrong twin.
    struct BlendTwin { ComPtr<ID3D11BlendState> src, premul; };
    BlendTwin gBlendTwins[32] = {};
    int gBlendTwinCount = 0;

    bool IsPrimVS(ID3D11VertexShader* vs)
    {
        const int n = gPrimVSCount.load(std::memory_order_acquire);
        for (int i = 0; i < n; i++) { if (gPrimVS[i] == vs) { return true; } }
        return false;
    }

    bool IsPrimPS(ID3D11PixelShader* ps)
    {
        const int n = gPrimPSCount.load(std::memory_order_acquire);
        for (int i = 0; i < n; i++) { if (gPrimPS[i] == ps) { return true; } }
        return false;
    }

    // Spotting an effect draw. Doubling the alpha is not enough on its own - most of the game's
    // shaders do that - but effect vertices carry integer uvs.
    const uint8_t kAlphaDouble[16] = { 0x00,0x00,0x80,0x3F, 0x00,0x00,0x80,0x3F,
                                       0x00,0x00,0x80,0x3F, 0x00,0x00,0x00,0x40 };

    // Walk the DXBC chunk table to ISGN and ask whether any TEXCOORD input is integer-typed.
    bool HasIntegerTexcoord(const uint8_t* b, size_t len)
    {
        if (len < 32 || memcmp(b, "DXBC", 4) != 0) { return false; }
        const uint32_t chunks = *reinterpret_cast<const uint32_t*>(b + 28);
        if (32 + chunks * 4 > len) { return false; }
        for (uint32_t i = 0; i < chunks; i++)
        {
            const uint32_t off = *reinterpret_cast<const uint32_t*>(b + 32 + i * 4);
            if (off + 16 > len || memcmp(b + off, "ISGN", 4) != 0) { continue; }
            const uint8_t* sig = b + off + 8;                 // past tag and size
            const uint32_t count = *reinterpret_cast<const uint32_t*>(sig);
            for (uint32_t e = 0; e < count; e++)
            {
                const uint8_t* el = sig + 8 + e * 24;
                if (el + 24 > b + len) { break; }
                const uint32_t nameOff = *reinterpret_cast<const uint32_t*>(el);
                const uint32_t compType = *reinterpret_cast<const uint32_t*>(el + 12);
                const char* name = reinterpret_cast<const char*>(sig + nameOff);
                if (name < reinterpret_cast<const char*>(b + len) &&
                    strncmp(name, "TEXCOORD", 8) == 0 && compType != 3)   // 3 = float32
                {
                    return true;
                }
            }
        }
        return false;
    }

    HRESULT __stdcall HookedCreatePixelShader(ID3D11Device* device, const void* bytecode,
        SIZE_T length, ID3D11ClassLinkage* linkage, ID3D11PixelShader** out)
    {
        const HRESULT hr = gCreatePS.call<HRESULT>(device, bytecode, length, linkage, out);
        if (SUCCEEDED(hr) && out && *out && bytecode && length > 36)
        {
            const uint8_t* b = static_cast<const uint8_t*>(bytecode);
            if (std::search(b, b + length, kAlphaDouble, kAlphaDouble + 16) != b + length)
            {
                std::scoped_lock lk(gShaderAdd);
                const int n = gPrimPSCount.load(std::memory_order_relaxed);
                if (n < 1024)
                {
                    gPrimPS[n] = *out;
                    gPrimPSCount.store(n + 1, std::memory_order_release);
                }
                else if (n == 1024) { spdlog::warn("MGS 3: Glow Overbright: effect pixel shader "
                                                   "list full, some draws will be missed."); }
            }
        }
        return hr;
    }

    HRESULT __stdcall HookedCreateVertexShader(ID3D11Device* device, const void* bytecode,
        SIZE_T length, ID3D11ClassLinkage* linkage, ID3D11VertexShader** out)
    {
        const HRESULT hr = gCreateVS.call<HRESULT>(device, bytecode, length, linkage, out);
        if (SUCCEEDED(hr) && out && *out && bytecode && length > 36 &&
            HasIntegerTexcoord(static_cast<const uint8_t*>(bytecode), length))
        {
            std::scoped_lock lk(gShaderAdd);
            const int n = gPrimVSCount.load(std::memory_order_relaxed);
            if (n < 1024)
            {
                gPrimVS[n] = *out;
                gPrimVSCount.store(n + 1, std::memory_order_release);
            }
            else if (n == 1024) { spdlog::warn("MGS 3: Glow Overbright: effect vertex shader "
                                               "list full, some draws will be missed."); }
        }
        return hr;
    }

    void __stdcall HookedVSSetShader(ID3D11DeviceContext* ctx, ID3D11VertexShader* vs,
        ID3D11ClassInstance* const* inst, UINT num)
    {
        if (ctx != gImmediate) { gVSSetShader.call<void>(ctx, vs, inst, num); return; }
        gCurVS = vs;
        gVSSetShader.call<void>(ctx, vs, inst, num);
    }

    void __stdcall HookedPSSetShader(ID3D11DeviceContext* ctx, ID3D11PixelShader* ps,
        ID3D11ClassInstance* const* inst, UINT num)
    {
        if (ctx != gImmediate) { gPSSetShader.call<void>(ctx, ps, inst, num); return; }
        gCurPS = ps;
        gPSSetShader.call<void>(ctx, ps, inst, num);
    }

    bool IsPrimDraw(ID3D11VertexShader* vs, ID3D11PixelShader* ps)
    {
        bool okVS = false, okPS = false;
        const int nvs = gPrimVSCount.load(std::memory_order_acquire);
        const int nps = gPrimPSCount.load(std::memory_order_acquire);
        for (int i = 0; i < nvs && !okVS; i++) { okVS = (gPrimVS[i] == vs); }
        for (int i = 0; i < nps && !okPS; i++) { okPS = (gPrimPS[i] == ps); }
        return okVS && okPS;
    }

    // The same blend with the rgb source factor set to ONE, cached per state object.
    ID3D11BlendState* PremultipliedTwin(ID3D11DeviceContext* ctx, ID3D11BlendState* src,
        const D3D11_BLEND_DESC& desc)
    {
        for (int i = 0; i < gBlendTwinCount; i++)
        {
            if (gBlendTwins[i].src.Get() == src) { return gBlendTwins[i].premul.Get(); }
        }
        if (gBlendTwinCount >= 32) { return nullptr; }
        BlendTwin& slot = gBlendTwins[gBlendTwinCount++];
        slot.src = src;
        D3D11_BLEND_DESC d = desc;
        for (int rt = 0; rt < 8; rt++)
        {
            if (d.RenderTarget[rt].SrcBlend == D3D11_BLEND_SRC_ALPHA)
            {
                d.RenderTarget[rt].SrcBlend = D3D11_BLEND_ONE;
            }
            if (!d.IndependentBlendEnable) { break; }
        }
        ComPtr<ID3D11Device> dev;
        ctx->GetDevice(dev.GetAddressOf());
        if (dev) { dev->CreateBlendState(&d, slot.premul.GetAddressOf()); }
        return slot.premul.Get();
    }

    void InstallShaderHook(ID3D11Device* dev)
    {
        if (!dev || gCreatePS) { return; }
        void** dvt = *reinterpret_cast<void***>(dev);
        gCreatePS = safetyhook::create_inline(dvt[15], reinterpret_cast<void*>(HookedCreatePixelShader));
        LOG_HOOK(gCreatePS, "MGS 3: Glow Overbright: CreatePixelShader")
        gCreateVS = safetyhook::create_inline(dvt[12], reinterpret_cast<void*>(HookedCreateVertexShader));
        LOG_HOOK(gCreateVS, "MGS 3: Glow Overbright: CreateVertexShader")
    }

    HRESULT WINAPI HookedCreateDevice(IDXGIAdapter* adapter, D3D_DRIVER_TYPE type, HMODULE sw,
        UINT flags, const D3D_FEATURE_LEVEL* levels, UINT numLevels, UINT sdk,
        ID3D11Device** outDev, D3D_FEATURE_LEVEL* outLevel, ID3D11DeviceContext** outCtx)
    {
        const HRESULT hr = gCreateDevice.call<HRESULT>(adapter, type, sw, flags, levels,
            numLevels, sdk, outDev, outLevel, outCtx);
        if (SUCCEEDED(hr) && outDev) { InstallShaderHook(*outDev); }
        return hr;
    }

    HRESULT WINAPI HookedCreateDeviceAndSwapChain(IDXGIAdapter* adapter, D3D_DRIVER_TYPE type,
        HMODULE sw, UINT flags, const D3D_FEATURE_LEVEL* levels, UINT numLevels, UINT sdk,
        const DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** outSwap, ID3D11Device** outDev,
        D3D_FEATURE_LEVEL* outLevel, ID3D11DeviceContext** outCtx)
    {
        const HRESULT hr = gCreateDeviceSwap.call<HRESULT>(adapter, type, sw, flags, levels,
            numLevels, sdk, desc, outSwap, outDev, outLevel, outCtx);
        if (SUCCEEDED(hr) && outDev) { InstallShaderHook(*outDev); }
        return hr;
    }

    // Why we look at the pixels to spot this one texture.
    //
    // For decals we can just ask, because Konami wrote down which textures skip the lighting.
    // Nobody wrote down "round this one off" - a PS2 did that to every texture, so there was
    // nothing to write. The PC only tells us the size, and 1189 textures here are 64x64. The
    // texture's own id would settle it, but the id we can get hold of turns out to belong to
    // whatever was bound before this.
    //
    // One sheet only: rounding off makes the picture step in whole numbers. On a PS2 a step was
    // about a pixel, so nobody saw it. Ours are dozens of pixels wide and show up as banding.
    //
    // This function is the question. SheetWantsQuantising does the slow part, fetching the
    // pixels. Another sheet means changing this one and nothing else.
    bool IsGSQuantSheet(const D3D11_TEXTURE2D_DESC& d, const bool* seen)
    {
        // The sheet's own 16 alpha levels. Two s052a shadow overlays are also 64x64 with 16
        // levels topping out at 57, so the count and the max are not enough on their own.
        static constexpr uint8_t kRaySheet[16] =
            { 0, 2, 6, 10, 13, 16, 18, 19, 21, 24, 27, 30, 34, 39, 45, 57 };
        if (d.Width != 64 || d.Height != 64) { return false; }
        int levels = 0;
        for (int a = 0; a < 256; a++) { if (seen[a]) { levels++; } }
        if (levels != 16) { return false; }
        for (uint8_t a : kRaySheet) { if (!seen[a]) { return false; } }
        return true;
    }

    // Hold a reference to the texture we key on: a freed one's address gets handed back, and
    // the cached answer would then apply to something else.
    struct SheetVerdict { ComPtr<ID3D11Texture2D> tex; bool quantise; };
    SheetVerdict gSheets[256] = {};
    int gSheetCount = 0;
    int gSheetNext = 0;

    // Fetches the pixels for the question above. Reading a texture back waits on the GPU, so we
    // remember the answer - the no as well, or the same sheet pays again on every draw.
    bool SheetWantsQuantising(ID3D11DeviceContext* ctx, ID3D11Texture2D* tex,
        const D3D11_TEXTURE2D_DESC& d)
    {
        if (!tex) { return false; }
        for (int i = 0; i < gSheetCount; i++)
        {
            if (gSheets[i].tex.Get() == tex) { return gSheets[i].quantise; }
        }
        if (d.Format != DXGI_FORMAT_B8G8R8A8_UNORM && d.Format != DXGI_FORMAT_R8G8B8A8_UNORM)
        {
            return false;
        }
        // The game drops and rebuilds these textures as it goes, so old answers go stale and
        // the list would otherwise fill up and stop taking new ones. Reuse the oldest slot,
        // which also lets go of a texture the game has finished with.
        if (gSheetCount < 256) { gSheetCount++; }
        SheetVerdict& slot = gSheets[gSheetNext];
        gSheetNext = (gSheetNext + 1) % 256;
        slot.tex = tex;                 // AddRef: the address cannot be recycled under us
        slot.quantise = false;

        ComPtr<ID3D11Device> dev;
        ctx->GetDevice(dev.GetAddressOf());
        if (!dev) { return false; }
        D3D11_TEXTURE2D_DESC sd = d;
        sd.MipLevels = 1; sd.ArraySize = 1; sd.Usage = D3D11_USAGE_STAGING;
        sd.BindFlags = 0; sd.MiscFlags = 0;
        sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        sd.SampleDesc.Count = 1; sd.SampleDesc.Quality = 0;
        ComPtr<ID3D11Texture2D> stage;
        if (FAILED(dev->CreateTexture2D(&sd, nullptr, stage.GetAddressOf()))) { return false; }
        ctx->CopySubresourceRegion(stage.Get(), 0, 0, 0, 0, tex, 0, nullptr);

        D3D11_MAPPED_SUBRESOURCE m{};
        if (FAILED(ctx->Map(stage.Get(), 0, D3D11_MAP_READ, 0, &m))) { return false; }
        bool seen[256] = {};
        for (UINT y = 0; y < 64; y++)
        {
            const uint8_t* row = static_cast<const uint8_t*>(m.pData) + static_cast<size_t>(y) * m.RowPitch;
            for (UINT x = 0; x < 64; x++)
            {
                seen[row[x * 4 + 3]] = true;
            }
        }
        ctx->Unmap(stage.Get(), 0);

        slot.quantise = IsGSQuantSheet(d, seen);
        return slot.quantise;
    }

    void __stdcall HookedPSSetShaderResources(ID3D11DeviceContext* ctx, UINT slot, UINT num,
        ID3D11ShaderResourceView* const* views)
    {
        if (ctx == gImmediate && slot == 0)
        {
            gCurSrcIsRT = false;
            gCurCand.Reset();
            // clear as we consume, or the next mesh inherits this verdict
            gCurDecal = (num > 0 && views && views[0]) &&
                gPendingDecal.exchange(false, std::memory_order_relaxed);

            if (num > 0 && views && views[0])
            {
                ComPtr<ID3D11Resource> res;
                views[0]->GetResource(res.GetAddressOf());
                ComPtr<ID3D11Texture2D> tex;
                if (res && SUCCEEDED(res.As(&tex)) && tex)
                {
                    D3D11_TEXTURE2D_DESC d{};
                    tex->GetDesc(&d);
                    gCurSrcIsRT = (d.BindFlags & D3D11_BIND_RENDER_TARGET) != 0;
                    // Only note it here. Reading the pixels waits on the GPU, so that is left
                    // until we know the draw is an effect - most 64x64 textures never are one.
                    if (!gCurSrcIsRT && d.Width == 64 && d.Height == 64)
                    {
                        gCurCand = tex;
                        gCurCandDesc = d;
                    }
                }
            }
        }
        gPSSetSRV.call<void>(ctx, slot, num, views);
    }

    void __stdcall HookedIASetVertexBuffers(ID3D11DeviceContext* ctx, UINT startSlot, UINT num,
        ID3D11Buffer* const* buffers, const UINT* strides, const UINT* offsets)
    {
        for (UINT i = 0; ctx == gImmediate && i < num && startSlot + i < 3; i++)
        {
            gVB[startSlot + i] = buffers ? buffers[i] : nullptr;
            gVBStride[startSlot + i] = strides ? strides[i] : 0;
            gVBOffset[startSlot + i] = offsets ? offsets[i] : 0;
        }
        gIASetVB.call<void>(ctx, startSlot, num, buffers, strides, offsets);
    }

    void __stdcall HookedDrawIndexed(ID3D11DeviceContext* ctx, UINT indexCount,
        UINT startIndex, INT baseVertex)
    {
        if (ctx != gImmediate)
        {
            gDrawIndexed.call<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }
        if (gReentrant)
        {
            gDrawIndexed.call<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }

        // an effect draw, on a 64x64 sheet
        if (gQuantPS && gCurCand && IsPrimDraw(gCurVS, gCurPS))
        {
            ComPtr<ID3D11BlendState> bs;
            float bf[4]; UINT mask = 0;
            ctx->OMGetBlendState(bs.GetAddressOf(), bf, &mask);
            D3D11_BLEND_DESC bd{};
            if (bs) { bs->GetDesc(&bd); }
            if (bs && bd.RenderTarget[0].BlendEnable &&
                bd.RenderTarget[0].SrcBlend == D3D11_BLEND_SRC_ALPHA &&
                SheetWantsQuantising(ctx, gCurCand.Get(), gCurCandDesc))
            {
                ID3D11BlendState* const premul = PremultipliedTwin(ctx, bs.Get(), bd);
                if (premul)
                {
                    gPSSetShader.call<void>(ctx, gQuantPS.Get(), nullptr, 0u);
                    ctx->OMSetBlendState(premul, bf, mask);
                    gReentrant = true;
                    gDrawIndexed.call<void>(ctx, indexCount, startIndex, baseVertex);
                    gReentrant = false;
                    ctx->OMSetBlendState(bs.Get(), bf, mask);
                    gPSSetShader.call<void>(ctx, gCurPS, nullptr, 0u);
                    return;
                }
            }
        }

        if (!gCurDecal || !gWhite)
        {
            gDrawIndexed.call<void>(ctx, indexCount, startIndex, baseVertex);
            return;
        }

        // slot 2 is the stream PS1011 multiplies in; stride 0 feeds every vertex the same 1.0
        ID3D11Buffer* white = gWhite.Get();
        const UINT zeroStride = 0;
        const UINT zeroOffset = 0;
        gIASetVB.call<void>(ctx, 2u, 1u, &white, &zeroStride, &zeroOffset);
        gDrawIndexed.call<void>(ctx, indexCount, startIndex, baseVertex);
        ID3D11Buffer* prev = gVB[2];
        gIASetVB.call<void>(ctx, 2u, 1u, &prev, &gVBStride[2], &gVBOffset[2]);
    }
}

void MGS3GlowOverbright::Initialize()
{
    if (!(eGameType & MGS3))
    {
        return;
    }
    if (!bEnabled)
    {
        spdlog::info("MGS 3: Glow Overbright: Disabled via config, skipping.");
        return;
    }

    // The shader list has to exist before the game builds its shaders, which is long before
    // the first Present - so catch the device at the d3d11 export.
    if (HMODULE d3d11 = LoadLibraryW(L"d3d11.dll"))
    {
        if (void* proc = reinterpret_cast<void*>(GetProcAddress(d3d11, "D3D11CreateDevice")))
        {
            gCreateDevice = safetyhook::create_inline(proc, reinterpret_cast<void*>(HookedCreateDevice));
            LOG_HOOK(gCreateDevice, "MGS 3: Glow Overbright: D3D11CreateDevice")
        }
        if (void* proc = reinterpret_cast<void*>(GetProcAddress(d3d11, "D3D11CreateDeviceAndSwapChain")))
        {
            gCreateDeviceSwap = safetyhook::create_inline(proc, reinterpret_cast<void*>(HookedCreateDeviceAndSwapChain));
            LOG_HOOK(gCreateDeviceSwap, "MGS 3: Glow Overbright: D3D11CreateDeviceAndSwapChain")
        }
    }

    // [rbx+0x20] is the texture record going down
    MAKE_HOOK_MID(baseModule,
        "48 8B 5B 20 48 85 DB 74 ?? 48 89 1D",
        "MGS 3: Glow Overbright: packet texture bind", {
        const uint8_t* tex = *reinterpret_cast<uint8_t**>(ctx.rbx + 0x20);
        if (tex)
        {
            gPendingDecal.store(IsAuthoredDecal(tex), std::memory_order_relaxed);
        }
    });
}

void MGS3GlowOverbright::OnDeviceReady(ID3D11Device* device)
{
    if (!(eGameType & MGS3) || !bEnabled || !device || gDrawIndexed)
    {
        return;
    }

    const float kWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(kWhite);
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = kWhite;
    if (FAILED(device->CreateBuffer(&bd, &init, gWhite.GetAddressOf())))
    {
        spdlog::error("MGS 3: Glow Overbright: could not make the neutral colour stream.");
        return;
    }

    ComPtr<ID3D11DeviceContext> ctx;
    device->GetImmediateContext(ctx.GetAddressOf());
    gImmediate = ctx.Get();
    if (!ctx)
    {
        return;
    }
    if (g_D3D11Hooks.D3DCompileFunc)
    {
        ComPtr<ID3DBlob> blob, err;
        const HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kQuantPS, strlen(kQuantPS), nullptr,
            nullptr, nullptr, "main", "ps_5_0", 0, 0,
            blob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
        if (FAILED(hr) || !blob)
        {
            spdlog::error("MGS 3: Glow Overbright: quantise shader failed to compile: {}",
                err ? static_cast<const char*>(err->GetBufferPointer()) : "unknown");
        }
        else if (FAILED(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(),
                                                  nullptr, gQuantPS.GetAddressOf())))
        {
            spdlog::error("MGS 3: Glow Overbright: quantise shader rejected by the device.");
        }
    }

    void** vt = *reinterpret_cast<void***>(ctx.Get());
    gPSSetSRV = safetyhook::create_inline(vt[8], reinterpret_cast<void*>(HookedPSSetShaderResources));
    LOG_HOOK(gPSSetSRV, "MGS 3: Glow Overbright: PSSetShaderResources")
    gIASetVB = safetyhook::create_inline(vt[18], reinterpret_cast<void*>(HookedIASetVertexBuffers));
    LOG_HOOK(gIASetVB, "MGS 3: Glow Overbright: IASetVertexBuffers")
    gDrawIndexed = safetyhook::create_inline(vt[12], reinterpret_cast<void*>(HookedDrawIndexed));
    LOG_HOOK(gDrawIndexed, "MGS 3: Glow Overbright: DrawIndexed")
    gPSSetShader = safetyhook::create_inline(vt[9], reinterpret_cast<void*>(HookedPSSetShader));
    LOG_HOOK(gPSSetShader, "MGS 3: Glow Overbright: PSSetShader")
    gVSSetShader = safetyhook::create_inline(vt[11], reinterpret_cast<void*>(HookedVSSetShader));
    LOG_HOOK(gVSSetShader, "MGS 3: Glow Overbright: VSSetShader")

    InstallShaderHook(device);
}
