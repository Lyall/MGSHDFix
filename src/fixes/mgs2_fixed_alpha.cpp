#include "stdafx.h"
#include "mgs2_fixed_alpha.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

// A GS blend reads its fixed alpha as 128 = 1.0, so anything above that is a deliberate overbright.
// A D3D blend factor is 255 = 1.0 and clamps there, so a 1.99x brighten draws flat and the factor
// can't be pushed past it. Lay the excess down as a second pass over the same primitive instead.

namespace
{
    // GS ABCD byte, a | b<<2 | c<<4 | d<<6, for Cv = (A - B) * C >> 7 + D.
    constexpr uint32_t kAbcd_SrcAddDst = 0x68;  // (0,2,2,1)  Cs*fix + Cd
    constexpr uint32_t kAbcd_SrcOnly   = 0xA8;  // (0,2,2,2)  Cs*fix
    constexpr uint32_t kAbcd_DstOnly   = 0xA9;  // (1,2,2,2)  Cd*fix
    constexpr uint32_t kAbcd_SrcSubDst = 0xA4;  // (0,1,2,2) (Cs - Cd)*fix

    enum class Repair
    {
        None,
        AddSource,       // one more additive pass at (k-1), on the game's own shading
        ScaleDest,       // Cd*(k-1) on top of Cd, white source against DEST_COLOR
    };

    const char* kWhiteShader = R"(
    float4 main() : SV_Target { return float4(1.0, 1.0, 1.0, 1.0); }
    )";

    SafetyHookInline gSetAlpha{};
    SafetyHookInline gDraw{};
    SafetyHookInline gDrawIndexed{};

    ID3D11DeviceContext* gImmediate = nullptr;

    ComPtr<ID3DBlob>             psBlob;
    ComPtr<ID3D11PixelShader>    psWhite;
    ComPtr<ID3D11BlendState>     blendAddSource;   // Cs*(k-1) + Cd
    ComPtr<ID3D11BlendState>     blendScaleDest;   // Cd + Cd*(k-1)

    // Write dropped and the test widened to include equal, so the repair lands on the pixels the
    // game's own pass just wrote. Holding the original keeps its address from being recycled under
    // the key.
    struct DepthVariant
    {
        ComPtr<ID3D11DepthStencilState> original;
        ComPtr<ID3D11DepthStencilState> repair;
    };
    std::unordered_map<ID3D11DepthStencilState*, DepthVariant> gDepthVariants;

    Repair   gRepair = Repair::None;
    uint32_t gAbcd = 0;
    float    gExcess = 0.0f;   // k - 1, always in (0, 0.992]
    bool     gDestScaleRepaired = false;

    void __fastcall HookedSetAlpha(uint64_t data, int blendPass)
    {
        gRepair = Repair::None;

        const uint32_t fix = static_cast<uint32_t>(data >> 32) & 0xFF;
        if (blendPass == 0 && fix > 128 && MGS2FixedAlpha::bLoaded)
        {
            const uint32_t abcd = static_cast<uint32_t>(data) & 0xFF;
            switch (abcd)
            {
            case kAbcd_SrcAddDst:
            case kAbcd_SrcOnly:
                gRepair = Repair::AddSource;
                break;
            // The port's pass is a no-op for (1,2,2,2) and lands on Cs-Cd for (0,1,2,2); both finish
            // with the same destination multiply.
            case kAbcd_DstOnly:
            case kAbcd_SrcSubDst:
                gRepair = Repair::ScaleDest;
                break;
            default:
                break;
            }

            if (gRepair != Repair::None)
            {
                gAbcd = abcd;
                gExcess = fix / 128.0f - 1.0f;
            }
        }

        gSetAlpha.fastcall<void>(data, blendPass);
    }

    // The port's own blend for the mode, so a draw that got here on a stale state is left alone.
    bool BlendMatchesMode(ID3D11DeviceContext* ctx)
    {
        ID3D11BlendState* blend = nullptr;
        float factor[4] = {};
        UINT mask = 0;
        ctx->OMGetBlendState(&blend, factor, &mask);
        if (!blend)
        {
            return false;
        }

        D3D11_BLEND_DESC desc = {};
        blend->GetDesc(&desc);
        blend->Release();

        const auto& rt = desc.RenderTarget[0];
        if (!rt.BlendEnable || factor[0] < 0.99f)
        {
            return false;
        }

        switch (gAbcd)
        {
        case kAbcd_SrcAddDst:
            return rt.SrcBlend == D3D11_BLEND_BLEND_FACTOR && rt.DestBlend == D3D11_BLEND_ONE
                && rt.BlendOp == D3D11_BLEND_OP_ADD;
        case kAbcd_SrcOnly:
            return rt.SrcBlend == D3D11_BLEND_BLEND_FACTOR && rt.DestBlend == D3D11_BLEND_ZERO
                && rt.BlendOp == D3D11_BLEND_OP_ADD;
        case kAbcd_DstOnly:
            return rt.SrcBlend == D3D11_BLEND_ZERO && rt.DestBlend == D3D11_BLEND_BLEND_FACTOR
                && rt.BlendOp == D3D11_BLEND_OP_ADD;
        case kAbcd_SrcSubDst:
            return rt.SrcBlend == D3D11_BLEND_BLEND_FACTOR && rt.DestBlend == D3D11_BLEND_BLEND_FACTOR
                && rt.BlendOp == D3D11_BLEND_OP_SUBTRACT;
        default:
            return false;
        }
    }

    ID3D11DepthStencilState* RepairDepthState(ID3D11DeviceContext* ctx, ID3D11DepthStencilState* current)
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        if (current)
        {
            current->GetDesc(&desc);
            if (!desc.DepthEnable)
            {
                return current;
            }
        }
        else
        {
            // Nothing bound is D3D11's default: test LESS, write on.
            desc.DepthEnable = TRUE;
            desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            desc.DepthFunc = D3D11_COMPARISON_LESS;
            desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
            desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
            desc.FrontFace = { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS };
            desc.BackFace = desc.FrontFace;
        }

        if (auto it = gDepthVariants.find(current); it != gDepthVariants.end())
        {
            return it->second.repair.Get();
        }

        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        switch (desc.DepthFunc)
        {
        case D3D11_COMPARISON_LESS:    desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL; break;
        case D3D11_COMPARISON_GREATER: desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; break;
        default: break;
        }

        ComPtr<ID3D11Device> dev;
        ctx->GetDevice(dev.GetAddressOf());
        DepthVariant variant;
        variant.original = current;
        if (!dev || FAILED(dev->CreateDepthStencilState(&desc, variant.repair.GetAddressOf())))
        {
            return current;
        }

        ID3D11DepthStencilState* repair = variant.repair.Get();
        gDepthVariants.emplace(current, std::move(variant));
        return repair;
    }

    // Same geometry, straight after the game's pass, carrying only what the clamp swallowed.
    template <typename DrawFn>
    void DrawRepairPass(ID3D11DeviceContext* ctx, DrawFn&& draw)
    {
        ID3D11BlendState* oldBlend = nullptr;
        ID3D11DepthStencilState* oldDepth = nullptr;
        ID3D11PixelShader* oldPS = nullptr;
        float oldFactor[4] = {};
        UINT oldMask = 0;
        UINT oldStencilRef = 0;

        ctx->OMGetBlendState(&oldBlend, oldFactor, &oldMask);
        ctx->OMGetDepthStencilState(&oldDepth, &oldStencilRef);

        const float factor[4] = { gExcess, gExcess, gExcess, gExcess };
        ctx->OMSetBlendState(gRepair == Repair::AddSource ? blendAddSource.Get() : blendScaleDest.Get(),
            factor, oldMask);

        if (ID3D11DepthStencilState* repairDepth = RepairDepthState(ctx, oldDepth); repairDepth != oldDepth)
        {
            ctx->OMSetDepthStencilState(repairDepth, oldStencilRef);
        }

        if (gRepair == Repair::ScaleDest)
        {
            ctx->PSGetShader(&oldPS, nullptr, nullptr);
            ctx->PSSetShader(psWhite.Get(), nullptr, 0);
        }

        draw();

        ctx->OMSetBlendState(oldBlend, oldFactor, oldMask);
        ctx->OMSetDepthStencilState(oldDepth, oldStencilRef);
        if (gRepair == Repair::ScaleDest)
        {
            ctx->PSSetShader(oldPS, nullptr, 0);
        }

        if (oldBlend) oldBlend->Release();
        if (oldDepth) oldDepth->Release();
        if (oldPS)    oldPS->Release();
    }

    template <typename DrawFn>
    void DispatchDraw(ID3D11DeviceContext* ctx, DrawFn&& draw)
    {
        draw();

        if (gRepair == Repair::None || ctx != gImmediate || !BlendMatchesMode(ctx))
        {
            return;
        }

        if (gRepair == Repair::ScaleDest)
        {
            gDestScaleRepaired = true;
        }

        DrawRepairPass(ctx, draw);
    }

    void __stdcall HookedDraw(ID3D11DeviceContext* ctx, UINT vertexCount, UINT startVertex)
    {
        DispatchDraw(ctx, [&] { gDraw.stdcall<void>(ctx, vertexCount, startVertex); });
    }

    void __stdcall HookedDrawIndexed(ID3D11DeviceContext* ctx, UINT indexCount, UINT startIndex, INT baseVertex)
    {
        DispatchDraw(ctx, [&] { gDrawIndexed.stdcall<void>(ctx, indexCount, startIndex, baseVertex); });
    }
}

bool MGS2FixedAlpha::ConsumeDestScaleRepair()
{
    const bool repaired = gDestScaleRepaired;
    gDestScaleRepaired = false;
    return repaired;
}

void MGS2FixedAlpha::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Fixed Alpha: Disabled in config, skipping.");
        return;
    }

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("MGS 2: Fixed Alpha: Failed to get D3DCompile.");
        return;
    }

    ComPtr<ID3DBlob> err;
    const HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kWhiteShader, strlen(kWhiteShader), nullptr, nullptr,
        nullptr, "main", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS 2: Fixed Alpha: Failed to compile pixel shader: {}",
            err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        return;
    }

    uint8_t* setAlpha = Memory::PatternScan(baseModule,
        "48 83 EC 48 48 8B C1 48 89 0D ?? ?? ?? ?? 48 C1 E8 20",
        "MGS 2: Fixed Alpha: bp\\shared\\BP_RenderGS.cpp -> BP_GS_SetAlpha_Default()");
    if (!setAlpha)
    {
        return;
    }

    gSetAlpha = safetyhook::create_inline(setAlpha, reinterpret_cast<void*>(HookedSetAlpha));
    LOG_HOOK(gSetAlpha, "MGS 2: Fixed Alpha: bp\\shared\\BP_RenderGS.cpp -> BP_GS_SetAlpha_Default()")
}

void MGS2FixedAlpha::OnDeviceReady()
{
    if (!(eGameType & MGS2) || !bEnabled || bLoaded || !gSetAlpha)
    {
        return;
    }

    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    ID3D11DeviceContext* ctx = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!dev || !ctx || !psBlob)
    {
        return;
    }

    if (FAILED(dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, psWhite.GetAddressOf())))
    {
        spdlog::error("MGS 2: Fixed Alpha: Failed to create pixel shader.");
        return;
    }
    psBlob.Reset();

    // Alpha stays with the game's own pass, so both of these only touch colour.
    auto makeBlend = [&](D3D11_BLEND src, D3D11_BLEND dst, ComPtr<ID3D11BlendState>& out) {
        D3D11_BLEND_DESC bd = {};
        auto& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = src;
        rt.DestBlend = dst;
        rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ZERO;
        rt.DestBlendAlpha = D3D11_BLEND_ONE;
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
        return SUCCEEDED(dev->CreateBlendState(&bd, out.GetAddressOf()));
        };

    if (!makeBlend(D3D11_BLEND_BLEND_FACTOR, D3D11_BLEND_ONE, blendAddSource)
        || !makeBlend(D3D11_BLEND_DEST_COLOR, D3D11_BLEND_BLEND_FACTOR, blendScaleDest))
    {
        spdlog::error("MGS 2: Fixed Alpha: Failed to create blend states.");
        return;
    }

    gImmediate = ctx;

    void** vtable = *reinterpret_cast<void***>(ctx);
    gDrawIndexed = safetyhook::create_inline(vtable[12], reinterpret_cast<void*>(HookedDrawIndexed));
    LOG_HOOK(gDrawIndexed, "MGS 2: Fixed Alpha: ID3D11DeviceContext::DrawIndexed")
    gDraw = safetyhook::create_inline(vtable[13], reinterpret_cast<void*>(HookedDraw));
    LOG_HOOK(gDraw, "MGS 2: Fixed Alpha: ID3D11DeviceContext::Draw")

    if (!gDraw || !gDrawIndexed)
    {
        return;
    }

    bLoaded = true;
    spdlog::info("MGS 2: Fixed Alpha initialized.");
}
