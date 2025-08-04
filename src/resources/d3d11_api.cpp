#include "common.hpp"
#include "d3d11_api.hpp"

#include "gpu_check.hpp"
#include "line_scaling.hpp"
#include "logging.hpp"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <dxgi.h>

// Function declarations
void afterPresent();

// Global hook storage
namespace
{
    SafetyHookInline CreateDXGIFactory_hook {};
    SafetyHookInline CreateSwapChain_hook {};
    SafetyHookInline PresentHook {};

    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    PresentFn oPresent = nullptr;

    HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags)
    {
        static bool initialized = false;
        if (!initialized)
        {
            initialized = true;
            g_D3D11Hooks.swapChain = pSwapChain;

            // Fetch device from swapchain
            ID3D11Device* device = nullptr;
            HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device));
            if (SUCCEEDED(hr) && device)
            {
                g_D3D11Hooks.d3dDevice.Set(device);

                ID3D11DeviceContext* context = nullptr;
                device->GetImmediateContext(&context);
                if (context)
                {
                    g_D3D11Hooks.d3dDeviceContext.Set(context);
                    context->Release(); 
                }

                device->Release(); 
            }
            else
            {
                spdlog::error("Failed to get D3D11Device from SwapChain. HRESULT: 0x{:08X}", hr);
            }

            // Get adapter via IDXGISwapChain -> IDXGIDevice -> IDXGIAdapter
            IDXGIDevice* dxgiDevice = nullptr;
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice))) && dxgiDevice)
            {
                IDXGIAdapter* adapter = nullptr;
                if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)) && adapter)
                {
                    g_D3D11Hooks.dxgiAdapter = adapter; // don't release, store raw

                    DXGI_ADAPTER_DESC desc;
                    if (SUCCEEDED(adapter->GetDesc(&desc)))
                    {
                        std::string gpuName = Util::WideToUTF8(desc.Description);

                        LARGE_INTEGER driverVersion = {};
                        if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driverVersion)))
                        {
                            UINT product = HIWORD(driverVersion.HighPart);
                            UINT version = LOWORD(driverVersion.HighPart);
                            UINT subVersion = HIWORD(driverVersion.LowPart);
                            UINT build = LOWORD(driverVersion.LowPart);

                            CheckMinimumGPU(gpuName, product, version, subVersion, build);
                        }
                        else
                        {
                            spdlog::warn("Could not query GPU driver version.");
                            spdlog::info("Running on GPU: {}", gpuName);
                        }
                    }
                }
                dxgiDevice->Release();
            }
            else
            {
                spdlog::error("Failed to get IDXGIDevice from SwapChain.");
            }

            afterPresent();
        }

        return PresentHook.call<HRESULT>(pSwapChain, syncInterval, flags);
    }



    void HookSwapChainPresent(IDXGISwapChain* swapChain)
    {
        if (!swapChain || oPresent)
            return;

        void** vtable = *reinterpret_cast<void***>(swapChain);
        oPresent = reinterpret_cast<PresentFn>(vtable[8]);

        PresentHook = safetyhook::create_inline(vtable[8], reinterpret_cast<void*>(HookedPresent));
        LOG_HOOK(PresentHook, "PresentHook");
    }

    HRESULT __stdcall HookedCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
    {
        HRESULT result = CreateSwapChain_hook.stdcall<HRESULT>(pFactory, pDevice, pDesc, ppSwapChain);
        if (SUCCEEDED(result) && ppSwapChain && *ppSwapChain)
        {
            g_D3D11Hooks.swapChain = *ppSwapChain;
            HookSwapChainPresent(*ppSwapChain);
        }
        else
        {
            spdlog::error("IDXGIFactory::CreateSwapChain failed. HRESULT: 0x{:08X}", result);
        }

        return result;
    }

    HRESULT WINAPI CreateDXGIFactory_hooked(REFIID riid, _COM_Outptr_ void** ppFactory)
    {
        HRESULT result = CreateDXGIFactory_hook.stdcall<HRESULT>(riid, ppFactory);

        if (SUCCEEDED(result))
        {
            g_D3D11Hooks.dxgiFactory = static_cast<IDXGIFactory*>(*ppFactory);
            // Hook CreateSwapChain now
            void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.dxgiFactory);
            CreateSwapChain_hook = safetyhook::create_inline(vtable[10], reinterpret_cast<void*>(HookedCreateSwapChain));
            LOG_HOOK(CreateSwapChain_hook, "CreateSwapChain.");
        }
        else
        {
            spdlog::error("CreateDXGIFactory failed. HRESULT: 0x{:08X}", result);
        }

        return result;
    }
}

void D3D11Hooks::Initialize()
{
    CreateDXGIFactory_hook = safetyhook::create_inline(CreateDXGIFactory, reinterpret_cast<void*>(CreateDXGIFactory_hooked));
    LOG_HOOK(CreateDXGIFactory_hook, "CreateDXGIFactory");
}


void D3D11Hooks::UnloadCompiler(const HMODULE d3dcompiler)
{
    if (!g_VectorScalingFix.bNeedsCompiler)
    {
        FreeLibrary(d3dcompiler);
        spdlog::info("D3D11Hooks: Released d3dcompiler_43.dll as it is no longer needed.");
    }
}


