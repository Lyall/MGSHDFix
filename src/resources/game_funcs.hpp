#pragma once

namespace MGS2_GameFuncs
{
    void HookFuncs();
    
    using GM_SeSet_t = int64_t(__fastcall*)(int a1, uint8_t a2, int16_t a3);
    inline GM_SeSet_t GM_SeSet = nullptr;

    using GM_ItemNum_t = int(__fastcall*)(int a1);
    inline GM_ItemNum_t GM_ItemNum = nullptr;

}


