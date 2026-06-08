#include "stdafx.h"
#include "custom_player_name.hpp"

#include "common.hpp"
#include "logging.hpp"


namespace
{

    //constexpr int MAX_RAY_LENGTH = 10;  // ray_prefix max length

    constexpr const char* sNameString_Snake = "SNAKE";
    constexpr const char* sNameString_Pliskin = "PLISKIN";
    constexpr const char* sNameString_MGS1_Snake = "SOLID SNAKE";
    constexpr const char* sNameString_Tuxedo_Snake = "TUXEDO SNAKE";
    constexpr const char* sNameString_Raiden = "RAIDEN";
    constexpr const char* sNameString_Jack = "JACK";
    constexpr const char* sNameString_Naked_Raiden = "NAKED RAIDEN";
    constexpr const char* sNameString_Ninja = "NINJA RAIDEN";


}
//todo - finish

void CustomPlayerName::Apply()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    if (!bUseCustomName && !bUseStoryName)
    {
        spdlog::info("MGS 2: Custom Player Name: Config disabled. Skipping");
        return;
    }

    if (bUseCustomName)
    {
        if (sCustomName.empty())
        {
            spdlog::warn("MGS 2: Custom Player Name: Custom name is empty. Skipping");
            return;
        }
        if (sCustomName == "LIFE")
        {
            spdlog::warn("MGS 2: Custom Player Name: Custom name is default (LIFE). Skipping");
            return;
        }
        spdlog::info("MGS 2: Custom Player Name: Applying custom name '{}'", sCustomName);
        
        MAKE_HOOK_MID(baseModule, "48 8B F2 48 8B E9 FF 15 ?? ?? ?? ?? 8D 43", "GM_InitGageSet", {
            std::string nameString = reinterpret_cast<const char*>(ctx.rdx);
            if (nameString != "LIFE")
            {
                return;
            }
            ctx.rdx = reinterpret_cast<uintptr_t>(sCustomName.c_str());
        })
        return;
    }

    spdlog::info("MGS 2: Custom Player Name: Applying story name overrides based on stage and character.");

    /*
        r_plt0 plant - normal gameplay
        r_plt1 plant - divesuit
        r_plt2 plant - naked
        r_plt3 plant - w20c - dragging fatman (probably sdx change?)
        r_plt4 plant - Poly-demo only: Gurlukovich soldier equipment/loadout (with cap)
        r_plt5 plant - Poly-demo only: Gurlukovich soldier equipment/loadout (with cap)
        r_plt6 plant - Low-poly     demo resident
        r_plt7 plant - Low-poly     demo resident
        r_plt8  plant - low poly    demo resident
        r_plt9  plant - low poly    demo resident
        r_plt10 plant - low poly    demo resident
        r_plt11 plant - low poly    demo resident
        r_plt12 plant - low poly    demo resident
        r_plt13 plant - low poly    demo resident
        r_plt_s snake tales - snake (plant)
        r_rai_b //Raiden / boss rush resident scenario
        r_sna_b //Snake / boss rush resident scenario
        r_tnk0 - tanker (normal gameplay)
        r_tnk_r - tanker digital camera resident
        r_vr_1 vr - snake (mgs1)
        r_vr_b vr - raiden (ninja)
        r_vr_p vr - pliskin
        r_vr_r vr - raiden (normal)
        r_vr_rp vr - raiden (photo)
        r_vr_sp vr - snake (photo)
        r_vr_s vr - snake (normal)
        r_vr_t vr - snake (tuxedo)
        r_vr_x vr - raiden (naked)
    */

    MAKE_HOOK_MID(baseModule, "48 8B F2 48 8B E9 FF 15 ?? ?? ?? ?? 8D 43", "GM_InitGageSet", {
            std::string nameString = reinterpret_cast<const char*>(ctx.rdx);

    /*

        {


            spdlog::info("character: {}, current stage: {}", current_character, currentStage);

            if (nameString == "LIFE")
            {
                switch (auto category = StageHelpers::categorizeStage(currentStage))
                {

                case StageHelpers::StageCategory::Tanker:
                    ctx.rdx = reinterpret_cast<uintptr_t>(nameString_Tanker.c_str());
                    return;

                case StageHelpers::StageCategory::Plant:
                    if (std::strcmp(current_character, "r_plt1") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_player_plant_divesuit.c_str());
                        return;
                    }
                    if (std::strcmp(currentStage, "w51a") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_player_plant_arsenal_outside.c_str());
                        return;
                    }

                    if (std::strcmp(currentStage, "w61a") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_player_plant_federal_hall.c_str());
                        return;
                    }
                    if (std::strncmp(currentStage, "w4", 2) == 0)
                    {
                        if (std::strcmp(current_character, "r_plt2") == 0)
                        {
                            ctx.rdx = reinterpret_cast<uintptr_t>(nameString_player_plant_naked.c_str());
                            return;
                        }
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_player_plant_arsenal.c_str());
                        return;
                    }
                    ctx.rdx = reinterpret_cast<uintptr_t>(nameString_Plant.c_str());
                    return;


                case StageHelpers::StageCategory::VR:
                    if ((std::strcmp(current_character, "r_vr_r") == 0) || (std::strcmp(current_character, "r_vr_rp") == 0))
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_raiden.c_str());
                        return;
                    }
                    if ((std::strcmp(current_character, "r_vr_s") == 0) || (std::strcmp(current_character, "r_vr_sp") == 0))
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_snake.c_str());
                        return;
                    }
                    if (std::strcmp(current_character, "r_vr_p") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_pliskin.c_str());
                        return;
                    }
                    if (std::strcmp(current_character, "r_vr_t") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_tuxedo_snake.c_str());
                        return;
                    }
                    if (std::strcmp(current_character, "r_vr_b") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_ninja_raiden.c_str());
                        return;
                    }
                    if (std::strcmp(current_character, "r_vr_x") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_naked_raiden.c_str());
                        return;
                    }
                    if (std::strcmp(current_character, "r_vr_1") == 0)
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_vr_mgs1_snake.c_str());
                        return;
                    }
                    return;

                case StageHelpers::StageCategory::SnakeTales:
                    if ((std::strcmp(current_character, "r_vr_s") == 0))
                    {
                        ctx.rdx = reinterpret_cast<uintptr_t>(nameString_snake_tales_snake.c_str());
                        return;
                    }

                    return;
                default:
                    spdlog::info("{} stage category is unknown.", currentStage);
                }
            }

            });
            */    
    
    })


/*
            else if (nameString == "OLGA")  nameString = nameString_Olga;
            else if (nameString == "FATMAN")  nameString = nameString_Fatman;
            else if (nameString == "FORTUNE")  nameString = nameString_Fortune;
            else if (nameString == "HARRIER")  nameString = nameString_Harrier;
            else if (nameString == "VAMP")  nameString = nameString_Vamp;
            else if (nameString == "VAMP O2")  nameString = nameString_VampO2;
            else if (nameString == "SOLIDUS") nameString = nameString_Solidus;
            else if (nameString == "EMMA") nameString = nameString_Emma;
            else if (nameString == "EMMA O2") nameString = nameString_EmmaO2;
            else if (nameString == "KASATKA") nameString = nameString_Kasatka;
            else if (nameString == "SNAKE") nameString = nameString_Snake;
            else if (nameString == "MERYL") nameString = nameString_Meryl;
            else if (nameString == "PREZ") nameString = nameString_Prez;
            else if (nameString == "Genola") nameString = nameString_Genola;
            else if (nameString == "Gurlugon") nameString = nameString_Gurlugon;
            else if (nameString == "Mech Genola") nameString = nameString_MechGenola;
            else if (nameString.starts_with("RAY"))
            {
                nameString = nameString_RAY + nameString.substr(3);
            }

            std::string nameString_player_o2 = "O2";
            std::string nameString_player_grip = "GRIP";

        });*/
}
