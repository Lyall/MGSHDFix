#include "stdafx.h"

#include "mgs2_restore_dogtag_viewer.hpp"
#include "common.hpp"
#include "gamevars.hpp"
#include "game_funcs.hpp"
#include "game_stages.hpp"
#include "logging.hpp"
#include "mgs2_linkvarbuf.hpp"

using namespace MGS2_GameFuncs;
using namespace MGS2_LinkVarBuf;

#define RETURN_IF_NOT_MSELECT_OR_W11A() \
    if (!g_GameVars.IsStage(MGS2Stages::MSELECT) && !g_GameVars.IsStage(MGS2Stages::W11A)) \
    { \
        return; \
    }

namespace
{

    
    int month {}, day {}, bloodtype {};


    struct l2dVertex { float x, y; uint8_t r, g, b, a; int32_t vert : 1, rgba : 1; };

    struct l2dSprite {
        int code;
        uint16_t id;
        void* obj;
        int vertex_num;
        l2dVertex* vertex;
        int pri_base, pri_adjust, tex_code, tex_handle;
        float u, v, w, h, width, height, magni;
        int spin_dir, now_status;
        int16_t status_num;
        void* stat;
        void* conv_func;
    };

    inline safetyhook::InlineHook hook_L2D_SetupLayout2;

    constexpr float NAME_DATA_X_OFFSET = -55.0f;
    constexpr float NAME_LABEL_UV_TRIM = 0.455f;


    int iBloodMinimumDobYear = 1990;
    int iDefaultDobYear = 1970;


    int hk_L2D_SetupLayout2(void* entry_ptr, int chanl, int base_pri, int add_flag, int pause_level, float safeZoneOffsetY)
    {
        int handle = hook_L2D_SetupLayout2.call<int>(entry_ptr, chanl, base_pri, add_flag, pause_level, safeZoneOffsetY);
        if (g_GameVars.IsStage(MGS2Stages::W11A) || g_GameVars.IsStage(MGS2Stages::MSELECT))
        {
            if (handle >= 0)
            {
                auto fix_sprite = [&](int strcode) {
                    auto* spr = static_cast<l2dSprite*>(L2D_GetParts(handle, strcode));
                    if (!spr) return;
                    int      vnum = spr->vertex_num;
                    int16_t  snum = spr->status_num;
                    auto* stat0 = reinterpret_cast<uint8_t*>(spr->stat);
                    for (int si = 0; si < snum; si++)
                    {
                        auto* vptr = *reinterpret_cast<l2dVertex**>(stat0 + si * 0x58 + 0x20);
                        if (!vptr) continue;
                        for (int vi = 0; vi < vnum; vi++)
                            if (vptr[vi].x >= 6000.0f) vptr[vi].x -= 6000.0f;
                    }
                };

                fix_sprite(241165);   // STR_PARTS_NAME_BLD  6058->58
                fix_sprite(7115211);  // STR_PARTS_BLD        6250->250
                fix_sprite(258369);   // STR_PARTS_NAME_SEX   6058->58
                fix_sprite(7132415);  // STR_PARTS_SEX        6134->134

                auto fix_y = [&](int strcode, float old_y, float new_y) {
                    auto* spr = static_cast<l2dSprite*>(L2D_GetParts(handle, strcode));
                    if (!spr) return;
                    int     vnum = spr->vertex_num;
                    int16_t snum = spr->status_num;
                    auto* stat0 = reinterpret_cast<uint8_t*>(spr->stat);
                    for (int si = 0; si < snum; si++)
                    {
                        auto* vptr = *reinterpret_cast<l2dVertex**>(stat0 + si * 0x58 + 0x20);
                        if (!vptr) continue;
                        for (int vi = 0; vi < vnum; vi++)
                            if (vptr[vi].y == old_y) vptr[vi].y = new_y;
                    }
                };

                fix_y(11990742, 94.0f, 116.0f);  // STR_PARTS_NAME_BIRTH  94->116 (down to REG's current pos)
                fix_y(257328, 116.0f, 160.0f);   // STR_PARTS_NAME_REG   116->160 (22px below BLD's y=138)

                auto fix_x = [&](int strcode, float delta_x) {
                    auto* spr = static_cast<l2dSprite*>(L2D_GetParts(handle, strcode));
                    if (!spr) return;
                    int     vnum = spr->vertex_num;
                    int16_t snum = spr->status_num;
                    auto* stat0 = reinterpret_cast<uint8_t*>(spr->stat);
                    for (int si = 0; si < snum; si++)
                    {
                        auto* vptr = *reinterpret_cast<l2dVertex**>(stat0 + si * 0x58 + 0x20);
                        if (!vptr) continue;
                        for (int vi = 0; vi < vnum; vi++)
                            vptr[vi].x += delta_x;
                    }
                };

                fix_x(13646838, NAME_DATA_X_OFFSET);

                // cut "CODENAME" in half
                if (auto* spr = static_cast<l2dSprite*>(L2D_GetParts(handle, 8133413)))
                {
                    int16_t snum = spr->status_num;
                    auto* stat0 = reinterpret_cast<uint8_t*>(spr->stat);
                    for (int si = 0; si < snum; si++)
                    {
                        auto* s = stat0 + si * 0x58;
                        auto& pos_u = *reinterpret_cast<float*>(s + 0x0C); // l2dStatus.pos_u
                        auto& size_u = *reinterpret_cast<float*>(s + 0x14); // l2dStatus.size_u
                        auto& size_w = *reinterpret_cast<float*>(s + 0x28); // l2dStatus.size_w (display width)

                        if (pos_u >= 0.0f && size_u > 0.0f)
                        {
                            pos_u += size_u * NAME_LABEL_UV_TRIM;
                            size_u *= (1.0f - NAME_LABEL_UV_TRIM);
                        }
                        if (size_w > 0.0f)
                            size_w *= (1.0f - NAME_LABEL_UV_TRIM);
                    }
                }
            }
        }
        return handle;
    }
}




void MGS2_RestoreDogtagViewer::Restore()
{
    if (!(eGameType & MGS2))
    {
        return;
    }


#pragma region dogtag_viewer

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? B1", "dogtag capture month (2002)", {
            month = (int)ctx.rax;
            //spdlog::info("captured month: {}", month);
                  });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? B1", "dogtag capture day (2002)", {
            day = (int)ctx.rax;
                  });

    MAKE_HOOK_MID(baseModule, "B1 ?? E9 ?? ?? ?? ?? E8", "dogtag capture blood (2002)", {
            bloodtype = (int)ctx.rax;
                  });


    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? E8", "dogtag capture month (2001)", {
            month = (int)ctx.rax;
            //spdlog::info("captured month: {}", month);
                  });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 4C 8B E8 E8 ?? ?? ?? ?? 44 8B F0 E8 ?? ?? ?? ?? 44 8B F8 E8 ?? ?? ?? ?? E8", "dogtag capture day (2001)", {
            day = (int)ctx.rax;
                  });

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? ?? 81 E9", "dogtag capture blood (2001)", {
            bloodtype = (int)ctx.rax;
                  });

    MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 44 89 44 24", "dogtag restoration month/day/blood", {
        //spdlog::info("month: {}, day: {}, bloodtype: {}", month, day, bloodtype);
        *reinterpret_cast<int*>(ctx.rsp + 0x28) = month;
        *reinterpret_cast<int*>(ctx.rsp + 0x30) = day;
        *reinterpret_cast<int*>(ctx.rsp + 0x38) = bloodtype;
                     });
#pragma endregion dogtag_viewer


    if (!bRestoreNodeScreen)
    {
        return;
    }


#pragma region node_screen
    const std::time_t now = std::time(nullptr);
    std::tm localTime {};

    if (localtime_s(&localTime, &now) == 0)
    {
        const int currentYear = localTime.tm_year + 1900;

        iBloodMinimumDobYear = currentYear - 11;
        iDefaultDobYear = currentYear - 31;
        spdlog::info("MGS2: Dogtag Viewer: Current year is {}. Blood will be disabled in the config via node if birthday is set to {} or earlier. Default DOB year is {}", currentYear, iBloodMinimumDobYear, iDefaultDobYear);
    }


    spdlog::info("Restoring node screen DOB/blood/sex entry.");


    /////////////////////////////////////                    staff names            //////////////////////////////////////
    ///
    MAKE_HOOK_MID(baseModule, "C6 43 ?? ?? EB ?? E8", "Node Staff Names", {
            auto* node = reinterpret_cast<char*>(ctx.rbx);
            auto* pWork = reinterpret_cast<char*>(ctx.r15);

            *reinterpret_cast<int*>(pWork + 0x114) = *reinterpret_cast<int*>(node + 0x38); // sex
            *reinterpret_cast<int*>(pWork + 0x148) = *reinterpret_cast<int*>(node + 0x3C); // year
            *reinterpret_cast<int*>(pWork + 0x14C) = *reinterpret_cast<int*>(node + 0x40); // month
            *reinterpret_cast<int*>(pWork + 0x150) = *reinterpret_cast<int*>(node + 0x44); // day
            *reinterpret_cast<int*>(pWork + 0x1C0) = *reinterpret_cast<int*>(node + 0x48); // blood
            *reinterpret_cast<int*>(pWork + 0x1E8) = *reinterpret_cast<int*>(node + 0x4C); // nation
                  });


    //////////////////////////////////////////////                    node screen           //////////////////////////////////////

    static int set_year = 1970;

    //update the node screen to properly show the birth year
    MAKE_HOOK_MID(baseModule, "44 8B 8B ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 44 8B 83 ?? ?? ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 44 8B 8B ?? ?? ?? ?? 48 8D 93 ?? ?? ?? ?? 44 8B 83 ?? ?? ?? ?? 48 8D 4C 24", "skoba\\etc\\name_layout.c -> BirthDayUpdate() -> @ l2766",
                  {
            char* buf = reinterpret_cast<char*>(ctx.rsp + 0x48);
            set_year = *reinterpret_cast<int*>(ctx.rbx + 0x148);
            sprintf(buf, "%04d", set_year);
                  });

    //fix year highlight width
    MAKE_HOOK_MID(baseModule, "66 41 0F 6E C0 0F 5B C0 F3 0F 11 80", "skoba\\etc\\name_layout.c -> BirthDayUpdate() -> l2801", {
        if (*reinterpret_cast<int*>(ctx.rbx + 0x160) == 0)
        {
            auto w = [](int d) -> uint32_t { return d == 1 ? 1u : 8u; };
            uint32_t prefix_w = (set_year >= 2000) ? 36u : 29u;
            ctx.r8 = prefix_w + w((set_year / 10) % 10) + w(set_year % 10);
        }
                  });


    //disable forced jump to month when selecting dob field
    if (uint8_t* PositionAct_scan = Memory::PatternScan(baseModule, "C7 81 ?? ?? ?? ?? ?? ?? ?? ?? 25 ?? ?? ?? ?? EB", "MGS2: skoba\\etc\\name_layout.c -> PositionAct() -> @ l2296"))
    {
        Memory::PatchBytes((uintptr_t)PositionAct_scan, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);
    }

    //allow pressing left to go back to year
    MAKE_HOOK_MID(baseModule, "0F BA E1 ?? 0F 82 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 48 8B CB", "skoba\\etc\\name_layout.c -> PadControlBirthEntry() -> @ l1861", {
            int  phase = *reinterpret_cast<int*>(ctx.rbx + 0x160);
            bool padL = (ctx.rcx >> 15) & 1;

            if (phase == 1 && padL)
            {
                *reinterpret_cast<int*>(ctx.rbx + 0x160) = 0;   // month -> year
                auto* hilight = *reinterpret_cast<char**>(ctx.rbx + 0x100);
                *reinterpret_cast<float*>(hilight + 0xA4) = 0.0f; // hilight.dw = 0
            }
                  });


    hook_L2D_SetupLayout2 = safetyhook::create_inline(reinterpret_cast<void*>(Memory::PatternScan(baseModule, "48 83 EC ?? 48 8B 05 ?? ?? ?? ?? 4C 8B D1", "MGS 2: L2D_SetupLayout2")),reinterpret_cast<void*>(hk_L2D_SetupLayout2));


    MAKE_HOOK_MID(baseModule, "66 C7 80 ?? ?? ?? ?? ?? ?? C6 80 ?? ?? ?? ?? ?? 48 63 9F ?? ?? ?? ?? 83 FB ?? 7D ?? 4C 8D B7", "skoba\\etc\\name_layout.c -> SprInit() -> @ l574 | SprInit_3 + 0x1A2", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        *reinterpret_cast<float*>(ctx.rax + 0x9C) = 160.0f;
                  });

    MAKE_HOOK_MID(baseModule, "C7 86 9C 00 00 00 00 00 90 42 48 63 9F 28 04 00 00", "skoba\\etc\\name_layout.c -> SprInit() -> name data field pos.x @ l550", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
    *reinterpret_cast<float*>(ctx.rsi + 0x98) += NAME_DATA_X_OFFSET;
                  });

    MAKE_HOOK_MID(baseModule, "8B 4F ?? E8 ?? ?? ?? ?? 48 85 C0 75 ?? 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? B8 ?? ?? ?? ?? E9 ?? ?? ?? ?? C6 80 ?? ?? ?? ?? 00", "skoba\\etc\\name_layout.c -> SprInit() -> @ l599", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        auto* kcej = *reinterpret_cast<uint8_t**>(ctx.rdi + 0x2F8);
        if (kcej) *reinterpret_cast<float*>(kcej + 0x9C) = 160.0f;
                  });

    MAKE_HOOK_MID(baseModule, "C7 80 ?? ?? ?? ?? ?? ?? ?? ?? C7 80 ?? ?? ?? ?? ?? ?? ?? ?? 66 C7 80 ?? ?? ?? ?? ?? ?? C6 80 ?? ?? ?? ?? ?? 48 63 9F ?? ?? ?? ?? 83 FB ?? 7D ?? 48 8D B7", "skoba\\etc\\name_layout.c -> SprInit() -> @ l616", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        *reinterpret_cast<float*>(ctx.rax + 0x9C) = 116.0f;
                  });


#pragma region Menu_Navigation

    MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? 8D 4A ?? E8 ?? ?? ?? ?? 8B 83", "skoba\\etc\\name_layout.c -> SoftKey() -> @ l925", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        ctx.rax--;
                  });

    MAKE_HOOK_MID(baseModule, "89 8B ?? ?? ?? ?? 8B 83 ?? ?? ?? ?? BA", "skoba\\etc\\name_layout.c -> PadControlNormal() -> UP sex @ l1430", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        ctx.rcx++;
                  });

    MAKE_HOOK_MID(baseModule, "81 A3 ?? ?? ?? ?? ?? ?? ?? ?? BA ?? ?? ?? ?? 89 83", "skoba\\etc\\name_layout.c -> PadControlNormal() -> UP blood @ l1433", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        if (ctx.rax == 2 && (ctx.rcx & 0xFFFFFFFF) == 3)
            ctx.rax = 3;
                  });

    MAKE_HOOK_MID(baseModule, "89 8B ?? ?? ?? ?? 8B 83 ?? ?? ?? ?? 83 F9", "skoba\\etc\\name_layout.c ->PadControlNormal() -> DOWN sex @ l1448", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        ctx.rcx--;
                  });

    MAKE_HOOK_MID(baseModule, "81 A3 ?? ?? ?? ?? ?? ?? ?? ?? 89 83 ?? ?? ?? ?? 8D 4A", "skoba\\etc\\name_layout.c -> PadControlNormal() -> DOWN blood @ l1451", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        if (ctx.rax == 4 && (ctx.rcx & 0xFFFFFFFF) == 3)
            ctx.rax = 3;
                  });

    MAKE_HOOK_MID(baseModule, "89 B3 ?? ?? ?? ?? 81 A3 ?? ?? ?? ?? ?? ?? ?? ?? 41 B8", "skoba\\etc\\name_layout.c -> PadControlNormal() -> cancel @ l1470", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        auto old_pos = *reinterpret_cast<int*>(ctx.rbx + 0x110);
        if (old_pos != 5) ctx.rsi++;
                  });

    MAKE_HOOK_MID(baseModule, "48 8B CB 89 93 ?? ?? ?? ?? E8", "skoba\\etc\\name_layout.c -> DotUpdate() -> @ l2188", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        auto pos = static_cast<int>(ctx.rdx & 0xFFFFFFFF);
        auto& code1 = *reinterpret_cast<int*>(ctx.rbx + 0x360);
        if (pos == 2) code1 = 4786416;  // STR_CUR_BIRTH
        if (pos == 4) code1 = 9064884;  // STR_CUR_REG
                  });

    MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? EB ?? 44 8B 83 ?? ?? ?? ?? 41 8B D0", "skoba\\etc\\name_layout.c -> PadControlBirthEntry() -> cancel @ l1870", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        ctx.rax++;
                  });


    MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 8D 4A ?? 48 8B 5C 24", "skoba\\etc\\name_layout.c -> PadControlRegionEntry() -> cancel @ l2029", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        auto old = *reinterpret_cast<int*>(ctx.rbx + 0x110);
        if (old - (int)(ctx.rax & 0xFFFFFFFF) == 2)
        {
            ctx.rax++;
        }
                  });

    // PadControlBirthEntry+0x1A8
    MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? 8D 4A ?? E8 ?? ?? ?? ?? E9 ?? ?? ?? ?? 8B B3", "skoba\\etc\\name_layout.c -> PadControlBirthEntry() -> confirm @ l1850", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        ctx.rax--;
                  });

                  // PadControlBirthEntry+0x2EB
    MAKE_HOOK_MID(baseModule, "89 83 ?? ?? ?? ?? EB ?? 41 8D 40", "skoba\\etc\\name_layout.c -> PadControlBirthEntry() -> confirm-sta @ l1850", {
        RETURN_IF_NOT_MSELECT_OR_W11A();
        ctx.rax--;
                  });

#pragma endregion 


        //update GM_MyYearData age check for GM_Configuration blood off.
    if (uint8_t* StoreData_scan = Memory::PatternScan(baseModule, "81 B8 ?? ?? ?? ?? ?? ?? ?? ?? 7C", "MGS2: skoba\\etc\\name_layout.c -> StoreData() -> @ l2633"))
    {
        Memory::PatchBytes((uintptr_t)(StoreData_scan + 6), reinterpret_cast<const char*>(&iBloodMinimumDobYear), 4);
        spdlog::info("MGS2: StoreData() age check for blood is now set to {}", iBloodMinimumDobYear); 
        spdlog::info("(If Birth Year is set to {} or earlier, blood will be disabled in the config via node.)", iBloodMinimumDobYear);
        //Util::DumpBytes((uintptr_t)StoreData_scan, 10);
    }

    if (uint8_t* GetResourcesByNode_scan = Memory::PatternScan(baseModule, "C7 83 ?? ?? ?? ?? ?? ?? ?? ?? C7 83 ?? ?? ?? ?? ?? ?? ?? ?? C7 83 ?? ?? ?? ?? ?? ?? ?? ?? 89 B3 ?? ?? ?? ?? 40 88 B3", "MGS2: skoba\\etc\\name_layout.c -> GetResourcesByNode() -> @ l3445"))
    {
        Memory::PatchBytes((uintptr_t)(GetResourcesByNode_scan + 6), reinterpret_cast<const char*>(&iDefaultDobYear), 4);
        spdlog::info("MGS2: GetResourcesByNode() default DOB year is now set to {}", iDefaultDobYear);
    }


#pragma endregion node_screen

#pragma region endgame_dogtag

    MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 41 B8", "takabe\\object\\eddogtag.c -> GetResources() -> @ l455", {
        ctx.rdx = reinterpret_cast<uintptr_t>(&"Name");
                  });


    MAKE_HOOK_MID(baseModule, "41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 4C 8B 05", "takabe\\object\\eddogtag.c -> GetResources() -> @ l463",
                  {
                      *reinterpret_cast<int*>(ctx.rdi + 0x88) = 166;
                      *reinterpret_cast<int*>(ctx.rdi + 0x8C) = 97;
                      WriteString(reinterpret_cast<void*>(ctx.rdi), "Blood", 4);
                  });

    MAKE_HOOK_MID(baseModule, "48 8D 15 ?? ?? ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 45 8B C5", "takabe\\object\\eddogtag.c -> GetResources() -> @ l500",
                  {
                      ctx.r8 = reinterpret_cast<uintptr_t>(GM_MySexData ? "FEMALE" : "MALE");
                  });

    static const char* blood_list[] = { "?", "A", "B", "AB", "O" };

    MAKE_HOOK_MID(baseModule, "48 8B 05 ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8D 4C 24 ?? 44 8B 88", "takabe\\object\\eddogtag.c -> GetResources() -> @ l505", {
                      char* buf = reinterpret_cast<char*>(ctx.rsp + 0x30);
                      sprintf(buf, "%s", blood_list[GM_MyBloodData.get()]);
                      *reinterpret_cast<int*>(ctx.rdi + 0x88) = 238;
                      *reinterpret_cast<int*>(ctx.rdi + 0x8C) = 97;
                      WriteString(reinterpret_cast<void*>(ctx.rdi), buf, 1);
                  });

    MAKE_HOOK_MID(baseModule, "41 B8 ?? ?? ?? ?? C7 87 ?? ?? ?? ?? ?? ?? ?? ?? 48 8D 54 24 ?? C7 87", "takabe\\object\\eddogtag.c -> GetResources() -> @ l512",
                  {
                      char* buf = reinterpret_cast<char*>(ctx.rsp + 0x30);
                      sprintf(buf, "%02d/%02d/%04d", static_cast<int>(GM_MyMonthData.get()), static_cast<int>(GM_MyDayData.get()), static_cast<int>(GM_MyYearData.get()));
                  });

#pragma endregion endgame_dogtag

}
