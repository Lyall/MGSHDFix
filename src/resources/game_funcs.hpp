#pragma once

namespace Shared_Gamefuncs
{
    void HookFuncs();

};



namespace MGS2_GameFuncs
{
    void HookGameFuncs();
    
    using GM_SeSet_t = int64_t(__fastcall*)(int a1, uint8_t a2, int16_t a3);
    inline GM_SeSet_t GM_SeSet = nullptr;

    using GM_ItemNum_t = int(__fastcall*)(int a1);
    inline GM_ItemNum_t GM_ItemNum = nullptr;

    using GM_WeaponNum_t = int(__fastcall*)(int a1);
    inline GM_WeaponNum_t GM_WeaponNum = nullptr;

    using GM_SetArea_t = void(__fastcall*)(int id, const char* dirname);
    inline GM_SetArea_t GM_SetArea = nullptr;

    using GCL_ChangeSenerioCode_t = void(__fastcall*)(int code);
    inline GCL_ChangeSenerioCode_t GCL_ChangeSenerioCode = nullptr;

    using L2D_GetObject_t = void* (__fastcall*)(int handle, int strcode);
    inline L2D_GetObject_t L2D_GetObject = nullptr;


    using L2D_GetParts_t = void* (__fastcall*)(int, int);
    inline L2D_GetParts_t L2D_GetParts = nullptr;

    using WriteString_t = void(__fastcall*)(void*, const char*, int);
    inline WriteString_t WriteString = nullptr;


    inline bool StartInDebugMode = false;
}



namespace MG1_Gamefuncs
{
    
    void HookGameFuncs();
}



namespace MGS3_Gamefuncs
{

    void HookGameFuncs();
}


