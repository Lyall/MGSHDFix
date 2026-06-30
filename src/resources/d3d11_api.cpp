#include "stdafx.h"

#include "common.hpp"
#include "d3d11_api.hpp"

#include "gpu_check.hpp"
#include "logging.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "color_correction.hpp"
#include "effect_speeds.hpp"
#include "input_handler.hpp"
#include "mgs2_3rd_person_freecam.hpp"
#include "mgs2_contrast_fix.hpp"
#include "mgs2_first_person_view_mode.hpp"
#include "mgs2_thermal_goggles.hpp"
#include "mgs2_underwater_filter.hpp"
#include "d3d11_text_overlay.hpp"
#include "mg1_display_scaling.hpp"
#include "mgs2_crossfade.hpp"
#include "mgs_smaa.hpp"
#include "depth_of_field.hpp"
#include "scene_depth.hpp"
void afterPresent();

namespace
{
    bool g_preMenuFired = false;

    // Hooks
    SafetyHookInline CreateDXGIFactory_hook {};
    SafetyHookInline CreateSwapChain_hook {};
    SafetyHookInline PresentHook {};
    SafetyHookInline ResizeBuffersHook {};

    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    PresentFn oPresent = nullptr;

    using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    ResizeBuffersFn oResizeBuffers = nullptr;

    void RefreshDeviceAndContext(IDXGISwapChain* swap)
    {
        ComPtr<ID3D11Device> device;
        if (SUCCEEDED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(device.GetAddressOf()))) && device)
        {
            g_D3D11Hooks.d3dDevice = device;

            ComPtr<ID3D11DeviceContext> context;
            device->GetImmediateContext(context.GetAddressOf());
            if (context)
            {
                g_D3D11Hooks.d3dDeviceContext = context;
                spdlog::info("D3D11 Device and Context refreshed successfully.");

                //HookDevice(device.Get());
            }
            else
            {
                spdlog::error("Failed to get ID3D11DeviceContext from ID3D11Device.");
            }
        }
        else
        {
            spdlog::error("Failed to get ID3D11Device from IDXGISwapChain.");
        }
    }



    HRESULT __stdcall HookedPresent(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags)
    {
        static bool firstInit = false;

        if (!firstInit)
        {
            firstInit = true;

            g_D3D11Hooks.swapChain = pSwapChain;
            RefreshDeviceAndContext(pSwapChain);

            // ==== GPU logging + driver version check ====
            IDXGIDevice* dxgiDevice = nullptr;
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice))) && dxgiDevice)
            {
                IDXGIAdapter* adapter = nullptr;
                if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)) && adapter)
                {
                    g_D3D11Hooks.dxgiAdapter = adapter;

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

                            if (!Util::IsSteamOS())
                            {
                                CheckMinimumGPU(gpuName, true, product, version, subVersion, build);
                            }
                            else
                            {
                                spdlog::info("Running on SteamOS with GPU: {}. Driver version: {}.{}.{}.{}", gpuName, product, version, subVersion, build);
                            }
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
            SceneDepth::Initialize();   // hook OMSetRenderTargets now that the context exists
            afterPresent();
        }

        {
            ComPtr<ID3D11Device> deviceFromSwap;
            if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(deviceFromSwap.GetAddressOf()))))
            {
                static ComPtr<ID3D11Device> lastDevice;
                if (deviceFromSwap.Get() != lastDevice.Get())
                {
                    lastDevice = deviceFromSwap;
                    RefreshDeviceAndContext(pSwapChain);
                }
            }

        }

        g_EffectSpeedFix.Tick();
        g_InputHandler.Update();


        if (eGameType & MGS2)
        {
            SceneDepth::ResetStatus();
            MGS2_ThirdPersonFreecam::Tick();
            MGS2_First_Person_View::Tick();
            MGS2ThermalGoggles::Tick();

            if (auto* work = MGS2_ContrastShader::GetActiveWork(); MGS2_ContrastShader::bShaderLoaded && work)
            {
                MGS2_ContrastShader::Draw(pSwapChain, work->keep_r_plus, work->keep_g_plus, work->keep_b_plus, work->keep_a_plus, work->nega_posi_flag);
            }
            g_MGS2UnderwaterFilterFix.BeforePresent();
            MGS2_Crossfade::OnPresent(pSwapChain);
        }
        else if (eGameType & MGS3)
        {
            g_DepthOfFieldFixes.OnPresent();

            ComPtr<ID3D11Texture2D> bb;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(bb.GetAddressOf()));
            if (bb)
            {
                ComPtr<ID3D11RenderTargetView> bbRTV;
                g_D3D11Hooks.d3dDevice->CreateRenderTargetView(bb.Get(), nullptr, bbRTV.GetAddressOf());
                if (bbRTV)
                    SMAA_AA::Draw(bbRTV.Get(), nullptr);
            }
        }
        else if (eGameType & MG)
        {
            MG1_DisplayScaling::Draw(pSwapChain);
        }

        ColorCorrection::Draw(pSwapChain);
        D3D11TextOverlay::Tick(); //keep last.
        g_preMenuFired = false;
        return PresentHook.call<HRESULT>(pSwapChain, syncInterval, flags);
    }


    HRESULT __stdcall HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
    {
        HRESULT result = ResizeBuffersHook.call<HRESULT>(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

        if (SUCCEEDED(result))
        {
            RefreshDeviceAndContext(pSwapChain);
        }

        return result;
    }

    void HookSwapChainPresent(IDXGISwapChain* swapChain)
    {
        if (!swapChain || oPresent)
            return;

        void** vtable = *reinterpret_cast<void***>(swapChain);
        oPresent = reinterpret_cast<PresentFn>(vtable[8]);

        PresentHook = safetyhook::create_inline(vtable[8], reinterpret_cast<void*>(HookedPresent));
        LOG_HOOK(PresentHook, "PresentHook");

        oResizeBuffers = reinterpret_cast<ResizeBuffersFn>(vtable[13]);
        ResizeBuffersHook = safetyhook::create_inline(vtable[13], reinterpret_cast<void*>(HookedResizeBuffers));
        LOG_HOOK(ResizeBuffersHook, "ResizeBuffersHook");
    }

    HRESULT __stdcall HookedCreateSwapChain(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
    {

        HRESULT result = CreateSwapChain_hook.stdcall<HRESULT>(pFactory, pDevice, pDesc, ppSwapChain);
        if (SUCCEEDED(result) && ppSwapChain && *ppSwapChain)
        {
            g_D3D11Hooks.swapChain = *ppSwapChain;
            RefreshDeviceAndContext(*ppSwapChain);
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
            void** vtable = *reinterpret_cast<void***>(g_D3D11Hooks.dxgiFactory.Get());
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
    HMODULE d3dcompiler = LoadLibraryA("d3dcompiler_43.dll");
    if (d3dcompiler)
    {
        g_D3D11Hooks.D3DCompileFunc = reinterpret_cast<pD3DCompile>(GetProcAddress(d3dcompiler, "D3DCompile"));
    }
    else
    {
        spdlog::error("D3D11Hooks: failed to load d3dcompiler_43.dll"); 
    }

    CreateDXGIFactory_hook = safetyhook::create_inline(CreateDXGIFactory, reinterpret_cast<void*>(CreateDXGIFactory_hooked));
    LOG_HOOK(CreateDXGIFactory_hook, "CreateDXGIFactory");

    if (eGameType & MGS2)
    {
        constexpr uint32_t kDG_DMAPACK_MENU = 0x0002;
        MAKE_HOOK_MID(baseModule, "40 55 57 41 56 48 8D AC 24 40 FC FF FF 48 81 EC C0 04 00 00 48 8B F9", "D3D11 Hooks: BP_RenderDmaPack_AutoPacket", {
                auto* dmapack = *reinterpret_cast<const uintptr_t* const*>(ctx.rcx);
                if (!dmapack)
                {
                    return;
                }
                if (!(*reinterpret_cast<const uint32_t*>(dmapack) & kDG_DMAPACK_MENU))
                {
                    return;
                }
                if (g_preMenuFired)
                {
                    return;
                }
                g_preMenuFired = true;
                SceneDepth::OnPreMenuRender();
                });
    }

    // dmapack hook disabled for mgs3 atm. it seems to be a bit too early in the render 
    /*mgs3:  "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 48 8B D9")
    if (!(*reinterpret_cast<const uint32_t*>(dmapack + 24) & kDG_DMAPACK_MENU))
    {
        return;
    }*/
}
