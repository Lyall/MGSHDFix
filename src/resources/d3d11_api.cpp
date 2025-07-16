#include "common.hpp"
#include "d3d11_api.hpp"
#include "logging.hpp"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern void preD3D11CreateDevice();
extern void afterD3D11CreateDevice();

extern void preCreateDXGIFactory();
extern void afterCreateDXGIFactory();
namespace
{
    // D3D11CreateDevice Hook
    SafetyHookInline D3D11CreateDevice_hook {};
    HRESULT WINAPI D3D11CreateDevice_hooked(
        _In_opt_ IDXGIAdapter* pAdapter,
        D3D_DRIVER_TYPE DriverType,
        HMODULE Software,
        UINT Flags,
        _In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
        UINT FeatureLevels,
        UINT SDKVersion,
        _COM_Outptr_opt_ ID3D11Device** ppDevice,
        _Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
        _COM_Outptr_opt_ ID3D11DeviceContext** ppImmediateContext)
    {
        preD3D11CreateDevice();
        HRESULT result = D3D11CreateDevice_hook.stdcall<HRESULT>(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
        g_D3D11Hooks.dxgiAdapter = pAdapter;
        g_D3D11Hooks.d3dDevice = *ppDevice;
        g_D3D11Hooks.d3dDeviceContext = *ppImmediateContext;
        if (SUCCEEDED(result))
        {
            if (g_D3D11Hooks.d3dDevice)
            {
                spdlog::info("D3D11CreateDevice successful.");
                afterD3D11CreateDevice();
            }
            else
            {
                spdlog::error("d3dDevice is nullptr after D3D11CreateDevice.");
            }
        }
        else
        {
            spdlog::error("Failed to create D3D11 Device. HRESULT: 0x{:08X}", result);
        }
        return result;
    }




    // D3D11CreateDevice Hook
    SafetyHookInline CreateDXGIFactory_hook {};
    HRESULT WINAPI CreateDXGIFactory_hooked(REFIID riid, _COM_Outptr_ void** ppFactory)
    {
        preCreateDXGIFactory();
        auto result = CreateDXGIFactory_hook.stdcall<HRESULT>(riid, ppFactory);
        if (SUCCEEDED(result))
        {
            g_D3D11Hooks.dxgiFactory = static_cast<IDXGIFactory*>(*ppFactory);
            if (g_D3D11Hooks.dxgiFactory)
            {
                spdlog::info("CreateDXGIFactory successful.");
                afterCreateDXGIFactory();
            }
            else
            {
                spdlog::error("DXGI Factory is nullptr after CreateDXGIFactory.");
            }
        }
        else
        {
            spdlog::error("Failed to create DXGI Factory.");
        }
        return result;
    }

}

void D3D11Hooks::Initialize()
{
    D3D11CreateDevice_hook = safetyhook::create_inline(D3D11CreateDevice, reinterpret_cast<void*>(D3D11CreateDevice_hooked));
    LOG_HOOK(D3D11CreateDevice_hook, "MG/MG2 | MGS 2 | MGS 3: D3D11CreateDevice: Hooked function.")
        CreateDXGIFactory_hook = safetyhook::create_inline(CreateDXGIFactory, reinterpret_cast<void*>(CreateDXGIFactory_hooked));
    LOG_HOOK(CreateDXGIFactory_hook, "MG/MG2 | MGS 2 | MGS 3: CreateDXGIFactory: Hooked function.")
}
