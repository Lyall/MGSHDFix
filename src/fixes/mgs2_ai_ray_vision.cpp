#include "stdafx.h"

#include "mgs2_ai_ray_vision.hpp"
#include "d3d11_api.hpp"
#include "common.hpp"
#include "gamevars.hpp"

#include "logging.hpp"

// The red field in RAY's view uses a GS source alpha of 0xFF. GS treats 128 as 1.0, so that
// asks for about 2x, which D3D11 clamps back to 1.0 on a UNORM target. The blend then turns
// into an opaque fill and hides the scene behind it.

namespace
{
    // the gain has to ride in rgb; D3D11 clamps blend factors to the target range too
    const char* kOverbrightShader = R"(
    struct PSIn
    {
        float4 pos : SV_POSITION;
        float4 col : TEXCOORD1;
    };
    float4 main(PSIn i) : SV_Target
    {
        float k = i.col.a * (255.0 / 128.0);
        return float4(saturate(i.col.rgb * k), saturate(2.0 - k));
    }
    )";

    constexpr UINT  kMaxCandidates   = 8;
    constexpr UINT  kVertexStride    = 32;
    constexpr UINT  kQuadVerts       = 4;
    constexpr UINT  kSlotBytes       = kVertexStride * kQuadVerts;
    constexpr UINT  kColourOffset    = 16;
    constexpr float kOverbrightAlpha = 128.5f / 255.0f;

    ComPtr<ID3DBlob>            psBlob;
    ComPtr<ID3D11PixelShader>   psOverbright;
    ComPtr<ID3D11BlendState>    blendOverbright;
    ComPtr<ID3D11Buffer>        vertexProbe[2];

    safetyhook::InlineHook Draw_hook;
    safetyhook::InlineHook Die512_hook;

    bool bArmed        = false;
    int  iPlateOrdinal = -1;
    UINT uCandidate    = 0;
    UINT uWriteSlot    = 0;
    UINT uProbed[2]    = { 0, 0 };

    // a 4-vertex src-over strip matches most of the HUD too, so the plate is picked out by its alpha
    bool IsCandidate(ID3D11DeviceContext* context, UINT vertexCount)
    {
        if (vertexCount != kQuadVerts)
        {
            return false;
        }

        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        context->IAGetPrimitiveTopology(&topology);
        if (topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP)
        {
            return false;
        }

        ID3D11BlendState* blend = nullptr;
        float factor[4] = {};
        UINT mask = 0;
        context->OMGetBlendState(&blend, factor, &mask);
        if (!blend)
        {
            return false;
        }

        D3D11_BLEND_DESC desc = {};
        blend->GetDesc(&desc);
        blend->Release();

        const auto& rt = desc.RenderTarget[0];
        return rt.BlendEnable
            && rt.SrcBlend == D3D11_BLEND_SRC_ALPHA
            && rt.DestBlend == D3D11_BLEND_INV_SRC_ALPHA
            && rt.BlendOp == D3D11_BLEND_OP_ADD;
    }

    void ProbeVertices(ID3D11DeviceContext* context, UINT startVertexLocation)
    {
        if (!vertexProbe[uWriteSlot] || uProbed[uWriteSlot] >= kMaxCandidates)
        {
            return;
        }

        ID3D11Buffer* vb = nullptr;
        UINT stride = 0;
        UINT offset = 0;
        context->IAGetVertexBuffers(0, 1, &vb, &stride, &offset);
        if (!vb)
        {
            return;
        }

        if (stride == kVertexStride)
        {
            D3D11_BOX box = {};
            box.left = offset + startVertexLocation * stride;
            box.right = box.left + kSlotBytes;
            box.bottom = 1;
            box.back = 1;

            context->CopySubresourceRegion(vertexProbe[uWriteSlot].Get(), 0, uProbed[uWriteSlot] * kSlotBytes, 0, 0, vb, 0, &box);
            uProbed[uWriteSlot]++;
        }

        vb->Release();
    }

    void ApplyOverbright(ID3D11DeviceContext* context, UINT vertexCount, UINT startVertexLocation)
    {
        ID3D11BlendState* oldBlend = nullptr;
        ID3D11PixelShader* oldPS = nullptr;
        float oldFactor[4] = {};
        UINT oldMask = 0;

        context->OMGetBlendState(&oldBlend, oldFactor, &oldMask);
        context->PSGetShader(&oldPS, nullptr, nullptr);

        context->OMSetBlendState(blendOverbright.Get(), oldFactor, oldMask);
        context->PSSetShader(psOverbright.Get(), nullptr, 0);

        Draw_hook.call<void>(context, vertexCount, startVertexLocation);

        context->OMSetBlendState(oldBlend, oldFactor, oldMask);
        context->PSSetShader(oldPS, nullptr, 0);

        if (oldBlend)
        {
            oldBlend->Release();
        }

        if (oldPS)
        {
            oldPS->Release();
        }
    }

    void __stdcall HookedDraw(ID3D11DeviceContext* context, UINT vertexCount, UINT startVertexLocation)
    {
        if (!MGS2_AiRayVision::bLoaded || !bArmed || !g_GameVars.IsStage(MGS2Stages::D080P01)
            || !IsCandidate(context, vertexCount))
        {
            Draw_hook.call<void>(context, vertexCount, startVertexLocation);
            return;
        }

        const UINT ordinal = uCandidate++;
        ProbeVertices(context, startVertexLocation);

        if (static_cast<int>(ordinal) == iPlateOrdinal)
        {
            ApplyOverbright(context, vertexCount, startVertexLocation);
            return;
        }

        Draw_hook.call<void>(context, vertexCount, startVertexLocation);
    }

    // read the previous frame's copy so Map never waits on the GPU
    void LatchPlateOrdinal(UINT readSlot)
    {
        auto* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!context || !vertexProbe[readSlot])
        {
            return;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(context->Map(vertexProbe[readSlot].Get(), 0, D3D11_MAP_READ, 0, &mapped)))
        {
            return;
        }

        const int previous = iPlateOrdinal;
        iPlateOrdinal = -1;

        for (UINT slot = 0; slot < uProbed[readSlot]; slot++)
        {
            const auto* colour = reinterpret_cast<const float*>(
                static_cast<const uint8_t*>(mapped.pData) + slot * kSlotBytes + kColourOffset);

            if (colour[3] > kOverbrightAlpha)
            {
                iPlateOrdinal = static_cast<int>(slot);

                if (g_Logging.bVerboseLogging && previous != iPlateOrdinal)
                {
                    spdlog::info("MGS 2: AI Ray Vision: Overbright plate found at draw {}.", slot);
                }
                break;
            }
        }

        context->Unmap(vertexProbe[readSlot].Get(), 0);
    }

    /// user\skoba\weapon\ai_ray_layout.c -> NewAiRaySight() -> Die()
    int64_t __fastcall MGS2_Die512(int64_t a1)
    {
        bArmed = false;
        iPlateOrdinal = -1;

        return Die512_hook.call<int64_t>(a1);
    }
}

void MGS2_AiRayVision::Setup()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: AI Ray Vision: Disabled in config. Skipping");
        return;
    }

    if (!g_D3D11Hooks.D3DCompileFunc)
    {
        spdlog::error("MGS 2: AI Ray Vision: Failed to get D3DCompile.");
        return;
    }

    ComPtr<ID3DBlob> err;
    const HRESULT hr = g_D3D11Hooks.D3DCompileFunc(kOverbrightShader, strlen(kOverbrightShader), nullptr, nullptr,
        nullptr, "main", "ps_5_0", 0, 0, psBlob.ReleaseAndGetAddressOf(), err.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        spdlog::error("MGS 2: AI Ray Vision: Failed to compile pixel shader: {}",
            err ? static_cast<const char*>(err->GetBufferPointer()) : "Unknown error");
        return;
    }

    MAKE_HOOK_MID(baseModule, "48 89 B3 ?? ?? ?? ?? 89 83", "MGS 2: AI Ray Vision: user\\skoba\\weapon\\ai_ray_layout.c -> NewAiRaySight() -> Act() -> MsgDie()", {
            bArmed = true;
                  });

    if (uint8_t* Die512ScanResult = Memory::PatternScan(baseModule, "4C 8D 05 ?? ?? ?? ?? 89 B8 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 80 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CB", "MGS 2: AI Ray Vision: user\\skoba\\weapon\\ai_ray_layout.c -> NewAiRaySight() -> Die()"))
    {
        const uintptr_t Die512Address = Memory::GetRipRelativeAddress(Die512ScanResult, 3, 7);

        Die512_hook = safetyhook::create_inline(reinterpret_cast<void*>(Die512Address), reinterpret_cast<void*>(MGS2_Die512));
        LOG_HOOK(Die512_hook, "MGS 2: AI Ray Vision: user\\skoba\\weapon\\ai_ray_layout.c -> NewAiRaySight() -> Die() (Die_512)")
    }

    spdlog::info("MGS 2: AI Ray Vision: Compiled pixel shader successfully.");
}

void MGS2_AiRayVision::Init()
{
    if (!(eGameType & MGS2) || !bEnabled || bLoaded)
    {
        return;
    }

    ID3D11Device* dev = g_D3D11Hooks.d3dDevice.Get();
    if (!dev || !psBlob)
    {
        return;
    }

    if (FAILED(dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, psOverbright.GetAddressOf())))
    {
        spdlog::error("MGS 2: AI Ray Vision: Failed to create pixel shader.");
        return;
    }

    D3D11_BLEND_DESC bd = {};
    auto& rt = bd.RenderTarget[0];
    rt.BlendEnable = TRUE;
    rt.SrcBlend = D3D11_BLEND_ONE;
    rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt.BlendOp = D3D11_BLEND_OP_SUBTRACT;
    rt.SrcBlendAlpha = D3D11_BLEND_ZERO;
    rt.DestBlendAlpha = D3D11_BLEND_ONE;
    rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(dev->CreateBlendState(&bd, blendOverbright.GetAddressOf())))
    {
        spdlog::error("MGS 2: AI Ray Vision: Failed to create blend state.");
        return;
    }

    D3D11_BUFFER_DESC pd = {};
    pd.ByteWidth = kSlotBytes * kMaxCandidates;
    pd.Usage = D3D11_USAGE_STAGING;
    pd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    for (auto& probe : vertexProbe)
    {
        if (FAILED(dev->CreateBuffer(&pd, nullptr, probe.GetAddressOf())))
        {
            spdlog::error("MGS 2: AI Ray Vision: Failed to create vertex probe buffer.");
            return;
        }
    }

    if (!Draw_hook)
    {
        void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.d3dDeviceContext.Get());
        Draw_hook = safetyhook::create_inline(vtable[13], reinterpret_cast<void*>(HookedDraw));
        LOG_HOOK(Draw_hook, "MGS 2: AI Ray Vision: ID3D11DeviceContext::Draw")
    }

    psBlob.Reset();

    bLoaded = true;
    spdlog::info("MGS 2: AI Ray Vision initialized.");
}

void MGS2_AiRayVision::OnPresent()
{
    if (!bLoaded)
    {
        return;
    }

    const UINT readSlot = uWriteSlot ^ 1;

    if (uProbed[readSlot] > 0)
    {
        LatchPlateOrdinal(readSlot);
    }
    else
    {
        iPlateOrdinal = -1;
    }

    uWriteSlot = readSlot;
    uProbed[uWriteSlot] = 0;
    uCandidate = 0;
}
