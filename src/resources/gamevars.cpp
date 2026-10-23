#include "stdafx.h"
#include "common.hpp"
#include "gamevars.hpp"
#include "mg2_outdoor_tiles.hpp"
#include "mg2_truckyard_fence.hpp"
#include "mg2_elevators.hpp"

#include "d3d11_text_overlay.hpp"
#include "depth_of_field.hpp"
#include "distance_culling.hpp"
#include "game_funcs.hpp"
#include "keep_aiming_after_firing.hpp"
#include "logging.hpp"
#include "mgs2_3rd_person_freecam.hpp"
#include "mgs2_first_person_view_mode.hpp"
#include "mgs2_rotor_procession.hpp"
#include "mgs2_tanker_fog.hpp"
#include "mgs2_title_lightning.hpp"
#include "mgs2_override_probe_cache.hpp"
#include "mgs2_area_prefetch.hpp"
#include "mgs2_sunglasses.hpp"
#include "mgs2_vamp_punch_fix.hpp"
#include "mgs3_linkvarbuf.hpp"
#include "resolution_scaling_fixes.hpp"
#include "stat_persistence.hpp"


void GameVars::Initialize()
{
    if (eGameType & MGS2)
    {
        aimingState = reinterpret_cast<uint64_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 2D ?? ?? ?? ?? F3 0F 10 1D", "MGS 2: GameVars: aimingState") + 2)); // +17DF86C 2.0.1
        cutsceneFlag = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 45 33 F6 8B 0D", "MGS 2: GameVars: cutsceneFlag") + 2));
        scriptedSequenceFlag = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "44 39 35 ?? ?? ?? ?? 89 15", "MGS 2: GameVars: scriptedSequenceFlag") + 3));
        actorWaitValue = reinterpret_cast<double*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "66 0F 2F 05 ?? ?? ?? ?? 73 ?? 33 C0", "MGS 2: GameVars: actorWaitValue") + 4));
        currentStage = reinterpret_cast<char const*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "4C 8D 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 4C 8D 05", "MGS 2: GameVars: currentStage") + 3));
        if (uint8_t* region = Memory::PatternScan(baseModule, "4C 63 05 ?? ?? ?? ?? 48 8D 1D", "MGS 2: GameVars: region index"))
        {
            p_RegionIndex = reinterpret_cast<int*>(Memory::GetRelativeOffset(region + 3));
        }
        if (uint8_t* dirs = Memory::PatternScan(baseModule, "48 8D 1D ?? ?? ?? ?? 4C 8B CF", "MGS 2: GameVars: region dirs"))
        {
            p_RegionDirs = reinterpret_cast<const char* const*>(Memory::GetRelativeOffset(dirs + 3));
        }

        GM_WaterLevel = reinterpret_cast<float*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F3 0F 5C 05 ?? ?? ?? ?? F3 41 0F 5C F3", "MGS 2: GameVars: GM_WaterLevel") + 4));

        heldTriggers = reinterpret_cast<uint32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F2 0F 11 05 ?? ?? ?? ?? 0F 11 0D", "MGS 2: GameVars: heldTriggers") + 4));
        GM_PlayerStatus = reinterpret_cast<uint64_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? 48 23 C1 C3", "MGS2: GM_PlayerStatus") + 3));
        p_GM_MenuStatus = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "0B 05 ?? ?? ?? ?? 0F BA E0 ?? 72 ?? E8 ?? ?? ?? ?? E8", "MGS2: p_GM_MenuStatus") + 2));
        GM_GameStatus = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "0B 05 ?? ?? ?? ?? 8B CB", "MGS2: GM_GameStatus") + 2));
        GM_VRStatus = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 8B DA 83 E0", "MGS2: GM_VRStatus") + 2));
        p_GV_PauseLevel= reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? A8 ?? 74 ?? A8 ?? 75", "MGS2: p_GV_PauseLevel") + 2));
        MGS2_LinkVarBuf::linkvarbuf = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8B 0D ?? ?? ?? ?? 89 81 ?? ?? ?? ?? 48 8B CB", "MGS2: LinkVarBuf") + 3));
        p_DG_Clock = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "2B 0D ?? ?? ?? ?? E8", "MGS2: DG_Clock") + 2));

        // system/libgcl/parse.c -> GCL_GetOption() reads the line and leaves the cursor behind it.
        p_GCL_CommandLine = reinterpret_cast<char***>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? 4C 8D 44 24 ?? 0F BE D9", "MGS2: GCL_CommandLine") + 3));
        p_GCL_NextStrPtr = reinterpret_cast<char**>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "48 8B 44 24 ?? 48 89 05 ?? ?? ?? ?? 48 83 C4", "MGS2: GCL_NextStrPtr") + 5, 3, 7));

        // system/libdg/frame.cpp -> DG_StartFrame() loads both globals before its undraw test.
        p_DG_UnDrawFrameCount64 = reinterpret_cast<int64_t*>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "48 8B 0D ?? ?? ?? ?? BF", "MGS2: DG_UnDrawFrameCount64"), 3, 7));
        p_DG_LastWhich = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 8B DF", "MGS2: DG_LastWhich"), 2, 6));
        p_DG_ObjQueueInit = Memory::PatternScan(baseModule, "48 C7 05 ?? ?? ?? ?? 00 03 00 00", "MGS 2: GameVars: DG_InitChanlSystem() -> DG_ObjQueue init");
        p_DG_Chanls = reinterpret_cast<DG_CHANL*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B CB", "MGS2: DG_Chanls") + 3));
        p_GM_CurrentStageMap = reinterpret_cast<int32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 1D ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B F8", "MGS2: GM_CurrentStageMap") + 2));
        p_HZX_CurrentGroupID = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 1D ?? ?? ?? ?? 4C 8D 44 24 ?? 33 ED 48 8D 54 24 ?? 89 2D", "MGS 2: GameVars: HZX_CurrentGroupID") + 2));

        p_GM_CameraDir = reinterpret_cast<SVECTOR*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8D 15 ?? ?? ?? ?? 83 25", "MGS 2: GameVars: GM_CameraDir") + 3));
        p_GM_PlayerWork = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? 48 8B 88 ?? ?? ?? ?? 0F BF 51 ?? 75", "MGS 2: GameVars: GM_PlayerWork") + 3));
        p_GM_PlayerBody = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F 10 49", "MGS 2: GameVars: GM_PlayerBody") + 3));
        p_ArmCamShift = reinterpret_cast<FVECTOR*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 10 87 ?? ?? ?? ?? F3 0F 5C 44 24", "MGS 2: GameVars: ArmCamShift") + 3));
        p_GM_CameraTarget = reinterpret_cast<FVECTOR*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8D 15 ?? ?? ?? ?? 48 8B F9 4D 89 73", "MGS 2: GameVars: GM_CameraTarget") + 3));

        if (uint8_t* ThirdPersonCameraHSpeed_Scan = Memory::PatternScan(baseModule, "F3 0F 59 05 ?? ?? ?? ?? F3 0F 2C F8 66 29 3D", "MGS 2: GameVars: game\\camera.c | gBP_3rdPersonCamera_HSpeed | @ L51"))
        {
            p_gBP_3rdPersonCamera_HSpeed = reinterpret_cast<float*>(Memory::GetRipRelativeAddress(ThirdPersonCameraHSpeed_Scan, 4, 8));
        }
        if (uint8_t* ThirdPersonCameraRot_Scan = Memory::PatternScan(baseModule, "0F B7 15 ?? ?? ?? ?? 0F BF CA", "MGS 2: GameVars: game\\camera.c | gBP_3rdPersonCamera_Rot | @ L49"))
        {
            p_gBP_3rdPersonCamera_Rot = reinterpret_cast<SVECTOR*>(Memory::GetRipRelativeAddress(ThirdPersonCameraRot_Scan, 3, 7));
        }
        if (uint8_t* ThirdPersonCameraVSpeed_Scan = Memory::PatternScan(baseModule, "F3 0F 59 05 ?? ?? ?? ?? F3 0F 2C C0 EB ?? 8B C7", "MGS 2: GameVars: game\\camera.c | gBP_3rdPersonCamera_VSpeed | @ L52"))
        {
            p_gBP_3rdPersonCamera_VSpeed = reinterpret_cast<float*>(Memory::GetRipRelativeAddress(ThirdPersonCameraVSpeed_Scan, 4, 8));
        }
        if (uint8_t* ThirdPersonCameraOverride_Scan = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 0F 28 74 24", "MGS 2: GameVars: game\\camera.c | gBP_3rdPersonCamera_Override | @ L46"))
        {
            p_gBP_3rdPersonCamera_Override = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(ThirdPersonCameraOverride_Scan, 2, 7));
        }
        if (uint8_t* ThirdPersonCameraDist_Scan = Memory::PatternScan(baseModule, "4C 8D 0D ?? ?? ?? ?? F3 0F 11 05", "MGS 2: GameVars: game\\camera.c | gBP_3rdPersonCamera_Dist | @ L50"))
        {
            p_gBP_3rdPersonCamera_Dist = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(ThirdPersonCameraDist_Scan, 3, 7));
        }
        if (uint8_t* CameraInheritRot_Scan = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 48 8B D9 74 ?? 48 8B 05", "MGS 2: GameVars: user\\sonoyama\\raiden\\routine.c | PL_IntoSubject() | @ L33"))
        {
            p_gBP_Camera_InheritRot = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(CameraInheritRot_Scan, 2, 7));
        }

        
        spdlog::info("GameVars: GM_GameStatus address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_GameStatus - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_MenuStatus address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GM_MenuStatus - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_PlayerStatus address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_PlayerStatus - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_VRStatus address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_VRStatus - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_WaterLevel address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_WaterLevel - (uintptr_t)baseModule);
        spdlog::info("GameVars: GV_PauseLevel address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GV_PauseLevel - (uintptr_t)baseModule);
        spdlog::info("GameVars: actorWaitValue address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)actorWaitValue - (uintptr_t)baseModule);
        spdlog::info("GameVars: aimingState address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)aimingState - (uintptr_t)baseModule);
        spdlog::info("GameVars: currentStage address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)currentStage - (uintptr_t)baseModule);
        spdlog::info("GameVars: GCL_CommandLine address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GCL_CommandLine - (uintptr_t)baseModule);
        spdlog::info("GameVars: GCL_NextStrPtr address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GCL_NextStrPtr - (uintptr_t)baseModule);
        spdlog::info("GameVars: cutsceneFlag address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)cutsceneFlag - (uintptr_t)baseModule);
        spdlog::info("GameVars: heldTriggers address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)heldTriggers - (uintptr_t)baseModule);
        spdlog::info("GameVars: linkvarbuf address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS2_LinkVarBuf::linkvarbuf - (uintptr_t)baseModule);
        spdlog::info("GameVars: scriptedSequenceFlag address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)scriptedSequenceFlag - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_Clock address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_Clock - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_LastWhich address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_LastWhich - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_UnDrawFrameCount address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_UnDrawFrameCount64 - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_Chanls address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_Chanls - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_CurrentStageMap address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GM_CurrentStageMap - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_CameraDir address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GM_CameraDir - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_PlayerWork address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GM_PlayerWork - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_PlayerBody address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GM_PlayerBody - (uintptr_t)baseModule);
        spdlog::info("GameVars: HZX_CurrentGroupID address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_HZX_CurrentGroupID - (uintptr_t)baseModule);
        spdlog::info("GameVars: gBP_3rdPersonCamera_Override address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_3rdPersonCamera_Override - (uintptr_t)baseModule);
        spdlog::info("GameVars: gBP_3rdPersonCamera_Rot address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_3rdPersonCamera_Rot - (uintptr_t)baseModule);
        spdlog::info("GameVars: gBP_3rdPersonCamera_Dist address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_3rdPersonCamera_Dist - (uintptr_t)baseModule);
        spdlog::info("GameVars: gBP_3rdPersonCamera_HSpeed address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_3rdPersonCamera_HSpeed - (uintptr_t)baseModule);
        spdlog::info("GameVars: gBP_3rdPersonCamera_VSpeed address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_3rdPersonCamera_VSpeed - (uintptr_t)baseModule);
        spdlog::info("GameVars: gBP_Camera_InheritRot address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_Camera_InheritRot - (uintptr_t)baseModule);

        if (uint8_t* LevelTransitionResult = Memory::PatternScan(baseModule, "89 73 ?? 81 25", "GameVars: Level Transition"))
        {
            static SafetyHookMid levelTransitionMidHook {};
            levelTransitionMidHook = safetyhook::create_mid(LevelTransitionResult,
                [](SafetyHookContext& ctx)
                {
                    OnLevelTransition();
                });
            LOG_HOOK(levelTransitionMidHook, "GameVars: Level Transition")
        }
    }
    else if (eGameType & MGS3)
    {
        scriptedSequenceFlag = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "44 39 35 ?? ?? ?? ?? 89 15", "MGS 3: GameVars: scriptedSequenceFlag") + 3));
        cutsceneFlag = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 45 33 F6 8B 0D", "MGS 3: GameVars: cutsceneFlag") + 2));
        aimingState = reinterpret_cast<uint64_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 35 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 49 89 8D", "MGS 3: GameVars: aimingState") + 2));
        heldTriggers = reinterpret_cast<uint32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8D 3D ?? ?? ?? ?? BA ?? ?? ?? ?? 41 B8", "MGS 3: GameVars: heldTriggers") + 3));
        actorWaitValue = reinterpret_cast<double*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 ?? ?? F2 0F 10 0D", "MGS 3: GameVars: actorWaitValue") + 13));
        currentStage = reinterpret_cast<char const*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "4C 8D 0D ?? ?? ?? ?? 48 C1 F8", "MGS 3: GameVars: currentStage") + 3));
        MGS3_LinkVarBuf::linkvarbuf = reinterpret_cast<uintptr_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "4C 8B 05 ?? ?? ?? ?? 66 41 83 78 ?? 00", "MGS3: LinkVarBuf") + 3));
        p_GV_PauseLevel = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "85 05 ?? ?? ?? ?? 8B 2D", "MGS3: p_GV_PauseLevel") + 2));
        GM_GameStatus = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "0B 05 ?? ?? ?? ?? 0F BA E0 ?? 72 ?? 8B CE", "MGS3: GM_GameStatus") + 2));
        p_GM_MenuStatus = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "0B 05 ?? ?? ?? ?? A8 ?? 74 ?? 66 23 CF", "MGS3: p_GM_MenuStatus") + 2));
        p_DG_Clock = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "2B 3D ?? ?? ?? ?? 89 3D", "MGS3: DG_Clock") + 2));

        // system/libdg/frame.cpp -> DG_StartFrame() loads both globals before its undraw test.
        p_DG_UnDrawFrameCount32 = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "8B 0D ?? ?? ?? ?? BB ?? ?? ?? ?? 8B 05", "MGS3: DG_UnDrawFrameCount32"), 2, 6));
        p_DG_LastWhich = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 8B FB", "MGS3: DG_LastWhich"), 2, 6));
        if (uint8_t* KeyboardMouseInputActive_Scan = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 74 ?? 83 3D ?? ?? ?? ?? 00 74 ?? B8 ?? ?? ?? ?? C3", "MGS 3: GameVars | Keyboard/mouse input active"))
        {
            p_KeyboardMouseInputActive = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(KeyboardMouseInputActive_Scan, 2, 7));
        }
        if (uint8_t* MouseMovementActive_Scan = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 74 ?? B8 ?? ?? ?? ?? C3 33 C0", "MGS 3: GameVars | Mouse movement active"))
        {
            p_MouseMovementActive = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(MouseMovementActive_Scan, 2, 7));
        }
        spdlog::info("GameVars: KeyboardMouseInputActive address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_KeyboardMouseInputActive - (uintptr_t)baseModule);
        spdlog::info("GameVars: MouseMovementActive address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_MouseMovementActive - (uintptr_t)baseModule);
#if defined(MGS3_FPS_DEV)
        if (uint8_t* FirstPersonCameraEnableMovement_Scan = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 75 ?? B9 BB 00 00 00 E8 ?? ?? ?? ?? 85 C0 0F 84", "MGS 3: GameVars: bp\\shared\\BP_Camera.cpp | gBP_1stPersonCamera_EnableMovement | @ L1086"))
        {
            p_gBP_1stPersonCamera_EnableMovement = reinterpret_cast<int32_t*>(Memory::GetRipRelativeAddress(FirstPersonCameraEnableMovement_Scan, 2, 7));
        }
        spdlog::info("GameVars: gBP_1stPersonCamera_EnableMovement address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_gBP_1stPersonCamera_EnableMovement - (uintptr_t)baseModule);
#endif

        spdlog::info("GameVars: cutsceneFlag address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)cutsceneFlag - (uintptr_t)baseModule);
        spdlog::info("GameVars: scriptedSequenceFlag address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)scriptedSequenceFlag - (uintptr_t)baseModule);
        spdlog::info("GameVars: actorWaitValue address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)actorWaitValue - (uintptr_t)baseModule);
        spdlog::info("GameVars: aimingState address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)aimingState - (uintptr_t)baseModule);
        spdlog::info("GameVars: heldTriggers address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)heldTriggers - (uintptr_t)baseModule);
        spdlog::info("GameVars: currentStage address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)currentStage - (uintptr_t)baseModule);
        spdlog::info("GameVars: linkvarbuf address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)MGS3_LinkVarBuf::linkvarbuf - (uintptr_t)baseModule);
        spdlog::info("GameVars: GV_PauseLevel address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GV_PauseLevel - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_GameStatus address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_GameStatus - (uintptr_t)baseModule);
        spdlog::info("GameVars: GM_MenuStatus address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_GM_MenuStatus - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_Clock address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_Clock - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_LastWhich address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_LastWhich - (uintptr_t)baseModule);
        spdlog::info("GameVars: DG_UnDrawFrameCount address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)p_DG_UnDrawFrameCount32 - (uintptr_t)baseModule);

        if (uint8_t* LevelTransitionResult = Memory::PatternScan(baseModule, "89 5F ?? E9 ?? ?? ?? ?? 39 1D", "GameVars: Level Transition"))
        {
            static SafetyHookMid levelTransitionMidHook {};
            levelTransitionMidHook = safetyhook::create_mid(LevelTransitionResult,
                [](SafetyHookContext& ctx)
                {
                    OnLevelTransition();
                });
            LOG_HOOK(levelTransitionMidHook, "GameVars: Level Transition")
        }
    }

    if (eGameType & MG)
    {
        //sub_1400D6F80
        auto* pSaveLoad = Memory::PatternScan(baseModule, "48 8B 05 ?? ?? ?? ?? 48 8B 90 ?? ?? ?? ?? 48 8B CA", "MG1/2: GameVars : save/load menu state");
        if (pSaveLoad)
        {
            pp_MGWrapperActor = reinterpret_cast<uint8_t**>(Memory::GetRelativeOffset(pSaveLoad + 3));
            spdlog::info("MG1/2: GameVars : Initialized save/load menu state.");
        }
        else
        {
            spdlog::error("MG1/2: GameVars : Save/load menu state unavailable.");
        }
    }

    if (eGameType & (MG|MGS2|MGS3))
    {
        Shared_Gamefuncs::HookFuncs();
    }
}

bool GameVars::MGIsSaveLoadVisible() const
{
    const auto* pActor = pp_MGWrapperActor ? *pp_MGWrapperActor : nullptr;
    constexpr size_t nSaveLoadActiveOffset = 0xC8;
    if (!pActor || *reinterpret_cast<const int*>(pActor + nSaveLoadActiveOffset) == 0)
    {
        return false;
    }
    constexpr size_t nDisplayHandleOffset = 0x90;
    constexpr size_t nActorHandleOffset = 0x28;
    constexpr size_t nDisplayBrightnessOffset = 0x1908;
    const auto hDisplay = *reinterpret_cast<const uintptr_t*>(pActor + nDisplayHandleOffset);
    if (!hDisplay)
    {
        return false;
    }
    const auto* pDisplay = reinterpret_cast<const uint8_t*>(hDisplay * 16);
    return *reinterpret_cast<const uintptr_t*>(pDisplay + nActorHandleOffset) == hDisplay && *reinterpret_cast<const float*>(pDisplay + nDisplayBrightnessOffset) <= 0.0f;
}

bool GameVars::InCutscene() const //does not count pad demos or FMVs, only full cutscenes.
{
    return cutsceneFlag == nullptr ? false : (*cutsceneFlag == 1);
}

bool GameVars::InScriptedSequence() const //Scripted sequences, i.e., forced codec calls, cutscenes, pad demos (ingame tutorials & forced movements ala first meeting Stillman), and FMVs.
{
    return scriptedSequenceFlag == nullptr ? false : (*scriptedSequenceFlag == 1);
}

double GameVars::ActorWaitValue() const
{
    return actorWaitValue == nullptr ? 1.0 / 60.0 : *actorWaitValue;
}


float GameVars::get_GM_WaterLevel() const
{
    return GM_WaterLevel == nullptr ? 0.0f : *GM_WaterLevel;
}

double GameVars::ActorWaitMultiplier() const
{
    return actorWaitValue == nullptr ? 1.0 : ((1.0/60) / *actorWaitValue);
}

const char* GameVars::MGS2_RegionDir() const
{
    if (!p_RegionIndex || !p_RegionDirs || *p_RegionIndex < 0 || *p_RegionIndex > 2)
    {
        return "eu";
    }
    return p_RegionDirs[*p_RegionIndex];
}

const char* GameVars::GetCurrentStage() const
{
    return currentStage == nullptr ? "nullptr" : currentStage;
}


void GameVars::SetAimingState(const uint64_t state) const
{
    if (aimingState != nullptr)
    {
        *aimingState = state;
        return;
    }
    spdlog::error("GameVars: SetAimingState: aimingState is null!");
}

uint64_t GameVars::GetAimingState() const
{
    if (aimingState != nullptr)
    {
        return *aimingState;
    }
    spdlog::error("GameVars: GetAimingState: aimingState is null!");
    return 0;
}


bool GameVars::MGS2IsHoldingWeaponMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_WeaponMenu) != 0;
}
bool GameVars::MGS2IsHoldingEquipmentMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_EquipmentMenu) != 0;
}
bool GameVars::MGS2IsHoldingFirstPerson() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_FirstPerson) != 0;
}
bool GameVars::MGS2IsHoldingLockOn() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_LockOn) != 0;
}

bool GameVars::MGS3IsHoldingEquipmentMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_EquipmentMenu) != 0;
}

bool GameVars::MGS3IsHoldingWeaponMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_WeaponMenu) != 0;
}

bool GameVars::MGS3IsHoldingLockOn() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_LockOn) != 0;
}

bool GameVars::MGS3IsHoldingFirstPerson() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_FirstPerson) != 0;
}


void GameVars::OnLevelTransition()
{


    g_StatPersistence.SaveStats();
    

    if (eGameType & MGS2)
    {
        KeepAimingAfterFiring::HandleLevelTransition();
        MGS2_First_Person_View::HandleLevelTransition();
        MGS2_ThirdPersonFreecam::HandleLevelTransition();
        MGS2Sunglasses::CheckOnTransition();
        //g_DepthOfFieldFixes.HandleLevelTransition();
        MGS2VampFPVPunch::HandleLevelTransition();
        ResolutionScalingFixes::HandleLevelTransition();
        D3D11TextOverlay::HandleLevelTransition();
        MGS2RotorProcession::HandleLevelTransition();
        MGS2TankerFog::HandleLevelTransition();
        MGS2_TitleLightning::HandleLevelTransition();
        MGS2_OverrideProbeCache::HandleLevelTransition();
        MGS2_AreaPrefetch::HandleLevelTransition();
        DistanceCulling::HandleLevelTransition();


    }
    else if (eGameType & MGS3)
    {
        KeepAimingAfterFiring::HandleLevelTransition();
        D3D11TextOverlay::HandleLevelTransition();
    }
    else if (eGameType & MG)
    {
        MG2_OutdoorTiles::HandleLevelTransition();
        MG2_TruckyardFence::HandleLevelTransition();
        MG2_Elevators::HandleLevelTransition();
    }

}


namespace
{
#define X(name, id, mode, disp) { id, mode, disp },

    inline const Stage g_StagesMGS2[] = { MGS2_STAGE_LIST };
    inline const Stage g_StagesMGS3[] = { MGS3_STAGE_LIST };

#undef X

    const Stage* FindStageByName(const char* currentStage)
    {
        if (!currentStage)
            return nullptr;

        // eGameType is always valid (exactly one of {MGS2, MGS3})
        const Stage* begin = nullptr;
        const Stage* end = nullptr;

        if (eGameType & MGS2)
        {
            begin = std::begin(g_StagesMGS2);
            end = std::end(g_StagesMGS2);
        }
        else if (eGameType & MGS3)
        {
            begin = std::begin(g_StagesMGS3);
            end = std::end(g_StagesMGS3);
        }

        for (auto it = begin; it != end; ++it)
        {
            if (_stricmp(it->sStageId, currentStage) == 0)
                return &(*it);
        }

        return nullptr;
    }
}

std::string GameVars::GetRichPresenceString() const
{
    const char* currentStage = GetCurrentStage();
    const Stage* s = FindStageByName(currentStage);

    if (s == nullptr)
    {
        if (currentStage == nullptr)
        {
            return "Unknown Stage";
        }
        return std::string("Unknown Stage (") + currentStage + ")";
    }

    // VR formatting uses " | ", everything else uses ": "
    const bool isVR = (std::strncmp(s->sGameMode, "VR:", 3) == 0);
    std::string result;

    if (isVR)
    {
        result = std::string(s->sGameMode) + " | " + s->sRichPresenceName;
    }
    else
    {
        result = std::string(s->sGameMode) + ": " + s->sRichPresenceName;
    }

    if (InCutscene())
    {
        result += " - In Cutscene";
    }

    return result;
}

std::string GameVars::GetGameMode() const
{
    const char* currentStage = GetCurrentStage();
    const Stage* s = FindStageByName(currentStage);

    if (s != nullptr)
    {
        return s->sGameMode;
    }

    if (currentStage == nullptr)
    {
        return "Unknown";
    }

    return std::string("Unknown (") + currentStage + ")";
}

MGS2GameMode GameVars::MGS2_GetGameMode() const
{
    const Stage* s = FindStageByName(GetCurrentStage());

    if (s == nullptr || s->sGameMode == nullptr)
    {
        return MGS2GameMode::Unknown;
    }

    if (_stricmp(s->sGameMode, "Menu") == 0)
    {
        return MGS2GameMode::Menu;
    }

    if (_stricmp(s->sGameMode, "Tanker") == 0)
    {
        return MGS2GameMode::Tanker;
    }

    if (_stricmp(s->sGameMode, "Plant") == 0)
    {
        return MGS2GameMode::Plant;
    }

    if (_stricmp(s->sGameMode, "Snake Tales") == 0)
    {
        return MGS2GameMode::SnakeTales;
    }

    if (_stricmp(s->sGameMode, "Alternate") == 0)
    {
        return MGS2GameMode::Alternate;
    }

    if (_stricmp(s->sGameMode, "VR: Sneaking") == 0)
    {
        return MGS2GameMode::VRSneaking;
    }

    if (_stricmp(s->sGameMode, "VR: Variety") == 0)
    {
        return MGS2GameMode::VRVariety;
    }

    if (_stricmp(s->sGameMode, "VR: First-Person") == 0)
    {
        return MGS2GameMode::VRFirstPerson;
    }

    if (_stricmp(s->sGameMode, "VR: Streaking") == 0)
    {
        return MGS2GameMode::VRStreaking;
    }

    if (_strnicmp(s->sGameMode, "VR: Weapons", 11) == 0)
    {
        return MGS2GameMode::VRWeapons;
    }

    spdlog::warn("Unknown MGS2 game mode: {}", s->sGameMode);

    return MGS2GameMode::Unknown;
}

// The stage table calls the cutscene stages Tanker/Plant too, so their 'd' prefix is still the tell.
bool GameVars::MGS2_IsPlayableStage() const
{
    const char* current = GetCurrentStage();
    if (current == nullptr || current[0] == '\0' || current[0] == 'd')
    {
        return false;
    }

    const MGS2GameMode mode = MGS2_GetGameMode();
    return mode != MGS2GameMode::Unknown && mode != MGS2GameMode::Menu;
}

MGS3GameMode GameVars::MGS3_GetGameMode() const
{
    const Stage* s = FindStageByName(GetCurrentStage());

    if (s == nullptr || s->sGameMode == nullptr)
    {
        return MGS3GameMode::Unknown;
    }

    if (_stricmp(s->sGameMode, "Menu") == 0)
    {
        return MGS3GameMode::Menu;
    }

    if (_stricmp(s->sGameMode, "Virtuous Mission") == 0)
    {
        return MGS3GameMode::VirtuousMission;
    }

    if (_stricmp(s->sGameMode, "Snake Eater") == 0)
    {
        return MGS3GameMode::SnakeEater;
    }

    spdlog::warn("Unknown MGS3 game mode: {}", s->sGameMode);

    return MGS3GameMode::Unknown;
}

bool GameVars::IsStage(const char* stageConst) const
{
    const char* current = GetCurrentStage();
    if (current == nullptr)
    {
        spdlog::error("IsStage() called with null current stage pointer");
        return false;
    }

    return _stricmp(current, stageConst) == 0;
}

bool GameVars::IsAnyStage(std::initializer_list<const char*> stages) const
{
    const char* current = GetCurrentStage();
    if (current == nullptr)
    {
        spdlog::error("IsAnyStage() called with null current stage pointer");
        return false;
    }

    for (const char* s : stages)
    {
        if (_stricmp(current, s) == 0)
        {
            return true;
        }
    }
    return false;
}

uint64_t GameVars::Get_PL_Status() const
{
    return GM_PlayerStatus == nullptr ? 0 : *GM_PlayerStatus;
}

void GameVars::Unset_PL_Status(uint64_t bits) const
{
    if (GM_PlayerStatus != nullptr)
        *GM_PlayerStatus &= ~bits;
}

int GameVars::Get_GM_GameStatus() const
{
    return GM_GameStatus == nullptr ? 0 : *GM_GameStatus;
}

int GameVars::Get_GM_VRStatus() const
{
    return GM_VRStatus == nullptr ? 0 : *GM_VRStatus;
}


int& GameVars::GV_PauseLevel() const
{
    static int dummyPauseLevel = 0;

    if (p_GV_PauseLevel== nullptr)
    {
        return dummyPauseLevel;
    }

    return *p_GV_PauseLevel;
}


int& GameVars::GM_MenuStatus() const
{
    static int dummyGM_MenuStatus = 0;

    if (p_GM_MenuStatus == nullptr)
    {
        return dummyGM_MenuStatus;
    }
    return *p_GM_MenuStatus;
}

int& GameVars::DG_Clock() const
{
    static int dummyDG_Clock = 0;
    if (p_DG_Clock == nullptr)
    {
        return dummyDG_Clock;
    }
    return *p_DG_Clock;
}

int& GameVars::DG_LastWhich() const
{
    static int dummyDG_LastWhich = -1; // -1 so a failed scan never holds.
    if (p_DG_LastWhich == nullptr)
    {
        return dummyDG_LastWhich;
    }
    return *p_DG_LastWhich;
}

int64_t GameVars::DG_UnDrawFrameCount() const
{
    if (p_DG_UnDrawFrameCount64 != nullptr)
    {
        return *p_DG_UnDrawFrameCount64;
    }
    if (p_DG_UnDrawFrameCount32 != nullptr)
    {
        return *p_DG_UnDrawFrameCount32;
    }
    return 0;
}
