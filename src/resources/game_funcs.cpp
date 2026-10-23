// ReSharper disable CppClangTidyReadabilityUseConcisePreprocessorDirectives
#include "stdafx.h"
#include "mg_developer_menu.hpp"

#include "common.hpp"
#include "logging.hpp"

#include "game_funcs.hpp"

#include "gamevars.hpp"
#include "mg2_linkvarbuf.hpp"
#include "mgs2_linkvarbuf.hpp"
#include "mgs3_linkvarbuf.hpp"
#include "input_handler.hpp"
/*
#if defined(RELEASE_BUILD)
#define RELEASE_CLEARED
#undef RELEASE_BUILD
#endif
*/
namespace
{
    int* GM_LoadRequest = nullptr;

#if !defined(RELEASE_BUILD)
    void* null_fn(int, int) { return nullptr; }

    void HookReturn(uint8_t* addr)
    {
        static SafetyHookInline hook;
        hook = safetyhook::create_inline(addr, null_fn);
    }

    using NewCharaCallback = std::function<void* (int, int)>;

    NewCharaCallback* pending = nullptr;

    std::unordered_map<std::string, std::unordered_map<uint32_t, NewCharaCallback>> return_hooks;

    struct ObserveEntry {
        std::function<void()> callback;
        SafetyHookMid hook;
    };

    std::unordered_map<std::string, std::unordered_map<uint32_t, ObserveEntry>> observe_hooks;

    SafetyHookInline GM_GetCharaID_hook;

    void* override_stub(int name, int map)
    {
        NewCharaCallback* cb = pending;
        pending = nullptr;
        if (cb && *cb) return (*cb)(name, map);
        return nullptr;
    }

    void* hooked_GM_GetCharaID(int nID)
    {
        if (const char* stage = Shared_Gamefuncs::GM_GetArea())
        {
            auto osit = observe_hooks.find(stage);
            if (osit != observe_hooks.end())
            {
                auto oiit = osit->second.find(static_cast<uint32_t>(nID));
                if (oiit != osit->second.end() && !oiit->second.hook)
                {
                    void* fn = GM_GetCharaID_hook.call<void*>(nID);
                    auto cb = oiit->second.callback;
                    static std::function<void()>* s_cb = nullptr;
                    s_cb = &oiit->second.callback;
                    oiit->second.hook = safetyhook::create_mid(fn, [](SafetyHookContext&) { if (s_cb) (*s_cb)(); });
                    return fn;
                }
            }

            auto sit = return_hooks.find(stage);
            if (sit != return_hooks.end())
            {
                auto iit = sit->second.find(static_cast<uint32_t>(nID));
                if (iit != sit->second.end())
                {
                    pending = &iit->second;
                    return reinterpret_cast<void*>(override_stub);
                }
            }
        }
        return GM_GetCharaID_hook.call<void*>(nID);
    }
#endif

    constexpr unsigned int STRCODE_SCENERIO_GCX = GameVars::GV_StrCode("scenerio");

    // Swap to the select stage under scenerio.gcx and flag a load, the same request the game's own
    // debug helper issues. Set by whichever game hooked, since the linkvars differ.
    void (*EnterDeveloperMenu)() = nullptr;

    void RegisterDevMenuHotkey()
    {
        if (!Shared_Gamefuncs::DevMenuHotkey || !EnterDeveloperMenu)
        {
            return;
        }

        g_InputHandler.RegisterHotkey(Shared_Gamefuncs::DevMenuHotkey, "Return to Developer Menu", []()
        {
            EnterDeveloperMenu();
        });
    }
}

namespace MGS2_GameFuncs
{

    /*
using NewItemChange_t = void* (__fastcall*)(int a1);
using NewItemChange2_t = void* (__fastcall*)(int a1);

static NewItemChange_t  NewItemChange = nullptr;
static NewItemChange2_t NewItemChange2 = nullptr;


    uint8_t* act154 = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 57 48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 33 FF 48 8B D9", "NewMenuPrimControl() -> Act_154");

    NewItemChange2 = reinterpret_cast<NewItemChange2_t>(ResolveCall(act154 + 0x5F));
    NewItemChange = reinterpret_cast<NewItemChange_t>(ResolveCall(act154 + 0xB6));
    */

}

void MGS2_GameFuncs::HookGameFuncs()
{
    using namespace Shared_Gamefuncs;
    using namespace MGS2_GameFuncs;
    using namespace MGS2_LinkVarBuf;
    using namespace MGS2Stages;
    spdlog::info("MGS2_GameFuncs: Hooking game functions.");
    GM_SeSet = reinterpret_cast<GM_SeSet_t>(Memory::PatternScan(baseModule, "83 F9 ?? 74 ?? ?? 83 E1", "GM_SeSet"));
    spdlog::info("MGS2_GameFuncs: GM_SeSet address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_SeSet - (uintptr_t)baseModule);

    DM_ExecDemoStream = Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 33 C0 48 8D 2D", "demo_pkt.c -> DM_ExecDemoStream()");
    BP_UpdatePreshadeBuffer = Memory::PatternScan(baseModule, "40 57 48 83 EC ?? 48 0F BF 41", "pshade.c -> BP_UpdatePreshadeBuffer()");

    uint8_t* GM_ItemNum_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 85 C0 7E ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB", "GM_ItemNum call site");
    GM_ItemNum = reinterpret_cast<GM_ItemNum_t>(Memory::ResolveCall(GM_ItemNum_scan));
    spdlog::info("MGS2_GameFuncs: GM_ItemNum address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_ItemNum - (uintptr_t)baseModule);

    uint8_t* GM_WeaponNum_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 85 C0 79 ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 0F 88", "GM_WeaponNum call site");
    GM_WeaponNum = reinterpret_cast<GM_WeaponNum_t>(Memory::ResolveCall(GM_WeaponNum_scan));
    spdlog::info("MGS2_GameFuncs: GM_WeaponNum address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_WeaponNum - (uintptr_t)baseModule);


    uint8_t* L2D_GetObject_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? BA ?? ?? ?? ?? 48 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? BA ?? ?? ?? ?? 48 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 8B", "L2D_GetObject call site");
    L2D_GetObject = reinterpret_cast<L2D_GetObject_t>(Memory::ResolveCall(L2D_GetObject_scan));
    spdlog::info("MGS2_GameFuncs: L2D_GetObject address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)L2D_GetObject - (uintptr_t)baseModule);

    uint8_t* L2D_GetParts_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 48 85 C0 74 ?? 8B 93 ?? ?? ?? ?? 0F 28 DE", "L2D_GetParts call site");
    L2D_GetParts = reinterpret_cast<L2D_GetParts_t>(Memory::ResolveCall(L2D_GetParts_scan));
    spdlog::info("MGS2_GameFuncs: L2D_GetParts address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)L2D_GetParts - (uintptr_t)baseModule);

    uint8_t* WriteString_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 41 B8", "WriteString call site");
    WriteString = reinterpret_cast<WriteString_t>(Memory::ResolveCall(WriteString_scan));
    spdlog::info("MGS2_GameFuncs: WriteString address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)WriteString - (uintptr_t)baseModule);

    uint8_t* GM_GetDGGroupID_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 48 8B CF 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? FF 8F ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 20 5F C3 48 83 C4 20 5F E9 ?? ?? ?? ?? CC 48 89 5C 24 ?? 57 48 83 EC 40 48 8B FA 0F 29 74 24 ?? 48 8B CF 0F 29 7C 24 ?? 49 8B D0 49 8B D8 E8 ?? ?? ?? ?? 48 C7 87 ?? ?? ?? ?? 44 00 00 00 0F 57 F6 F3 0F 10 3D ?? ?? ?? ?? F3 0F 10 2D ?? ?? ?? ?? F3 0F 10 25 ?? ?? ?? ?? F3 0F 10 1D ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 10 43 ?? F3 0F 59 C6 ?? ?? ?? ?? F3 0F 59 C7 F3 0F 2C C0 66 89 05 ?? ?? ?? ?? F3 0F 10 43 ?? F3 0F 59 C6 F3 0F 58 43 ?? C7 05 ?? ?? ?? ?? 00 10 FF 8F C6 05 ?? ?? ?? ?? FF C6 05 ?? ?? ?? ?? FF C6 05 ?? ?? ?? ?? FF F3 0F 59 C7 C6 05 ?? ?? ?? ?? 5A", "GM_GetDGGroupID call site");
    GM_GetDGGroupID = reinterpret_cast<GM_GetDGGroupID_t>(Memory::ResolveCall(GM_GetDGGroupID_scan));
    spdlog::info("MGS2_GameFuncs: GM_GetDGGroupID address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_GetDGGroupID - (uintptr_t)baseModule);

    uint8_t* UpdateVectors_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? FF 8F ?? ?? ?? ?? 48 8B 5C 24 ?? 48 83 C4 20 5F C3 48 83 C4 20 5F E9 ?? ?? ?? ?? CC 48 89 5C 24 ?? 57 48 83 EC 40 48 8B FA 0F 29 74 24 ?? 48 8B CF 0F 29 7C 24 ?? 49 8B D0 49 8B D8 E8 ?? ?? ?? ?? 48 C7 87 ?? ?? ?? ?? 44 00 00 00 0F 57 F6 F3 0F 10 3D ?? ?? ?? ?? F3 0F 10 2D ?? ?? ?? ?? F3 0F 10 25 ?? ?? ?? ?? F3 0F 10 1D ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 11 2D ?? ?? ?? ?? F3 0F 11 25 ?? ?? ?? ?? F3 0F 11 1D ?? ?? ?? ?? F3 0F 11 15 ?? ?? ?? ?? F3 0F 10 43 ?? F3 0F 59 C6 ?? ?? ?? ?? F3 0F 59 C7 F3 0F 2C C0 66 89 05 ?? ?? ?? ?? F3 0F 10 43 ?? F3 0F 59 C6 F3 0F 58 43 ?? C7 05 ?? ?? ?? ?? 00 10 FF 8F C6 05 ?? ?? ?? ?? FF C6 05 ?? ?? ?? ?? FF C6 05 ?? ?? ?? ?? FF F3 0F 59 C7 C6 05 ?? ?? ?? ?? 5A", "UpdateVectors call site");
    UpdateVectors_4 = reinterpret_cast<UpdateVectors_t>(Memory::ResolveCall(UpdateVectors_scan));
    spdlog::info("MGS2_GameFuncs: UpdateVectors address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)UpdateVectors_4 - (uintptr_t)baseModule);

    uint8_t* GV_DestroyActor_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 33 C0 48 8B 74 24 ?? 48 83 C4 ?? 5F C3 48 8B 74 24", "GV_DestroyActor call site");
    GV_DestroyActor = reinterpret_cast<GV_DestroyActor_t>(Memory::ResolveCall(GV_DestroyActor_scan));
    spdlog::info("MGS2_GameFuncs: GV_DestroyActor address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GV_DestroyActor - (uintptr_t)baseModule);

    HZX_OnlineHazardCheck = reinterpret_cast<HZX_OnlineHazardCheck_t>(Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 49 8B F8 48 8B F2", "HZX_OnlineHazardCheck"));
    spdlog::info("MGS2_GameFuncs: HZX_OnlineHazardCheck address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)HZX_OnlineHazardCheck - (uintptr_t)baseModule);

    if (StartInDebugMode)
    {
        uint8_t* GM_SetArea_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ?? 48 83 C3 1C 48 8B CB E8 ?? ?? ?? ?? 4C 8B C0 4C 2B C3 66 0F 1F 84 00", "GM_SetArea call site");
        GM_SetArea = reinterpret_cast<GM_SetArea_t>(Memory::ResolveCall(GM_SetArea_scan));
        spdlog::info("MGS2_GameFuncs: GM_SetArea address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_SetArea - (uintptr_t)baseModule);


        uint8_t* GCL_ChangeSenerioCode_scan = Memory::PatternScan(baseModule, "8B C8 E8 ?? ?? ?? ?? 33 C0 48 8B 4C 24", "GCL_ChangeSenerioCode call site");
        GCL_ChangeSenerioCode = reinterpret_cast<GCL_ChangeSenerioCode_t>(Memory::ResolveCall(GCL_ChangeSenerioCode_scan + 2));
        spdlog::info("MGS2_GameFuncs: GCL_ChangeSenerioCode address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GCL_ChangeSenerioCode - (uintptr_t)baseModule);

        GM_LoadRequest = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? A8 10 74 ?? E8", "MGS2: GM_PlayerStatus") + 2));
        spdlog::info("MGS2_GameFuncs: GM_LoadRequest address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_LoadRequest - (uintptr_t)baseModule);

        EnterDeveloperMenu = []()
        {
            using namespace Shared_Gamefuncs;
            using namespace MGS2_LinkVarBuf;
            using namespace MGS2Stages;

            if (!GM_SetArea || !GCL_ChangeSenerioCode || !GM_LoadRequest)
            {
                return;
            }

            GM_SetArea(GM_SaveArea, SELECT);
            GCL_ChangeSenerioCode(STRCODE_SCENERIO_GCX);
            GM_Result = 9999;
            *GM_LoadRequest = 0x0002 | 0x1;
        };

        MAKE_HOOK_MID(baseModule, "E9 ?? ?? ?? ?? C7 43 ?? 01 00 00 00 E9", "GM_StartDaemon() -> Act() | Set developer menu on startup", {
            static bool startup = true;
            if (!startup)
            {
                return;
            }
            startup = false;
            EnterDeveloperMenu();
                      });

        RegisterDevMenuHotkey();
    }


#if !defined(RELEASE_BUILD)

    /*
    return_hooks[MGS2Stages::D082P01][0x000381AC];

    return_hooks[MGS2Stages::D082P01][0x000381AC] = [](int name, int map) -> void* {
        spdlog::info("returning nullptr for NewChara, name {:X} map {:X}", name, map);
        return nullptr;
    };


    observe_hooks[MGS2Stages::N_TITLE][0x0FC14A5].callback = []() {
        spdlog::info("NewTitleScrMan called");
        };
    */




#endif


}


void MG1_Gamefuncs::HookGameFuncs()
{
    using namespace Shared_Gamefuncs;
    using namespace MG1_Gamefuncs;

#if !defined(RELEASE_BUILD)
                //return_hooks[MG1Stages::MG1][0xDD5EB6];      //  sub_1400D2C80    // mg_draw / init_game
    //return_hooks[MG1Stages::MG2][0x0FA91C];      //  sub_1400D20F0
    //return_hooks[MG1Stages::MG2][0x588DA3];      //  sub_1400D2C80
    //return_hooks[MG1Stages::MG2][0x5B316E];      //  _NewSaveVariable
    //return_hooks[MG1Stages::MG2][0x6B237D];      //  _NewGclAssert
    //return_hooks[MG1Stages::MG2][0x7BC389];      //  sub_140071BE0
    //return_hooks[MG1Stages::MG2][0x7EEDB2];      //  sub_1400CAF30
    //return_hooks[MG1Stages::MG2][0x9474FF];      //  sub_1400D1960
    //return_hooks[MG1Stages::MG2][0x99F754];      //  sub_1400CB370
    //return_hooks[MG1Stages::MG2][0x9BC19A];      //  _VRCLR_SetStars_1
    //return_hooks[MG1Stages::MG2][0x0A833FE];     //  sub_1400CE210
    //return_hooks[MG1Stages::MG2][0x0AAF706];     //  GM_COM_InventoryChangeMode
    //return_hooks[MG1Stages::MG2][0x0BCF6FF];     //  sub_1400D1890
            //return_hooks[MG1Stages::MG2][0x0D8361E];     //  j_GM_RealTimeClockGet        crashes game
    //return_hooks[MG1Stages::MG2][0x0DAF423];     //  sub_140071980
    //return_hooks[MG1Stages::MG2][0x0DC83C5];     //  _GM_LoadPack
    //return_hooks[MG1Stages::MG2][0x0DD5EB7];     //  sub_1400D68E0
    //return_hooks[MG1Stages::MG2][0x0E76D74];     //  NewGclVariableMove
    //return_hooks[MG1Stages::MG2][0x0E78C6D];     //  sub_1400CE540

#endif
    
}


void MG1_Gamefuncs::HookDllGameFuncs()
{
    //ShowMessage+3E
    uint8_t* pChunkDataCall = Memory::PatternScan(mg1Module, "E8 ?? ?? ?? ?? 4C 8D 4C 24 ?? 44 8B C3 33 D2 48 8B C8 48 8B F0 E8 ?? ?? ?? ?? 8B 5C 24", "MG1: ShowMessage()+0x3E : GetChunkEntryData()");
    if (pChunkDataCall)
    {
        uint8_t* pChunkData = Memory::ResolveCall(pChunkDataCall);
        if (*pChunkData == 0xE9)
        {
            pChunkData = Memory::ResolveCall(pChunkData);
        }
        pfnGetChunkEntryData = reinterpret_cast<GetChunkEntryData_t>(pChunkData);
    }
    pfnGetChunkEntrySize = reinterpret_cast<GetChunkEntrySize_t>(Memory::PatternScan(mg1Module, "40 53 48 83 EC ?? 48 63 DA E8 ?? ?? ?? ?? 48 63 4C 98 ?? 48 8D 05 ?? ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 33 D2", "MG1: GetChunkEntrySize()"));

    if (MG_DeveloperMenu::bEnabled)
    {
        pfnSceneTick = reinterpret_cast<SceneTick_t>(Memory::PatternScan(mg1Module, "41 55 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 ?? ?? ?? ?? 45 33 ED", "MG1: sub_18001B2A0() : scene tick"));
        pfnGotoStage = reinterpret_cast<GotoStage_t>(Memory::PatternScan(mg1Module, "40 53 48 83 EC ?? 8B D9 83 F9", "MG1: GotoStage()"));
        pfnSwitchScene = reinterpret_cast<SwitchScene_t>(Memory::PatternScan(mg1Module, "40 53 48 83 EC ?? 83 3D ?? ?? ?? ?? 00", "MG1: SwitchScene()"));
    }

    pfnShowMessage = reinterpret_cast<ShowMessage_t>(Memory::PatternScan(mg1Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC ?? 45 33 ED", "MG1: ShowMessage()"));
    pfnBlitImage = reinterpret_cast<BlitImage_t>(Memory::PatternScan(mg1Module, "40 53 56 57 41 54", "MG1: BlitImage()"));
    pfnSetFont = reinterpret_cast<SetFont_t>(Memory::PatternScan(mg1Module, "48 89 5C 24 ?? 4C 89 44 24", "MG1: SetFont()"));
    pfnDrawTextSlot = reinterpret_cast<DrawTextSlot_t>(Memory::PatternScan(mg1Module, "40 53 55 56 57 41 55", "MG1: DrawTextSlot()"));
    pfnComposeTextLayer = reinterpret_cast<ComposeTextLayer_t>(Memory::PatternScan(mg1Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 8B F1", "MG1: ComposeFrame()"));
    pfnMeasureTextFont = reinterpret_cast<MeasureTextFont_t>(Memory::PatternScan(mg1Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4D 63 F1", "MG1: MeasureTextFont()"));
    pfnUploadTexture = reinterpret_cast<UploadTexture_t>(Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 33 C0 4C 8D 44 24", "MG1: sub_140025F80() : upload texture"));

    TestFlag = reinterpret_cast<TestFlag_t>(Memory::PatternScan(mg1Module, "40 53 48 83 EC ?? 48 63 D9 E8 ?? ?? ?? ?? 4C 8B C3", "MG1: TestFlag()"));
}


void MG2_Gamefuncs::HookDllGameFuncs()
{
    uint8_t* pGetEngineState = Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? BA ?? ?? ?? ?? 8B 80", "MG2: BuildDoors()+0x27 : GetEngineState() call");
    if (pGetEngineState)
    {
        pfnGetEngineState = reinterpret_cast<GetEngineState_t>(Memory::ResolveCall(pGetEngineState));
    }

    if (MG_DeveloperMenu::bEnabled)
    {
        pfnSceneTick = reinterpret_cast<SceneTick_t>(Memory::PatternScan(mg2Module, "48 83 EC ?? 8B 05 ?? ?? ?? ?? 48 89 5C 24", "MG2: sub_180025A90() : scene tick"));
        uint8_t* pGetPlayer = Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? 48 8B D8 E8 ?? ?? ?? ?? 48 8B CB", "MG2: TickDoorTrigger()+0x2D : GetPlayer() call");
        if (pGetPlayer)
        {
            pfnGetPlayer = reinterpret_cast<GetPlayer_t>(Memory::ResolveCall(pGetPlayer));
        }
        pfnPrepareRoomEntry = reinterpret_cast<PrepareRoomEntry_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 45 33 F6", "MG2: sub_18001B6E0() : prepare room entry"));
        pfnRequestScene = reinterpret_cast<RequestScene_t>(Memory::PatternScan(mg2Module, "83 3D ?? ?? ?? ?? 00 74 ?? 83 F9", "MG2: RequestScene()"));
        pfnRunSceneTransition = reinterpret_cast<RunSceneTransition_t>(Memory::PatternScan(mg2Module, "48 83 EC ?? 83 3D ?? ?? ?? ?? 00 0F 84", "MG2: RunSceneTransition()"));
        pfnIsGameplayActive = reinterpret_cast<IsGameplayActive_t>(Memory::PatternScan(mg2Module, "48 83 EC ?? 83 3D ?? ?? ?? ?? 00 75", "MG2: IsGameplayActive()"));
    }

    pfnTestFlag = reinterpret_cast<TestFlag_t>(Memory::PatternScan(mg2Module, "8B C1 99 83 E2 ?? 03 C2 8B C8 83 E0 ?? 2B C2", "MG2: TestFlag()"));
    pfnUpdateCompanionFollow = reinterpret_cast<CompanionFollow_t>(Memory::PatternScan(mg2Module, "40 53 56 57 41 55 41 57 48 83 EC ?? 48 8B 0D", "MG2: UpdateCompanionFollow()"));
    pfnResetCompanionFollow = reinterpret_cast<CompanionFollow_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? E8 ?? ?? ?? ?? 48 8B F0", "MG2: ResetCompanionFollow()"));
    if (pfnUpdateCompanionFollow)
    {
        auto* pUpdate = reinterpret_cast<uint8_t*>(pfnUpdateCompanionFollow);
        MG2_LinkVarBuf::pCompanionList = reinterpret_cast<uintptr_t*>(Memory::GetRipRelativeAddress(pUpdate + 0xC, 3, 7));
        pfnGetHandleData = reinterpret_cast<GetHandleData_t>(Memory::ResolveCall(pUpdate + 0x13));
        if (!pfnGetPlayer)
        {
            pfnGetPlayer = reinterpret_cast<GetPlayer_t>(Memory::ResolveCall(pUpdate + 0x1B));
        }
        if (!pfnGetEngineState)
        {
            pfnGetEngineState = reinterpret_cast<GetEngineState_t>(Memory::ResolveCall(pUpdate + 0x23));
        }
    }

    pfnLoadImageBlob = reinterpret_cast<LoadImageBlob_t>(Memory::PatternScan(mg2Module, "40 53 48 83 EC ?? 4C 8B C2", "MG2: LoadImageBlob()"));
    pfnGetChunkEntryData = reinterpret_cast<GetChunkEntryData_t>(Memory::PatternScan(mg2Module, "40 53 48 83 EC ?? 48 63 DA E8 ?? ?? ?? ?? 48 63 8C 98 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? ?? ?? 48 83 C4", "MG2: GetChunkEntryData()"));
    pfnGetChunkEntrySize = reinterpret_cast<GetChunkEntrySize_t>(Memory::PatternScan(mg2Module, "40 53 48 83 EC ?? 48 63 DA E8 ?? ?? ?? ?? 48 63 8C 98 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 33 D2", "MG2: GetChunkEntrySize()"));
    pfnIsBossSurvival = reinterpret_cast<IsBossSurvival_t>(Memory::PatternScan(mg2Module, "48 83 EC ?? 48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? ?? 48 83 C4", "MG2: IsBossSurvival()"));
    MAKE_HOOK_MID(mg2Module, "C7 05 ?? ?? ?? ?? 01 00 00 00 48 8B 5C 24 ?? 48 8B 6C 24 ?? 48 8B 74 24 ?? 48 83 C4 20", "MG2: Level Transition : GotoRoom()+0xB6", {
        GameVars::OnLevelTransition();
    });

    pfnSetFont = reinterpret_cast<SetFont_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 4C 89 44 24", "MG2: RegisterFont()"));
    pfnDrawTextItem = reinterpret_cast<DrawTextItem_t>(Memory::PatternScan(mg2Module, "40 53 55 56 57 41 55 48 81 EC", "MG2: DrawTextItem()"));
    pfnComposeTextLayer = reinterpret_cast<ComposeTextLayer_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 8B F1", "MG2: sub_180024460() : compose text layer"));
    pfnMeasureTextFont = reinterpret_cast<MeasureTextFont_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 56 41 57 48 83 EC ?? 4D 63 F1", "MG2: sub_1800242F0() : measure text"));
    pfnUploadTexture = reinterpret_cast<UploadTexture_t>(Memory::PatternScan(baseModule, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 33 C0 4C 8D 44 24", "MG2: sub_140025F80() : upload texture"));

    pfnAVS_GetLocalizedString = reinterpret_cast<AVS_GetLocalizedString_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC ?? 8B FA", "MG2: AVS_GetLocalizedString()"));
    pfnDrawMessageText = reinterpret_cast<DrawMessageText_t>(Memory::PatternScan(mg2Module, "4C 8B DC 53 57 41 55", "MG2: DrawMessageText()"));
    pfnRandom = reinterpret_cast<Random_t>(Memory::PatternScan(mg2Module, "40 53 48 83 EC ?? 8B D9 FF 15", "MG2: Random()"));
    pfnScriptRun = reinterpret_cast<ScriptRun_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B D9", "MG2: ScriptRun()"));
    uint8_t* pGetObjectSprite = Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? 83 BD ?? ?? ?? ?? 00 4C 8B F8", "MG2: TickDoorTrigger()+0x40 : GetObjectSprite() call");
    if (pGetObjectSprite)
    {
        pfnGetObjectSprite = reinterpret_cast<GetObjectSprite_t>(Memory::ResolveCall(pGetObjectSprite));
    }
    pfnDrawSprite = reinterpret_cast<DrawSprite_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 55", "MG2: DrawSprite()"));
    pfnInitSprite = reinterpret_cast<InitSprite_t>(Memory::PatternScan(mg2Module, "40 53 48 83 EC ?? 8B 44 24", "MG2: InitSprite()"));
    pfnBuildDoors = reinterpret_cast<BuildDoors_t>(Memory::PatternScan(mg2Module, "40 55 53 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 05", "MG2: BuildDoors()"));
    pfnGetLayerImage = reinterpret_cast<GetLayerImage_t>(Memory::PatternScan(mg2Module, "48 63 C1 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? ?? ?? ?? ?? C3", "MG2: GetLayerImage()"));
    pfnDrawSprites = reinterpret_cast<DrawSprites_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? ?? ?? E8", "MG2: DrawSprites()"));
    pfnLayerFillRect = reinterpret_cast<LayerFillRect_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 41 54", "MG2: LayerFillRect()"));
    pfnRequestSound = reinterpret_cast<RequestSound_t>(Memory::PatternScan(mg2Module, "48 89 5C 24 ?? 57 48 83 EC ?? 48 63 F9", "MG2: RequestSound()"));
    uint8_t* pSetDrawColor = Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? 8B 74 24", "MG2: DrawRadar()+0x34B : SetDrawColor() call");
    if (pSetDrawColor)
    {
        pfnSetDrawColor = reinterpret_cast<SetDrawColor_t>(Memory::ResolveCall(pSetDrawColor));
    }

    pfnScriptCommand = reinterpret_cast<ScriptCommand_t>(Memory::PatternScan(mg2Module, "40 55 53 56", "MG2: ScriptCommand()"));

    HasItem = reinterpret_cast<HasItem_t>(Memory::PatternScan(mg2Module, "40 53 48 83 EC ?? 8B D9 83 F9 ?? 48 8B 0D ?? ?? ?? ?? 7D ?? E8 ?? ?? ?? ?? 8B CB", "MG2: HasItem()"));
}


void MGS3_Gamefuncs::HookGameFuncs()
{
    using namespace Shared_Gamefuncs;
    using namespace MGS3_Gamefuncs;
    using namespace MGS3_LinkVarBuf;
    using namespace MGS3Stages;


    if (StartInDebugMode)
    {
        uint8_t* GM_SetArea_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8", "GM_SetArea call site");
        GM_SetArea = reinterpret_cast<GM_SetArea_t>(Memory::ResolveCall(GM_SetArea_scan));
        spdlog::info("MGS3_GameFuncs: GM_SetArea address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_SetArea - (uintptr_t)baseModule);

        uint8_t* GCL_ChangeSenerioCode_scan = Memory::PatternScan(baseModule, "83 0D ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9 ?? ?? ?? ?? 48 83 C4 ?? E9", "GCL_ChangeSenerioCode call site");
        GCL_ChangeSenerioCode = reinterpret_cast<GCL_ChangeSenerioCode_t>(Memory::ResolveCall(GCL_ChangeSenerioCode_scan + 0xC));
        spdlog::info("MGS3_GameFuncs: GCL_ChangeSenerioCode address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GCL_ChangeSenerioCode - (uintptr_t)baseModule);


        // or dword ptr [rip+disp32], imm8 - the imm8 makes this 7 bytes, so the displacement is
        // not the last field and GetRelativeOffset would land a byte low.
        GM_LoadRequest = reinterpret_cast<int*>(Memory::GetRipRelativeAddress(Memory::PatternScan(baseModule, "83 0D ?? ?? ?? ?? ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9 ?? ?? ?? ?? 48 83 C4 ?? E9", "MGS3: GM_PlayerStatus"), 2, 7));
        spdlog::info("MGS3_GameFuncs: GM_LoadRequest address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_LoadRequest - (uintptr_t)baseModule);

        EnterDeveloperMenu = []()
        {
            using namespace Shared_Gamefuncs;
            using namespace MGS3_LinkVarBuf;
            using namespace MGS3Stages;

            if (!GM_SetArea || !GCL_ChangeSenerioCode || !GM_LoadRequest)
            {
                return;
            }

            GM_SetArea(GM_SaveArea, SELECT);
            GCL_ChangeSenerioCode(STRCODE_SCENERIO_GCX);
            GM_Result = 9999;
            *GM_LoadRequest = 0x3;
        };

        if (uint8_t* Act_addr = Memory::PatternScan(baseModule, "48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 ?? 48 89 5C 24", "GM_StartDaemon() -> Act() | Set developer menu on startup"))
        {
            static SafetyHookMid Act_Startup_hook {};

            Act_Startup_hook = safetyhook::create_mid(Act_addr + 0x406, [](SafetyHookContext& ctx)
                {
                    static bool startup = true;
                    if (!startup)
                    {
                        return;
                    }

                    startup = false;
                    EnterDeveloperMenu();
                });

            LOG_HOOK(Act_Startup_hook, "GM_StartDaemon() -> Act()+0x406 | Set developer menu on startup");
        }

        RegisterDevMenuHotkey();
    }


}



void Shared_Gamefuncs::HookFuncs()
{
    pfnGetLanguage = reinterpret_cast<GetLanguage_t>(GetProcAddress(engineModule, "BP_COsContext_GetLanguage_MGS"));

    uint8_t* GM_GetArea_scan = Memory::PatternScan(baseModule, (eGameType & MGS2) ? "E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 4C 8D 0D ?? ?? ?? ?? 4C 8B D0" : "E8 ?? ?? ?? ?? 33 C9 4C 8D 05 ?? ?? ?? ?? 0F 1F 40", "GM_GetArea call site");
    Shared_Gamefuncs::GM_GetArea = reinterpret_cast<Shared_Gamefuncs::GM_GetArea_t*>(Memory::ResolveCall(GM_GetArea_scan));
    spdlog::info("Shared_GameFuncs: GM_GetArea address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)Shared_Gamefuncs::GM_GetArea - (uintptr_t)baseModule);

    if (eGameType & (MGS2|MGS3))
    {
        //_NewDelay+3C
        uint8_t* BP_AdjustTick_scan = Memory::PatternScan(baseModule, (eGameType & MGS2) ? "E8 ?? ?? ?? ?? 8B F8 B1 ?? E8 ?? ?? ?? ?? 48 85 C0 74 ?? E8 ?? ?? ?? ?? 48 8B E8" : "E8 ?? ?? ?? ?? 48 63 97 ?? ?? ?? ?? 48 8D 4C 24", "BP_AdjustTick call site (delay.c -> NewDelay())");
        Shared_Gamefuncs::BP_AdjustTick = reinterpret_cast<Shared_Gamefuncs::BP_AdjustTick_t>(Memory::ResolveCall(BP_AdjustTick_scan));
        spdlog::info("Shared_GameFuncs: BP_AdjustTick address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)Shared_Gamefuncs::BP_AdjustTick - (uintptr_t)baseModule);

    }

    //stage_flatfs.cpp -> stage_begin_load_kp_file() | @l475
    uint8_t* BP_GetAssetLoadFullPath_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 48 8D 95 ?? ?? ?? ?? 48 8D 8D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8", "BP_GetAssetLoadFullPath call site");
    BP_GetAssetLoadFullPath = reinterpret_cast<BP_GetAssetLoadFullPath_t>(Memory::ResolveCall(BP_GetAssetLoadFullPath_scan));
    spdlog::info("Shared_GameFuncs: BP_GetAssetLoadFullPath address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)BP_GetAssetLoadFullPath - (uintptr_t)baseModule);



#if !defined(RELEASE_BUILD)

    spdlog::info("Shared_GameFuncs: Hooking CHARA stage function table.");

    uint8_t* GM_GetCharaID_scan = Memory::PatternScan(baseModule, eGameType & MGS2 ? "E8 ?? ?? ?? ?? 48 8B F0 48 85 C0 75 ?? 8D 46 ?? 48 83 C4" : "E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 75 ?? 48 8D 43 ?? 48 83 C4 ?? 5B", "GM_GetCharaID call site");
    GM_GetCharaID_hook = safetyhook::create_inline(Memory::ResolveCall(GM_GetCharaID_scan), hooked_GM_GetCharaID);
    spdlog::info("Shared_GameFuncs: GM_GetCharaID address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_GetCharaID_hook.target() - (uintptr_t)baseModule);

#endif




    switch (eGameType)
    {
    case MGS2:
        MGS2_GameFuncs::HookGameFuncs();
        break;
    case MGS3:
        MGS3_Gamefuncs::HookGameFuncs();
        break;
    case MG:
        MG1_Gamefuncs::HookGameFuncs();
        break;
    default:
        return;

    }
}

#if defined (RELEASE_CLEARED)
#define RELEASE_BUILD
#endif
