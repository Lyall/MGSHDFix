#pragma once
#include "helper.hpp"
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D11Hooks final
{
public:
    static void Initialize();
    static void UnloadCompiler(HMODULE d3dcompiler);

    HWND MainHwnd = nullptr;

    // ===================== Device and Context =====================
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dDeviceContext;

    // ===================== DXGI =====================
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory> dxgiFactory;
    ComPtr<IDXGISwapChain> swapChain;
};

inline D3D11Hooks g_D3D11Hooks;
