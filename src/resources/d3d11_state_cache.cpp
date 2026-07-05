#include "stdafx.h"
#include "d3d11_state_cache.hpp"
#include "helper.hpp"

namespace
{
    constexpr UINT kContextIASetInputLayout = 17;
    constexpr UINT kContextIASetPrimitiveTopology = 24;
    constexpr UINT kContextExecuteCommandList = 58;
    constexpr UINT kContextClearState = 110;

    SafetyHookInline gIASetInputLayoutHook {};
    SafetyHookInline gIASetPrimitiveTopologyHook {};
    SafetyHookInline gExecuteCommandListHook {};
    SafetyHookInline gClearStateHook {};

    ID3D11DeviceContext* gCachedContext = nullptr;
    ID3D11InputLayout* gCurrentInputLayout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY gCurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    bool gInputLayoutKnown = false;
    bool gTopologyKnown = false;
    bool gStateCacheReady = false;
    bool gHooksInstalled = false;

    void ResetCachedState(ID3D11DeviceContext* context)
    {
        gCachedContext = context;
        gCurrentInputLayout = nullptr;
        gCurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        gInputLayoutKnown = false;
        gTopologyKnown = false;
    }

    void EnsureContext(ID3D11DeviceContext* context)
    {
        if (gCachedContext != context)
        {
            ResetCachedState(context);
        }
    }

    void __stdcall IASetInputLayoutHooked(ID3D11DeviceContext* context, ID3D11InputLayout* inputLayout)
    {
        EnsureContext(context);
        if (gStateCacheReady && gInputLayoutKnown && gCurrentInputLayout == inputLayout)
        {
            return;
        }

        gCurrentInputLayout = inputLayout;
        gInputLayoutKnown = true;
        gIASetInputLayoutHook.stdcall<void>(context, inputLayout);
    }

    void __stdcall IASetPrimitiveTopologyHooked(ID3D11DeviceContext* context, D3D11_PRIMITIVE_TOPOLOGY topology)
    {
        EnsureContext(context);
        if (gStateCacheReady && gTopologyKnown && gCurrentTopology == topology)
        {
            return;
        }

        gCurrentTopology = topology;
        gTopologyKnown = true;
        gIASetPrimitiveTopologyHook.stdcall<void>(context, topology);
    }

    void __stdcall ExecuteCommandListHooked(ID3D11DeviceContext* context, ID3D11CommandList* commandList, BOOL restoreContextState)
    {
        gExecuteCommandListHook.stdcall<void>(context, commandList, restoreContextState);
        ResetCachedState(context);
    }

    void __stdcall ClearStateHooked(ID3D11DeviceContext* context)
    {
        gClearStateHook.stdcall<void>(context);

        gCachedContext = context;
        gCurrentInputLayout = nullptr;
        gCurrentTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        gInputLayoutKnown = true;
        gTopologyKnown = true;
    }
}

void D3D11StateCache::Initialize(ID3D11Device* /*device*/, ID3D11DeviceContext* context)
{
    if (gHooksInstalled || !context)
    {
        return;
    }

    void** vtable = *reinterpret_cast<void***>(context);
    gIASetInputLayoutHook = safetyhook::create_inline(vtable[kContextIASetInputLayout], reinterpret_cast<void*>(IASetInputLayoutHooked));
    gIASetPrimitiveTopologyHook = safetyhook::create_inline(vtable[kContextIASetPrimitiveTopology], reinterpret_cast<void*>(IASetPrimitiveTopologyHooked));
    gExecuteCommandListHook = safetyhook::create_inline(vtable[kContextExecuteCommandList], reinterpret_cast<void*>(ExecuteCommandListHooked));
    gClearStateHook = safetyhook::create_inline(vtable[kContextClearState], reinterpret_cast<void*>(ClearStateHooked));

    gStateCacheReady = gIASetInputLayoutHook && gIASetPrimitiveTopologyHook && gExecuteCommandListHook && gClearStateHook;
    gHooksInstalled = gIASetInputLayoutHook || gIASetPrimitiveTopologyHook || gExecuteCommandListHook || gClearStateHook;
    ResetCachedState(context);
}

void D3D11StateCache::Invalidate(ID3D11DeviceContext* context)
{
    if (context)
    {
        ResetCachedState(context);
    }
}

