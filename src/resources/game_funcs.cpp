#include "stdafx.h"

#include "common.hpp"
#include "logging.hpp"

#include "game_funcs.hpp"

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


}


void MGS2_GameFuncs::HookFuncs()
{
    spdlog::info("MGS2_GameFuncs: Hooking game functions.");
    GM_SeSet = reinterpret_cast<GM_SeSet_t>(Memory::PatternScan(baseModule, "83 F9 ?? 74 ?? ?? 83 E1", "GM_SeSet"));
    spdlog::info("MGS2_GameFuncs: GM_SeSet address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_SeSet - (uintptr_t)baseModule);
    
    uint8_t* GM_ItemNum_scan = Memory::PatternScan(baseModule, "E8 ?? ?? ?? ?? 85 C0 7E ?? B9 ?? ?? ?? ?? E8 ?? ?? ?? ?? EB", "GM_ItemNum call site");
    GM_ItemNum = reinterpret_cast<GM_ItemNum_t>(Memory::ResolveCall(GM_ItemNum_scan));
    spdlog::info("MGS2_GameFuncs: GM_ItemNum address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)GM_ItemNum - (uintptr_t)baseModule);
}

