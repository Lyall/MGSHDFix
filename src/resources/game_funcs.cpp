#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"

#include "game_funcs.hpp"

#include "gamevars.hpp"
#include "mgs2_linkvarbuf.hpp"

namespace
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

    constexpr unsigned int STRCODE_SCENERIO_GCX = GameVars::GV_StrCode("scenerio");
    int* GM_LoadRequest = nullptr;
}


void MGS2_GameFuncs::HookFuncs()
{
    spdlog::info("MGS2_GameFuncs: Hooking game functions.");
    GM_SeSet = reinterpret_cast<GM_SeSet_t>(Memory::PatternScan(baseModule, "83 F9 ?? 74 ?? ?? 83 E1", "GM_SeSet"));
    spdlog::info("MGS2_GameFuncs: GM_SeSet address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_SeSet - (uintptr_t)baseModule);
    
    uint8_t* GM_ItemNum_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 85 C0 7E ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB", "GM_ItemNum call site");
    GM_ItemNum = reinterpret_cast<GM_ItemNum_t>(Memory::ResolveCall(GM_ItemNum_scan));
    spdlog::info("MGS2_GameFuncs: GM_ItemNum address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_ItemNum - (uintptr_t)baseModule);

    uint8_t* L2D_GetObject_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? BA ?? ?? ?? ?? 48 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 8B ?? ?? ?? ?? BA ?? ?? ?? ?? 48 89 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? 8B 8B", "L2D_GetObject call site");
    L2D_GetObject = reinterpret_cast<L2D_GetObject_t>(Memory::ResolveCall(L2D_GetObject_scan));
    spdlog::info("MGS2_GameFuncs: L2D_GetObject address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)L2D_GetObject - (uintptr_t)baseModule);

    uint8_t* L2D_GetParts_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 48 85 C0 74 ?? 8B 93 ?? ?? ?? ?? 0F 28 DE", "L2D_GetParts call site");
    L2D_GetParts = reinterpret_cast<L2D_GetParts_t>(Memory::ResolveCall(L2D_GetParts_scan));
    spdlog::info("MGS2_GameFuncs: L2D_GetParts address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)L2D_GetParts - (uintptr_t)baseModule);

    uint8_t* WriteString_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 41 B8", "WriteString call site");
    WriteString = reinterpret_cast<WriteString_t>(Memory::ResolveCall(WriteString_scan));
    spdlog::info("MGS2_GameFuncs: WriteString address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)WriteString - (uintptr_t)baseModule);






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

        MAKE_HOOK_MID(baseModule, "E9 ?? ?? ?? ?? C7 43 ?? 01 00 00 00 E9", "GM_StartDaemon() -> Act() | Set developer menu on startup", {
            static bool startup = true;
            if (!startup)
            {
                return;
            }
            startup = false;
            GM_SetArea(MGS2_LinkVarBuf::GM_SaveArea, "select");
            GCL_ChangeSenerioCode(STRCODE_SCENERIO_GCX);
            MGS2_LinkVarBuf::GM_Result = 9999;
            *GM_LoadRequest = 0x0002 | 0x1;
                      });
    }

    /*
    */
}

