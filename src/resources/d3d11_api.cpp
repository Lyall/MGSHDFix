#include "stdafx.h"

#include "common.hpp"
#include "d3d11_api.hpp"

#include "gamevars.hpp"
#include "gpu_check.hpp"
#include "logging.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "color_correction.hpp"
#include "effect_speeds.hpp"
#include "input_handler.hpp"
#include "mgs2_3rd_person_freecam.hpp"
#include "mgs2_contrast_fix.hpp"
#include "mgs2_ai_ray_vision.hpp"
#include "mgs2_first_person_view_mode.hpp"
#include "mgs2_thermal_goggles.hpp"
#include "mgs2_underwater_filter.hpp"
#include "mgs2_demo_blur.hpp"
#include "mgs2_gas_haze.hpp"
#include "d3d11_text_overlay.hpp"
#include "mg1_display_scaling.hpp"
#include "mgs2_crossfade.hpp"
#include "photo_camera.hpp"
#include "mgs3_crossfade_capture.hpp"
#include "mgs_smaa.hpp"
#include "depth_of_field.hpp"
#include "mgs3_film_grain.hpp"
#include "scene_depth.hpp"
#include "mgs2_soft_shadows.hpp"
#include "mgs3_glow_overbright.hpp"
#include "mgs2_scanline_scale.hpp"
#include "mgs3_map_relight.hpp"
#include "d3d11_state_cache.hpp"
void afterPresent();

namespace
{
    bool g_preMenuFired = false;

    // Hooks
    SafetyHookInline CreateDXGIFactory_hook {};
    SafetyHookInline CreateSwapChain_hook {};
    SafetyHookMid PresentHook {};
    SafetyHookInline ResizeBuffersHook {};

    using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    ResizeBuffersFn oResizeBuffers = nullptr;

    // The PS2 never presented a frame it hadn't drawn; the port does, and it flashes dim.
    ComPtr<ID3D11Texture2D> g_heldFrame;
    UINT g_heldW = 0, g_heldH = 0;
    bool g_heldValid = false;

    // A stage load parks DG_UnDrawFrameCount at DG_UNDRAW_MAX, and the port draws its own
    // loading screen through those frames, so hold only the short scheduled runs.
    constexpr int64_t kMaxHeldRun = 64;

    // DG_StartFrame clears DG_LastWhich to -1 only on the path that draws.
    bool IsUnDrawnFrame()
    {
        return g_GameVars.DG_LastWhich() >= 0 && g_GameVars.DG_UnDrawFrameCount() <= kMaxHeldRun;
    }

    // The MGS3 crossfade teardown can present one ungraded frame; it shows up as the same shadow
    // targets rebinding at exactly double their steady rate. Only consulted inside the short
    // window the crossfade Die hook arms, so scenery changes elsewhere can never trip it.
    bool IsDoubleRenderedFrame(bool releaseWindow)
    {
        static std::vector<std::pair<const void*, uint32_t>> baseline;
        static int steady = 0;
        static bool firedLast = false;

        std::vector<std::pair<const void*, uint32_t>> now;
        SceneDepth::GetShadowPasses(now);

        const bool sameTargets = now.size() == baseline.size() && !now.empty() &&
            std::equal(now.begin(), now.end(), baseline.begin(),
                [](const auto& a, const auto& b) { return a.first == b.first; });

        if (sameTargets && std::equal(now.begin(), now.end(), baseline.begin(),
                [](const auto& a, const auto& b) { return a.second == b.second; }))
        {
            steady++;
            firedLast = false;
            return false;
        }

        const bool doubled = sameTargets && !firedLast && steady >= 3 && releaseWindow &&
            std::equal(now.begin(), now.end(), baseline.begin(),
                [](const auto& a, const auto& b) { return a.second == b.second * 2; });

        if (doubled)
        {
            firedLast = true;   // hold the duplicate, keep the baseline for the frames after it
            return true;
        }

        baseline = now;
        steady = 0;
        firedLast = false;
        return false;
    }

    void HoldPreviousFrame(IDXGISwapChain* swap)
    {
        // Consume the passes every present, before any bail-out, or they accrue into a false double.
        SceneDepth::ReadAndResetShadowSetCount();
        const bool doubleRendered = IsDoubleRenderedFrame(MGS3_CrossfadeCapture::ReleaseWindowTick());

        auto* device = g_D3D11Hooks.d3dDevice.Get();
        auto* context = g_D3D11Hooks.d3dDeviceContext.Get();
        if (!device || !context || !swap)
        {
            return;
        }

        ComPtr<ID3D11Texture2D> backbuffer;
        if (FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backbuffer.GetAddressOf()))) || !backbuffer)
        {
            return;
        }

        D3D11_TEXTURE2D_DESC bb {};
        backbuffer->GetDesc(&bb);
        if (!g_heldFrame || g_heldW != bb.Width || g_heldH != bb.Height)
        {
            g_heldFrame.Reset();
            g_heldValid = false;
            D3D11_TEXTURE2D_DESC td = bb;
            td.MipLevels = 1;
            td.BindFlags = 0;
            td.MiscFlags = 0;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.CPUAccessFlags = 0;
            if (FAILED(device->CreateTexture2D(&td, nullptr, g_heldFrame.GetAddressOf())))
            {
                return;
            }
            g_heldW = bb.Width;
            g_heldH = bb.Height;
        }

        const bool undrawn = IsUnDrawnFrame();

        // A stage boundary invalidates the held frame and every frame-feedback capture, or the
        // old scene replays into the new stage's first frames.
        static std::string s_heldStage;
        static int64_t s_lastUndrawCount = 0;
        static int s_parkSyncFrames = 0;
        const int64_t undrawCount = g_GameVars.DG_UnDrawFrameCount();
        const char* stage = g_GameVars.GetCurrentStage();
        if ((stage && s_heldStage != stage) || s_lastUndrawCount > kMaxHeldRun)
        {
            g_heldValid = false;
            if (eGameType & MGS2)
            {
                MGS2DemoBlur::InvalidateCapture();
                MGS2GasHaze::InvalidateCapture();
                g_MGS2UnderwaterFilterFix.InvalidateCapture();
            }
            if (stage)
            {
                s_heldStage = stage;
            }
        }
        if (undrawCount > kMaxHeldRun && s_lastUndrawCount <= kMaxHeldRun)
        {
            s_parkSyncFrames = 4;
        }
        s_lastUndrawCount = undrawCount;

        // During loading the port shows its two internal frame buffers in turns. If they differ,
        // the old frame flashes on screen - so make them equal when the load starts.
        if (s_parkSyncFrames > 0 && undrawCount > kMaxHeldRun)
        {
            s_parkSyncFrames--;
            SceneDepth::SyncRecentSceneTargets();
            return;
        }

        if ((undrawn || doubleRendered) && g_heldValid)
        {
            if (g_Logging.bVerboseLogging)
            {
                spdlog::info("D3D11Hooks: {} frame, holding previous image. (stage={})",
                    undrawn ? "undrawn" : "double-rendered", g_GameVars.GetCurrentStage());
            }
            context->CopyResource(backbuffer.Get(), g_heldFrame.Get());
            return;
        }

        if (undrawn)
        {
            return;
        }

        context->CopyResource(g_heldFrame.Get(), backbuffer.Get());
        g_heldValid = true;
    }

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


    //Don't hook Present directly, as streaming software like OBS might hook before us, resulting in our Present() effects not showing up on streams / in recordings.
    void BeforePresent(safetyhook::Context& ctx)
    {
        IDXGISwapChain* pSwapChain = reinterpret_cast<IDXGISwapChain*>(ctx.rcx);
        static bool firstInit = false;

        if ((eGameType & (MGS2 | MGS3)) && firstInit)
        {
            HoldPreviousFrame(pSwapChain);
        }

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
            MGS2ScanlineScale::OnDeviceReady();
            MGS3MapRelight::OnDeviceReady();
            MGS3GlowOverbright::OnDeviceReady(g_D3D11Hooks.d3dDevice.Get());
            if (eGameType & (MGS2 | MGS3))
            {
                // Drop redundant IA state changes - the games re-set layout/topology per draw.
                D3D11StateCache::Initialize(g_D3D11Hooks.d3dDevice.Get(), g_D3D11Hooks.d3dDeviceContext.Get());
            }
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

        //g_EffectSpeedFix.Tick();
        g_InputHandler.Update();

        if (eGameType & MGS3)
        {
            MGS3FilmGrain::BeginPresent();
        }

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
            MGS2_AiRayVision::OnPresent();
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
        if (eGameType & MGS3)
        {
            MGS3FilmGrain::EndPresent();
        }
        if (eGameType & (MGS2 | MGS3))
        {
            PhotoCamera::OnPresent(pSwapChain);   // before the overlay: the photo is the game frame
        }
        D3D11TextOverlay::Tick(); //keep last.
        g_preMenuFired = false;
        g_D3D11Hooks.FrameCount++;
    }


    HRESULT __stdcall HookedResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
    {
        HRESULT result = ResizeBuffersHook.call<HRESULT>(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

        if (SUCCEEDED(result))
        {
            RefreshDeviceAndContext(pSwapChain);
            MGS2SoftShadows::Reset();
        }

        return result;
    }

    void HookSwapChainPresent(IDXGISwapChain* swapChain)
    {
        if (!swapChain || oResizeBuffers)
            return;

        void** vtable = *reinterpret_cast<void***>(swapChain);
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
    if (!(eGameType & (MG|MGS2|MGS3)))
    {
        return;
    }

    spdlog::info("D3D11Hooks: Initializing D3D11 hooks.");
    uint8_t* Present_scan = Memory::PatternScan(baseModule, eGameType & MGS2 ? "FF 50 ?? 8B F0" : "FF 50 ?? 48 8D 4C 24 ?? 8B F0", "D3D11Hooks: Before present hook");
    if (!Present_scan)
    {
        return;
    }
    
    PresentHook = safetyhook::create_mid(Present_scan, BeforePresent);
    spdlog::info("D3D11Hooks: BeforePresent hook installed successfully.");


    if (const HMODULE d3dcompiler = LoadLibraryA("d3dcompiler_43.dll"))
    {
        g_D3D11Hooks.D3DCompileFunc = reinterpret_cast<pD3DCompile>(GetProcAddress(d3dcompiler, "D3DCompile"));
        spdlog::info("D3D11Hooks: d3dcompiler_43.dll loaded successfully.");
    }
    else
    {
        spdlog::error("D3D11Hooks: failed to load d3dcompiler_43.dll");
    }

    CreateDXGIFactory_hook = safetyhook::create_inline(CreateDXGIFactory, reinterpret_cast<void*>(CreateDXGIFactory_hooked));
    LOG_HOOK(CreateDXGIFactory_hook, "CreateDXGIFactory");

    if (eGameType & MGS2)
    {
        constexpr uint32_t kDG_DMAPACK_NORMAL = 0x0001;
        constexpr uint32_t kDG_DMAPACK_MENU = 0x0002;
        MAKE_HOOK_MID(baseModule, "40 55 57 41 56 48 8D AC 24 40 FC FF FF 48 81 EC C0 04 00 00 48 8B F9", "D3D11 Hooks: BP_RenderDmaPack_AutoPacket", {
                auto* dmapack = *reinterpret_cast<const uintptr_t* const*>(ctx.rcx);
                if (!dmapack)
                {
                    return;
                }

                const auto* bytes = reinterpret_cast<const uint8_t*>(dmapack);
                const uint32_t dmapackFlags = *reinterpret_cast<const uint32_t*>(bytes);

                // NORMAL|MENU first runs on the normal channel.
                if (!g_preMenuFired &&
                    (dmapackFlags & kDG_DMAPACK_MENU) &&
                    !(dmapackFlags & kDG_DMAPACK_NORMAL))
                {
                    g_preMenuFired = true;
                    SceneDepth::OnPreMenuRender();
                }

                });
    }

    // dmapack hook disabled for mgs3 atm. it seems to be a bit too early in the render 
    /*mgs3:  "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 0F 29 70 ?? 48 8B D9")
    if (!(*reinterpret_cast<const uint32_t*>(dmapack + 24) & kDG_DMAPACK_MENU))
    {
        return;
    }*/
}
