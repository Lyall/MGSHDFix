#pragma once

using Microsoft::WRL::ComPtr;

class D3D11Hooks final
{
public:
    static void Initialize();

    HWND MainHwnd = nullptr;

    // ===================== Device and Context =====================
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dDeviceContext;

    // ===================== DXGI =====================
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory> dxgiFactory;
    ComPtr<IDXGISwapChain> swapChain;

    pD3DCompile D3DCompileFunc;

    uint64_t FrameCount = 0;

};

inline D3D11Hooks g_D3D11Hooks;
