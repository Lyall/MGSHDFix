#include "stdafx.h"

#include "mgs2_coolant_mirror.hpp"

#include "common.hpp"

#include "logging.hpp"



void CoolantMirrorFix::ApplyFix()
{
    /*
    MAKE_HOOK_MID(baseModule, "44 8B 43 ?? B8", "MGS2: Coolant Mirror", {
    const auto prim = *reinterpret_cast<uintptr_t*>(ctx.rbx + 88);
    *reinterpret_cast<uint64_t*>(prim + 224) = 0x44ULL; //(*GM_Item == 13) //13 = wp_coolspray
    spdlog::info("hit");
            //? 0x42ULL   // SCE_GS_SET_ALPHA(2, 0, 0, 1, 0) - thermal alpha
            //: 0x44ULL;  // SCE_GS_SET_ALPHA(0, 1, 0, 1, 0) - normal alpha

                  })

        MAKE_HOOK_MID(baseModule, "F3 0F 58 C3 C7 87", "test 1", {
        Util::DumpContext(ctx);
    })


                      */





/*

        MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 57 48 83 EC ?? 45 33 C9 48 8B F9 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 0F 84 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 48 8B C8 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 81 4B ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 83 4B", "test 1", {
spdlog::info("hit 2");
                      })



        MAKE_HOOK_MID(baseModule, "E8 ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 48 8D 97 ?? ?? ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? F3 44 0F 10 9F", "test 1", {
spdlog::info("hit 6");
                      })


        MAKE_HOOK_MID(baseModule, "0F 84 ?? ?? ?? ?? ?? ?? ?? 4C 8D 87", "test 1", {
spdlog::info("hit 7");
                      })


        MAKE_HOOK_MID(baseModule, "F3 0F 58 44 24 ?? F3 44 0F 59 DA", "test 1", {
spdlog::info("hit 8");
                      })

        MAKE_HOOK_MID(baseModule, "F3 0F 10 8F ?? ?? ?? ?? 0F 2F CE F3 0F 10 47 ?? F3 0F 10 57", "test 1", {
spdlog::info("hit 9");
                      })


        MAKE_HOOK_MID(baseModule, "F3 0F 10 8F ?? ?? ?? ?? 0F 2F CE F3 0F 10 47 ?? F3 0F 59 D1", "test 1", {
spdlog::info("hit 10");
                      })



        MAKE_HOOK_MID(baseModule, "F3 0F 5C 44 24 ?? F3 41 0F 5C C8", "test 1", {
spdlog::info("hit 11");
                      })


        MAKE_HOOK_MID(baseModule, "75 ?? E8 ?? ?? ?? ?? 48 8B D0 48 8B CF", "test 1", {
spdlog::info("hit 12");
                      })

*/

}
