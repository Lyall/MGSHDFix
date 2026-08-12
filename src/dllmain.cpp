#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"
#include "submodule_initiailization.hpp"
#include "config.hpp"
#include "config_keys.hpp"

///Resources
#include "d3d11_api.hpp"
#include "gamevars.hpp"
#include "steamworks_api.hpp"
#include "version_checking.hpp"

///Features
#include "custom_resolution_and_borderless.hpp"
#include "distance_culling.hpp"
#include "effect_speeds.hpp"
#include "intro_skip.hpp"
#include "keep_aiming_after_firing.hpp"
#include "pause_on_focus_loss.hpp"
#include "stat_persistence.hpp"
#include "mgs2_sunglasses.hpp"
#include "pressure_inputs.hpp"
#include "ds3_rumble.hpp"
#include "mgs2_restore_dogtags.hpp"
#include "swap_menu_buttons.hpp"
#include "mgs2_restore_phone_jingle.hpp"
#include "mgs2_restore_action_level_selection.hpp"
#include "mgs2_3rd_person_freecam.hpp"
#include "mgs2_difficulty.hpp"
#include "mgs2_hostage_type_easter_egg.hpp"
#include "original_camera_positions.hpp"
#include "expand_bp_assets.hpp"
#include "mgs2_vamp_punch_fix.hpp"

///Fixes
#include "aiming_full_tilt.hpp"
#include "mgs2_blood_stains.hpp"
#include "mgs2_camera_offsets.hpp"
#include "mgs2_demo_scope.hpp"
#include "mgs2_scope_warp.hpp"
#include "mgs2_water_effects.hpp"
#include "mgs2_codec_band.hpp"
#include "mgs2_lens_droplets.hpp"
#include "mgs2_gas_haze.hpp"
#include "mgs2_demo_camera_judder.hpp"
#include "mgs2_hair_layering.hpp"
#include "mgs2_rotor_procession.hpp"
#include "mgs2_reverb_wet_level.hpp"
#include "mgs2_railgun_beam.hpp"
#include "mgs2_demo_blur.hpp"
#include "mgs2_flare_occlusion.hpp"
#include "mgs2_tanker_snake_snap.hpp"
#include "mgs2_glass_dmapack_overflow.hpp"
#include "cpu_core_limit.hpp"
#include "aiming_after_equip.hpp"
#include "line_scaling.hpp"
#include "optical_camo.hpp"
#include "stereo_audio.hpp"
#include "water_reflections.hpp"
#include "mgs3_hud_fixes.hpp"
#include "mgs3_film_grain.hpp"
#include "mgs3_crossfade_capture.hpp"
#include "windows_fullscreen_optimization.hpp"
#include "busy_loop_fix.hpp"
#include "mgs2_snakearm_voice.hpp"
#include "resolution_scaling_fixes.hpp"
#include "texture_live_swaps.hpp"
#include "mgs2_coolant_mirror.hpp"
#include "mgs2_restore_dogtag_viewer.hpp"
#include "mgs2_underwater_filter.hpp"
#include "mgs2_hostage_model.hpp"
#include "mgs2_ray_photo_voice.hpp"

//Warnings
#include "asi_loader_checks.hpp"
#include "corrupt_save_message.hpp"
#include "mute_warning.hpp"
#include "reshade_compatibility_checks.hpp"
#include "background_shuffle_warning.hpp"
#include "check_gamesave_folder.hpp"
#include "bugfix_mod_checks.hpp"

///WIP
#include "depth_of_field.hpp"
#include "mg1_custom_loading_screens.hpp"
#include "mgs2_kirari_sun2_fix.hpp"
#include "mgs2_preshade_lights.hpp"
#include "mgs3_fix_camera_offset.hpp"
#include "mgs3_fix_holster_after_torture.hpp"
#include "mgs2_msx_colonel.hpp"
#include "adjustable_captions.hpp"
#include "color_correction.hpp"
#include "custom_player_name.hpp"
#include "mgs2_first_person_view_mode.hpp"
#include "cutscene_pausing.hpp"
#include "d3d11_text_overlay.hpp"
#include "mg1_display_scaling.hpp"
#include "mgs2_codec_background.hpp"
#include "mgs2_contrast_fix.hpp"
#include "mgs2_ai_ray_vision.hpp"
#include "mgs2_parrot_radar_fix.hpp"
#include "mgs2_restore_sol_radar.hpp"
#include "mgs2_restore_elevator_glitch.hpp"
#include "mgs2_shimmer.hpp"
#include "mgs2_crossfade.hpp"
#include "photo_camera.hpp"
#include "mgs2_newscrconcentrateblur.hpp"
#include "mgs2_enhanced_demos.hpp"
#include "playtime_fixes.hpp"
#include "mgs2_snake_tales_radar.hpp"
#include "mgs2_thermal_goggles.hpp"
#include "mgs_smaa.hpp"
#include "caption_replacements.hpp"
#include "screenspace_fixes.hpp"
#include "windows_preferred_gpu.hpp"
//#include "texture_buffer_size.hpp" //disabled for now, the vanilla limit was increased to 128MB/texture in 2.0.0, so there's no much need until 8k gaming is standard & there's a need for a 16k texture pack lol.




#if !defined(RELEASE_BUILD)
#include "unit_tests.hpp"
#endif

static void Init_Miscellaneous()
{
    if (eGameType & (MG|MGS2|MGS3|LAUNCHER))
    {
        if (bDisableCursor)
        {
            // Launcher | MG/MG2 | MGS 2 | MGS 3: Disable mouse cursor
            // Thanks again emoose!
            if (uint8_t* MGS2_MGS3_MouseCursorScanResult = Memory::PatternScan(eGameType & LAUNCHER ? unityPlayer : baseModule, "BA 00 7F 00 00 33 ?? FF ?? ?? ?? ?? ?? 48 ?? ??", "Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor"))
            {
                // The game enters 32512 in the RDX register for the function USER32.LoadCursorA to load IDC_ARROW (normal select arrow in windows)
                // Set this to 0 and no cursor icon is loaded
                Memory::PatchBytes((uintptr_t)MGS2_MGS3_MouseCursorScanResult + 0x2, "\x00", 1);
                spdlog::info("Launcher | MG/MG2 | MGS 2 | MGS 3: Mouse Cursor: Patched instruction.");
            }
        }
    }

    if ((bDisableTextureFiltering || iAnisotropicFiltering > 0) && (eGameType & (MGS2|MGS3)))
    {
        if (uint8_t* MGS3_SetSamplerStateInsnScanResult = Memory::PatternScan(baseModule, "48 8B ?? ?? ?? ?? ?? 44 39 ?? ?? 38 ?? ?? ?? 74 ?? 44 89 ?? ?? ?? ?? ?? ?? EB ?? 48 ?? ??", "MGS 2 | MGS 3: Texture Filtering"))
        {
            static SafetyHookMid SetSamplerStateInsnXMidHook{};
            SetSamplerStateInsnXMidHook = safetyhook::create_mid(MGS3_SetSamplerStateInsnScanResult + 0x7,
                [](SafetyHookContext& ctx)
                {
                    // [rcx+rax+0x438] = D3D11_SAMPLER_DESC, +0x14 = MaxAnisotropy
                    *reinterpret_cast<int*>(ctx.rcx + ctx.rax + 0x438 + 0x14) = iAnisotropicFiltering;

                    // Override filter mode in r9d with aniso value and run compare from orig game code
                    // Game code will then copy in r9d & update D3D etc when r9d is different to existing value
                    //0x1 = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR (Linear mips is essentially perspective correction.) 0x55 = D3D11_FILTER_ANISOTROPIC
                    ctx.r9 = bDisableTextureFiltering ? 0x1 : 0x55;
                });
            LOG_HOOK(SetSamplerStateInsnXMidHook, "MGS 2 | MGS 3: Texture Filtering")
        }

    }

    if (eGameType & MGS3 && bMouseSensitivity)
    {
        // MG 1/2 | MGS 2 | MGS 3: MouseSensitivity
        uint8_t* MGS3_MouseSensitivityScanResult = Memory::PatternScanSilent(baseModule, "F3 0F ?? ?? ?? F3 0F ?? ?? 66 0F ?? ?? ?? 0F ?? ?? 66 0F ?? ?? 8B ?? ??");
        if (MGS3_MouseSensitivityScanResult)
        {
            spdlog::info("MGS 3: Mouse Sensitivity: Address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS3_MouseSensitivityScanResult - (uintptr_t)baseModule);

            static SafetyHookMid MouseSensitivityXMidHook{};
            MouseSensitivityXMidHook = safetyhook::create_mid(MGS3_MouseSensitivityScanResult,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] *= fMouseSensitivityXMulti;
                });

            static SafetyHookMid MouseSensitivityYMidHook{};
            MouseSensitivityYMidHook = safetyhook::create_mid(MGS3_MouseSensitivityScanResult + 0x2E,
                [](SafetyHookContext& ctx)
                {
                    ctx.xmm0.f32[0] *= fMouseSensitivityYMulti;
                });
        }
        else if (!MGS3_MouseSensitivityScanResult)
        {
            spdlog::error("MGS 3: Mouse Sensitivity: Pattern scan failed.");
        }
    }
}


using NHT_COsContext_SetControllerID_Fn = void (*)(int controllerType);
NHT_COsContext_SetControllerID_Fn NHT_COsContext_SetControllerID = nullptr;
void NHT_COsContext_SetControllerID_Hook(int controllerType)
{
    spdlog::info("NHT_COsContext_SetControllerID_Hook: controltype {} -> {}", Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, controllerType), bIsPS2controltype ? "PS2" : Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
    NHT_COsContext_SetControllerID(iLauncherConfigCtrlType);
}

static bool forcedLauncherShutdown = false;

static void Init_LauncherConfigOverride()
{
    // If we know games steam appid, try creating steam_appid.txt file, so that game EXE can be launched directly in future runs
    if (game)
    {
        const std::filesystem::path steamAppidPath = sExePath.parent_path() / "steam_appid.txt";

        try
        {
            if (!std::filesystem::exists(steamAppidPath))
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Creating steam_appid.txt to allow direct EXE launches.");
                std::ofstream steamAppidOut(steamAppidPath);
                if (steamAppidOut.is_open())
                {
                    steamAppidOut << game->SteamAppId;
                    steamAppidOut.close();
                }
                if (std::filesystem::exists(steamAppidPath))
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: steam_appid.txt created successfully.");
                }
                else
                {
                    spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: steam_appid.txt creation failed.");
                }
            }
        }
        catch (const std::exception& ex)
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launcher Config: steam_appid.txt creation failed (exception: %s)", ex.what());
        }
    }

    LPWSTR commandLine = GetCommandLineW();

    if (eGameType & LAUNCHER)
    {
        bool hasJumpstart = wcsstr(commandLine, L"-jump gamestart");

        if (bLauncherConfigSkipLauncher)
        {

            if (!hasJumpstart || Util::IsProcessParent("steam.exe"))
            {
                auto gameExePath = sExePath.parent_path() / game->ExeName;

                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher set, attempting game launch");

                PROCESS_INFORMATION processInfo {};
                STARTUPINFO startupInfo {};
                startupInfo.cb = sizeof(STARTUPINFO);

                std::wstring commandLine = L"\"" + gameExePath.wstring() + L"\"";

                if (game->ExeName == kGames.at(MG).ExeName)
                {
                    // Add launch parameters for MG MSX
                    auto transformString = [](const std::string& input, int (*transformation)(int)) -> std::wstring
                        {
                            std::string transformedString = input;
                            std::transform(transformedString.begin(), transformedString.end(), transformedString.begin(), transformation);
                            return Util::UTF8toWide(transformedString);
                        };

                    commandLine += L" -mgst " + std::wstring(sLauncherConfigMSXGame == ConfigKeys::SkipLauncherMSX_Option_MG1 ? L"mg1" : L"mg2"); // -mgst must be lowercase
                }

                commandLine += L" -region " + Util::UTF8toWide(sSkipLauncherRegion) + L" -lan " + Util::UTF8toWide(sSkipLauncherLanguage) + L" -selfregion EU -launcherpath launcher.exe" + L" -ctrltype " + Util::UTF8toWide(Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypesInternal, iLauncherConfigCtrlType));
                std::string sCommandLine = Util::WideToUTF8(commandLine);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launch command line: {}", sCommandLine);

                // Call CreateProcess to start the game process
                if (CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
                {
                    // Successfully started the process
                    CloseHandle(processInfo.hProcess);
                    CloseHandle(processInfo.hThread);

                    // Force launcher to exit
                    forcedLauncherShutdown = true;
                    spdlog::shutdown();
                    ExitProcess(EXIT_SUCCESS);
                }
                else
                {
                    spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: SkipLauncher failed to create game EXE process");
                }

            }
            else //hasJumpstart && bLauncherConfigSkipLauncher -> we reentered the launcher from the main game. lets terminate once the game finishes closing.
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher jumpstart detected on commandline.");
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Waiting for companion game to exit before terminating launcher.");
                while (Util::IsProcessRunning(sExePath / game->ExeName))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Companion game exited, exiting launcher.");
                forcedLauncherShutdown = true;
                spdlog::shutdown();
                ExitProcess(EXIT_SUCCESS);
            }
        }
        else if (bLauncherJumpStart)
        {
            if (!hasJumpstart)
            {

                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: JumpStart set, attempting to restart launcher with -jump gamestart");
                std::filesystem::path gameExePath = sExePath.parent_path() / "launcher.exe";

                PROCESS_INFORMATION processInfo = {};
                STARTUPINFO startupInfo = {};
                startupInfo.cb = sizeof(STARTUPINFO);
                std::wstring commandLine = L"\"" + gameExePath.wstring() + L"\"";
                commandLine += L" -jump gamestart";
                if (CreateProcess(nullptr, (LPWSTR)commandLine.c_str(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo))
                {
                    // Successfully started the process
                    CloseHandle(processInfo.hProcess);
                    CloseHandle(processInfo.hThread);

                    // Force launcher to exit
                    forcedLauncherShutdown = true;
                    spdlog::shutdown();
                    ExitProcess(EXIT_SUCCESS);
                }
                spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Failed to restart launcher with jumpstart.");
            }
            else
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Launcher Jumpstarted.");
            }
        }

        return;
    }


    // Certain config such as language/button style is normally passed from launcher to game via arguments
    // When game EXE gets ran directly this config is left at default (english game, xbox buttons)
    // If launcher argument isn't detected we'll allow defaults to be changed by hooking the engine functions responsible for them
    bool hasCtrltype = wcsstr(commandLine, L"-ctrltype") != nullptr;
    bool hasRegion = wcsstr(commandLine, L"-region") != nullptr;
    bool hasLang = wcsstr(commandLine, L"-lan") != nullptr;

    if (!engineModule)
    {
        spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: engineModule is null, cannot apply INI overrides for Region/Language/ControllerType");
    }

    if (!hasCtrltype || bIsPS2controltype)
    {
        NHT_COsContext_SetControllerID = decltype(NHT_COsContext_SetControllerID)(GetProcAddress(engineModule, "NHT_COsContext_SetControllerID"));
        if (NHT_COsContext_SetControllerID)
        {
            if (Memory::HookIAT(baseModule, "Engine.dll", NHT_COsContext_SetControllerID, NHT_COsContext_SetControllerID_Hook))
            {
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Overriding controller glyphs with: {} icons.", bIsPS2controltype ? "PS2" : Util::GetUppercaseNameAtIndex(kLauncherConfigCtrlTypes, iLauncherConfigCtrlType));
            }
            else
            {
                spdlog::error("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Failed to apply NHT_COsContext_SetControllerID IAT hook");
            }
        }
        else
        {
            spdlog::error("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to locate NHT_COsContext_SetControllerID export");
        }
        if (bIsPS2controltype)
        {
            if (uint8_t* PS4ControllerScan = Memory::PatternScan(baseModule, "6F 76 72 5F 73 74 6D 2F 63 74 72 6C 74 79 70 65 5F 70 73 34 2F", "PS4 Controller Glyphs"))
            {
                Memory::PatchBytes((uintptr_t)PS4ControllerScan, "\x6F\x76\x72\x5F\x73\x74\x6D\x2F\x63\x74\x72\x6C\x74\x79\x70\x65\x5F\x70\x73\x32\x2F", 21);
                spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: Patched PS4 controller glyphs to PS2 glyphs.");
            }
        }
    }
    else
    {
        spdlog::info("MG/MG2 | MGS 2 | MGS 3: Launcher Config: -ctrltype specified on command-line, skipping INI override");
    }

}


static bool DetectGame()
{
    eGameType = UNKNOWN;
    // Special handling for launcher.exe
    if (bIsLauncher)
    {
        for (const auto& [type, info] : kGames)
        {
            auto gamePath = sExePath.parent_path() / info.ExeName;
            if (std::filesystem::exists(gamePath))
            {
                spdlog::info("Detected launcher for game: {} (app {})", info.GameTitle.c_str(), info.SteamAppId);
                eGameType = LAUNCHER;
                unityPlayer = GetModuleHandleA("UnityPlayer.dll");
                game = &info;
                sGameSavePath = sExePath / ((game->ExeName == kGames.at(MG).ExeName) ? "mg12_savedata_win" : (game->ExeName == kGames.at(MGS2).ExeName) ? "mgs2_savedata_win" : "mgs3_savedata_win");
                return true;
            }
        }

        spdlog::error("Failed to detect supported game, unknown launcher");
        FreeLibraryAndExitThread(baseModule, 1);
    }

    for (const auto& [type, info] : kGames)
    {
        if (info.ExeName == sExeName)
        {
            spdlog::info("Detected game: {} (app {})", info.GameTitle.c_str(), info.SteamAppId);
            eGameType = type;
            game = &info;

            sGameSavePath = sExePath / (eGameType & MG ? "mg12_savedata_win" : eGameType & MGS2 ? "mgs2_savedata_win" : "mgs3_savedata_win");
            spdlog::info("Game Save Path: {}", sGameSavePath.string());
            if (engineModule = GetModuleHandleA("Engine.dll"); !engineModule)
            {
                spdlog::error("Failed to get Engine.dll module handle");
            }
            return true;
        }
    }

    spdlog::error("Failed to detect supported game, {} isn't supported by MGSHDFix", sExeName.c_str());
    FreeLibraryAndExitThread(baseModule, 1);
}

void AfterSteamInitialized()
{
    static bool bInitialized = false;
    if (bInitialized)
    {
        spdlog::warn("AfterSteamInitialized() called multiple times, skipping initialization.");
        return;
    }
    bInitialized = true;

    spdlog::info("AfterSteamInitialized() started");
    g_StatPersistence.OnSteamInitialized();
    spdlog::info("AfterSteamInitialized() completed");

}

void AfterSteamInputInitialized()
{
    static bool bInitialized = false;
    if (bInitialized)
    {
        spdlog::warn("AfterSteamInputInitialized() called multiple times, skipping initialization.");
        return;
    }
    bInitialized = true;

}


void afterPresent()
{
    static bool bInitialized = false;
    if (bInitialized)
    {
        spdlog::warn("afterPresent() called multiple times, skipping initialization.");
        return;
    }
    bInitialized = true;
    spdlog::info("afterPresent() started");
    if (!(eGameType & MG))
    {
        g_VectorScalingFix.LoadCompiledShader();
    }
    g_MuteWarning.CheckStatus();
    g_SteamAPI.OnSteamInputLoaded();

    if (eGameType & (MGS2|MGS3))
    {
        CaptionReplacements::InitializeCaptionOverrides();
        PhotoCamera::Initialize();
    }

    if (eGameType & MGS2)
    {
        MGS2_CodecBackground::Init();
        MGS2_ContrastShader::Init();
        MGS2_ShimmerEffect::Init();
        MGS2_Crossfade::Initialize();
        g_MGS2UnderwaterFilterFix.InstallD3D11StateHooks();
        MGS2_AiRayVision::Init();
    }
    else if (eGameType & MG)
    {
        MG1_DisplayScaling::Init();
    }
    ColorCorrection::Init();
    if (!(eGameType & MG))
    {
        SMAA_AA::Init();
    }
    D3D11TextOverlay::Init();
    spdlog::info("afterPresent() completed");
}

#if !defined(RELEASE_BUILD)

void ScanAndPatchSkybox();
#endif


static void InitializeSubsystems()
{
    //Initialization order (these systems initialize vars used by following ones.)
    INITIALIZE(g_Logging.LogSysInfo());            //0
    INITIALIZE(DetectGame());                      //1
    INITIALIZE(ASILoaderCompatibility::Check());   //2
    INITIALIZE(Config::Read());                    //3
    INITIALIZE(g_GameVars.Initialize());           //4
    INITIALIZE(g_D3D11Hooks.Initialize());         //5 Caches the D3DDevice, DXGIFactory, and D3DContext from D3DCreateDevice/DXGICreateFactory
    INITIALIZE(ReshadeCompatibility::Check());     //6 Dependent on ReadConfig, must also be before LauncherConfigOverride to warn the user before a crash.
    INITIALIZE(Init_LauncherConfigOverride());     //7
    INITIALIZE(CustomResolutionAndBorderless::Init_FixDPIScaling());              //8 Needs to be anywhere before the window is created in CustomResolution.
    INITIALIZE(CustomResolutionAndBorderless::Init_CalculateScreenSize());        //9
    INITIALIZE(CustomResolutionAndBorderless::Init_CustomResolution());           //10

    INITIALIZE(CustomResolutionAndBorderless::Init_ScaleEffects());               
    INITIALIZE(CustomResolutionAndBorderless::Init_AspectFOVFix());
    INITIALIZE(CustomResolutionAndBorderless::Init_HUDFix());
    INITIALIZE(Init_Miscellaneous());
    INITIALIZE(BP_FilesysChanges::Initialize()); //keep this early. liveswaps & other asset replacements need to check its state.

        //Features
    //INITIALIZE(g_TextureBufferSize.Initialize());
    INITIALIZE(PressureInputs::Initialize());
    INITIALIZE(Ds3Rumble::Initialize());

    if (eGameType & MGS2)
    {
        INITIALIZE(MGS2_GlassDmapackOverflow::Initialize());
        INITIALIZE(g_MGS2Sunglasses.Initialize());
        INITIALIZE(MGS2_RestoreDogtags::Initialize());
        INITIALIZE(MGS2_RestoreOriginalDifficulty::Apply());
        INITIALIZE(MGS2_RestorePhoneJingle::Apply());
        INITIALIZE(MGS2_RestoreActionLevelSelection::Apply());
        INITIALIZE(MGS2_RestoreSoLRadar::Apply());
        INITIALIZE(MGS2_RestoreElevatorGlitch::Initialize());
        INITIALIZE(SwapMenuButtons::SetMenuButtonInputs());
        INITIALIZE(MGS2_ThirdPersonFreecam::Activate());
        INITIALIZE(MGS2_Hostage_Type_Easter_Egg::Force());
        INITIALIZE(MGS2_First_Person_View::Activate());
        INITIALIZE(MGS2RetroColonel::Initialize());
        INITIALIZE(MGS2_SnakeTalesRadar::Apply());
        INITIALIZE(MGS2ThermalGoggles::Setup());
        INITIALIZE(CustomPlayerName::Apply());
        INITIALIZE(MGS2VampFPVPunch::Apply());
    }
    INITIALIZE(g_PauseOnFocusLoss.Initialize());
    INITIALIZE(g_IntroSkip.Initialize());
    INITIALIZE(g_KeepAimingAfterFiring.Initialize());
    INITIALIZE(g_DistanceCulling.Initialize());
    INITIALIZE(OriginalCameraPositions::Activate());
    INITIALIZE(AdjustableCaptions::Apply());


    INITIALIZE(ColorCorrection::Setup());

        //Fixes
    if (eGameType & MGS2)
    {
        INITIALIZE(g_OpticalCamoFix.Initialize());
        INITIALIZE(FixAimingFullTilt::Initialize());
        INITIALIZE(MGS2BloodStains::Initialize());
        INITIALIZE(MGS2CameraOffsets::Initialize());
        INITIALIZE(MGS2DemoScope::Initialize());
        INITIALIZE(MGS2ScopeWarp::Initialize());
        INITIALIZE(MGS2WaterEffects::Initialize());
        INITIALIZE(MGS2CodecBand::Initialize());
        INITIALIZE(MGS2LensDroplets::Initialize());
        INITIALIZE(MGS2GasHaze::Initialize());
        INITIALIZE(MGS2DemoCameraJudder::Initialize());
        INITIALIZE(MGS2HairLayering::Initialize());
        INITIALIZE(MGS2RotorProcession::Initialize());
        INITIALIZE(MGS2TankerSnakeSnap::Initialize());
        INITIALIZE(FixReverbWetLevel::Initialize());
        INITIALIZE(MGS2RailgunBeam::InitializeEarly());
        INITIALIZE(MGS2DemoBlur::Initialize());
        INITIALIZE(MGS2FlareOcclusion::Initialize());
        INITIALIZE(CoolantMirrorFix::ApplyFix());
        INITIALIZE(ResolutionScalingFixes::ApplyFixes()); // Always load after custom resolution
        INITIALIZE(TextureLiveSwaps::ApplyFixes());
        INITIALIZE(SnakeArmFixes::ApplyFixes());
        INITIALIZE(MGS2_Kirari_Sun2Fix::ApplyFix());
        INITIALIZE(MGS2PreshadeLights::Initialize());
        INITIALIZE(MGS2_RestoreDogtagViewer::Restore());
        INITIALIZE(g_DepthOfFieldFixes.Initialize());
        INITIALIZE(g_MGS2UnderwaterFilterFix.Initialize());
        INITIALIZE(MGS2_ShimmerEffect::SetupHooks());
        INITIALIZE(MGS2_ParrotRadarFix::Apply());
        INITIALIZE(HostageModel::ApplyFix());
        INITIALIZE(MGS2RayPhotoVoice::Initialize());
        INITIALIZE(MGS2_CodecBackground::Setup());
        INITIALIZE(MGS2_ContrastShader::Setup());
        INITIALIZE(MGS2_AiRayVision::Setup());
        INITIALIZE(MGS2ConcentrateBlur::Initialize());
        INITIALIZE(MGS2EnhancedDemos::Initialize());
        INITIALIZE(CaptionReplacements::Setup());
        INITIALIZE(SMAA_AA::CompileShaders());
        INITIALIZE(ScreenspaceFixes::Apply());
    }
    else if (eGameType & MGS3)
    {
        INITIALIZE(ResolutionScalingFixes::ApplyFixes()); // Always load after custom resolution
        INITIALIZE(g_WaterReflectionFix.Initialize());
        INITIALIZE(MGS3HudFixes::Initialize());
        INITIALIZE(MGS3FilmGrain::Initialize());
        INITIALIZE(MGS3_CrossfadeCapture::Initialize());
        INITIALIZE(MGS3FixCameraOffset::Activate());
        INITIALIZE(g_DepthOfFieldFixes.Initialize());
        INITIALIZE(CaptionReplacements::Setup());
        INITIALIZE(SMAA_AA::CompileShaders());
        INITIALIZE(ScreenspaceFixes::Apply());

        MAKE_HOOK_MID(baseModule, "89 44 24 ?? 0F B7 05 ?? ?? ?? ?? 66 89 44 24 ?? 0F B6 05", "MGS 3: Hires Textures Enabled Check", {
                static bool printed = false;
                if (printed)
                {
                    return;
                }
                printed = true;
                spdlog::info("High Resolution Texture Pack is enabled.");
                      });
            
    }
    else if (eGameType & MG)
    {
        INITIALIZE(MG1_DisplayScaling::Setup());
    }
    INITIALIZE(g_CPUCoreLimitFix.ApplyFix());
    INITIALIZE(g_VectorScalingFix.Initialize());
    INITIALIZE(g_EffectSpeedFix.Initialize()); //todo - fix more effects, ie rain speed, bullet trails, helicopter rotors
    INITIALIZE(g_StereoAudioFix.Initialize());
    INITIALIZE(DamagedSaveFix::Initialize());
    INITIALIZE(g_FixAimAfterEquip.Initialize());
    INITIALIZE(FixFullscreenOptimization::Fix());
    INITIALIZE(HighPerformanceGpu::Fix());
    INITIALIZE(g_BusyLoopFix.Initialize());
    INITIALIZE(FixPlaytime::Apply());
    INITIALIZE(D3D11TextOverlay::Setup());



#if !defined(RELEASE_BUILD) //todo category

    INITIALIZE(CutscenePausing::Setup());

    //todo: Make ultrawide & 4:3 reposition HUD elements correctly instead of stretching them
    //INITIALIZE(MGS3FixHolster::Initialize());
    //INITIALIZE(MG1CropBorders::Initialize());
    //INITIALIZE(MG1CustomLoadingScreens::Initialize());
    //INITIALIZE(AntiAliasing::Initialize());
#endif

        //Warnings
    INITIALIZE(g_MuteWarning.Setup());
    INITIALIZE(BackgroundShuffleWarning::Check());
    INITIALIZE(CheckGamesaveFolderWritable::CheckStatus());
    INITIALIZE(BugfixMods::Check());


    if (!(eGameType & LAUNCHER))
    {
        INITIALIZE(g_SteamAPI.Setup());
        INITIALIZE(g_StatPersistence.Setup());
    }

    INITIALIZE(CheckForUpdates());

#if !defined(RELEASE_BUILD)
    INITIALIZE(UnitTests::runAllTests());

#endif
}

std::mutex mainThreadFinishedMutex;
std::condition_variable mainThreadFinishedVar;
bool mainThreadFinished = false;

DWORD __stdcall Main(void*)
{
    g_Logging.initStartTime = std::chrono::high_resolution_clock::now();
    Logging::Initialize();

    INITIALIZE(InitializeSubsystems());

    // Signal any threads (e.g., the memset hook) that are waiting for initialization to finish.
    {
        std::lock_guard lock(mainThreadFinishedMutex);
        mainThreadFinished = true;
    }
    mainThreadFinishedVar.notify_all();

    return TRUE;
}

std::mutex memsetHookMutex;
bool memsetHookCalled = false;
static void* (__cdecl* memset_Fn)(void* Dst, int Val, size_t Size); // Pointer to the next function in the memset chain (could be another hook or the real CRT memset).
static void* __cdecl memset_Hook(void* Dst, int Val, size_t Size) // Our memset hook, which blocks the game's main thread until initialization is complete.
{
    std::lock_guard lock(memsetHookMutex);

    if (!memsetHookCalled)
    {
        memsetHookCalled = true;

        // Restore the original (or previously-hooked) memset in the IAT.
        // This ensures future memset calls bypass our hook and run at full speed.
        Memory::WriteIAT(baseModule, "VCRUNTIME140.dll", "memset", memset_Fn);

        // Block the current thread here until our main initialization is complete.
        std::unique_lock finishedLock(mainThreadFinishedMutex);
        mainThreadFinishedVar.wait(finishedLock, []
            {
                return mainThreadFinished;
            });
    }

    // Forward the memset call to the next function (another hook or the real memset).
    return reinterpret_cast<decltype(memset_Fn)>(memset_Fn)(Dst, Val, Size);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (forcedLauncherShutdown)
    {
        return TRUE;
    }
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        if (GetModuleHandleA("VCRUNTIME140.dll"))
        {
            // Read the current IAT entry for memset in the base module.
            // Note: it may already point to another mod's hook if they loaded first.
            void* currentIATMemset = Memory::ReadIAT(baseModule, "VCRUNTIME140.dll", "memset");

            // Save the current pointer so we can call it later (chaining to the next hook or real memset).
            memset_Fn = reinterpret_cast<decltype(memset_Fn)>(currentIATMemset);

            // Overwrite the IAT entry with our memset_Hook, so our code intercepts memset calls.
            // We always overwrite unconditionally to ensure our hook is active.
            // This will prevent other mods that also hook memset from unpausing the main thread before our Main() has finished.
            Memory::WriteIAT(baseModule, "VCRUNTIME140.dll", "memset", &memset_Hook);
        }

        // Create our main thread, which runs the initialization logic.
        if (const HANDLE mainHandle = CreateThread(nullptr, 0, Main, nullptr, CREATE_SUSPENDED, nullptr))
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_TIME_CRITICAL); // Give our thread higher priority than the game's.
            ResumeThread(mainHandle);
            CloseHandle(mainHandle);
        }

        // Prevent monitor or system sleep while the game is running.
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH)
    {
        //spdlog::info("DLL_PROCESS_DETACH called, shutting down MGSHDFix.");
        g_StatPersistence.SaveStats();
        g_BusyLoopFix.Shutdown();
        Ds3Rumble::Shutdown();
        spdlog::shutdown();
    }
    return TRUE;
}
