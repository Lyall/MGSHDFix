#include "stdafx.h"
#include "mg_inventory_menu.hpp"

#include "common.hpp"
#include "mg_developer_menu.hpp"
#include "mg1_linkvarbuf.hpp"
#include "mg2_linkvarbuf.hpp"

namespace
{
    struct Entry
    {
        int nId;
        const char* pszName;
    };

    constexpr Entry astMG1Weapons[] =
    {
        { MG1_ITEM_INDEX_HANDGUN, "Handgun" },
        { MG1_ITEM_INDEX_SUBMACHINE_GUN, "Submachine Gun" },
        { MG1_ITEM_INDEX_GRENADE_LAUNCHER, "Grenade Launcher" },
        { MG1_ITEM_INDEX_ROCKET_LAUNCHER, "Rocket Launcher" },
        { MG1_ITEM_INDEX_MINE, "Mine" },
        { MG1_ITEM_INDEX_PLASTIC_EXPLOSIVE, "Plastic Explosive" },
        { MG1_ITEM_INDEX_RC_MISSILE, "RC Missile" },
        { MG1_ITEM_INDEX_SUPPRESSOR, "Suppressor" },
    };
    constexpr Entry astMG1Items[] =
    {
        { MG1_ITEM_INDEX_CIGARETTES, "Cigarettes" },
        { MG1_ITEM_INDEX_FLASHLIGHT, "Flashlight" },
        { MG1_ITEM_INDEX_OXYGEN_CYLINDER, "Oxygen Cylinder" },
        { MG1_ITEM_INDEX_RATIONS, "Rations" },
        { MG1_ITEM_INDEX_COMPASS, "Compass" },
        { MG1_ITEM_INDEX_BINOCULARS, "Binoculars" },
        { MG1_ITEM_INDEX_BOMB_BLAST_SUIT, "Bomb Blast Suit" },
        { MG1_ITEM_INDEX_CARDBOARD_BOX, "Cardboard Box" },
        { MG1_ITEM_INDEX_INFRARED_GOGGLES, "Infrared Goggles" },
        { MG1_ITEM_INDEX_TRANSMITTER, "Transmitter" },
        { MG1_ITEM_INDEX_ENEMY_UNIFORM, "Enemy Uniform" },
        { MG1_ITEM_INDEX_ANTENNA, "Antenna" },
        { MG1_ITEM_INDEX_MINE_DETECTOR, "Mine Detector" },
        { MG1_ITEM_INDEX_PARACHUTE, "Parachute" },
        { MG1_ITEM_INDEX_GAS_MASK, "Gas Mask" },
        { MG1_ITEM_INDEX_ANTIDOTE, "Antidote" },
        { MG1_ITEM_INDEX_CARD_1, "Card 1" },
        { MG1_ITEM_INDEX_CARD_2, "Card 2" },
        { MG1_ITEM_INDEX_CARD_3, "Card 3" },
        { MG1_ITEM_INDEX_CARD_4, "Card 4" },
        { MG1_ITEM_INDEX_CARD_5, "Card 5" },
        { MG1_ITEM_INDEX_CARD_6, "Card 6" },
        { MG1_ITEM_INDEX_CARD_7, "Card 7" },
        { MG1_ITEM_INDEX_CARD_8, "Card 8" },
        { MG1_ITEM_INDEX_BANDANA, "Bandana" },
        { MG1_ITEM_INDEX_BODY_ARMOR, "Body Armor" },
    };
    constexpr Entry astMG2Weapons[] =
    {
        { MG2_WEAPON_INDEX_HANDGUN, "Handgun" },
        { MG2_WEAPON_INDEX_SUBMACHINE_GUN, "Submachine Gun" },
        { MG2_WEAPON_INDEX_GRENADE, "Grenade" },
        { MG2_WEAPON_INDEX_STINGER, "Stinger Missile" },
        { MG2_WEAPON_INDEX_RC_MISSILE, "RC Missile" },
        { MG2_WEAPON_INDEX_PLASTIC_EXPLOSIVE, "Plastic Explosive" },
        { MG2_WEAPON_INDEX_MINE, "Mine" },
        { MG2_WEAPON_INDEX_CAMOUFLAGE_MAT, "Camouflage Mat" },
        { MG2_WEAPON_INDEX_GAS_GRENADE, "Gas Grenade" },
        { MG2_WEAPON_INDEX_MOUSE, "Mouse" },
        { MG2_WEAPON_INDEX_LIGHTER, "Lighter" },
        { MG2_WEAPON_INDEX_SUPPRESSOR, "Suppressor" },
    };
    constexpr const char* apszMG2Items[] =
    {
        "Cigarettes", "Bandana", "Binoculars", "Night-Vision Goggles", "Infrared Goggles", "Gas Mask",
        "Body Armor", "Oxygen Tank", "Mine Detector", "Hang Glider", "Cardboard Box", "Bucket",
        "Cold Medicine", "Cassette Tape", "Egg (owl)", "Egg (snake)", "Brooch",
        "Card 1", "Card 2", "Card 3", "Card 4", "Card 5", "Card 6", "Card 7", "Card 8", "Card 9",
        "Red Card", "Blue Card", "Green Card", "Spray", "Cartridge", "Ration B1", "Ration B2", "Ration B3",
    };

    MgSubGame s_eGame = MgSubGame::NONE;
    std::array<int, 2> s_anSelection {};
    std::string s_sNotice;

    bool Available(int nContext)
    {
        const auto eGame = static_cast<MgSubGame>(nContext);
        if (s_eGame != eGame)
        {
            s_eGame = eGame;
            s_anSelection = {};
            s_sNotice.clear();
        }
        if (eGame == MgSubGame::MG1)
        {
            return MG1_LinkVarBuf::GetGameRecord() && MG1_Gamefuncs::TestFlag;
        }
        return eGame == MgSubGame::MG2 && MG2_Gamefuncs::pfnGetEngineState && MG2_Gamefuncs::pfnGetEngineState();
    }

    std::vector<Entry> GetEntries(bool bWeapons)
    {
        if (s_eGame == MgSubGame::MG1)
        {
            return bWeapons ? std::vector<Entry>(std::begin(astMG1Weapons), std::end(astMG1Weapons)) : std::vector<Entry>(std::begin(astMG1Items), std::end(astMG1Items));
        }
        if (bWeapons)
        {
            return { std::begin(astMG2Weapons), std::end(astMG2Weapons) };
        }
        std::vector<Entry> vEntries;
        for (int nId = 0; nId < static_cast<int>(std::size(apszMG2Items)); nId++)
        {
            vEntries.push_back({ nId, apszMG2Items[nId] });
        }
        return vEntries;
    }

    int GetQuantity(int nId, bool bWeapons)
    {
        return s_eGame == MgSubGame::MG1 ? MG1_LinkVarBuf::GetInventoryQuantity(nId, bWeapons) : MG2_LinkVarBuf::GetInventoryQuantity(nId, bWeapons);
    }

    bool ChangeMG1(int nId, bool bWeapons, bool bAdd)
    {
        const auto eResult = MG1_LinkVarBuf::ChangeInventory(nId, bWeapons, bAdd);
        if (eResult == MG1_LinkVarBuf::InventoryChangeResult::InventoryFull)
        {
            s_sNotice = "Inventory full.";
        }
        return eResult == MG1_LinkVarBuf::InventoryChangeResult::Changed;
    }

    bool ChangeMG2(int nId, bool bWeapons, bool bAdd)
    {
        const auto eResult = MG2_LinkVarBuf::ChangeInventory(nId, bWeapons, bAdd);
        if (eResult == MG2_LinkVarBuf::InventoryChangeResult::InventoryFull)
        {
            s_sNotice = "Inventory full.";
        }
        else if (eResult == MG2_LinkVarBuf::InventoryChangeResult::WeaponsFull)
        {
            s_sNotice = "Weapon inventory full.";
        }
        return eResult == MG2_LinkVarBuf::InventoryChangeResult::Changed;
    }

    bool HandleInput(bool bWeapons, MG_DeveloperMenu::Input eInput)
    {
        using MG_DeveloperMenu::Input;
        const auto vEntries = GetEntries(bWeapons);
        int& nSelection = s_anSelection[bWeapons ? 1 : 0];
        const int nCount = static_cast<int>(vEntries.size());
        const int nStep = eInput == Input::Up ? -1 : eInput == Input::Down ? 1 : eInput == Input::Previous ? -10 : eInput == Input::Next ? 10 : 0;
        nSelection = ((nSelection + nStep) % nCount + nCount) % nCount;
        if (eInput == Input::Left || eInput == Input::Right)
        {
            s_sNotice.clear();
            const auto& stEntry = vEntries[nSelection];
            const bool bAdd = eInput == Input::Right;
            const bool bChanged = s_eGame == MgSubGame::MG1 ? ChangeMG1(stEntry.nId, bWeapons, bAdd) : ChangeMG2(stEntry.nId, bWeapons, bAdd);
            if (bChanged)
            {
                s_sNotice = std::string(bAdd ? "Added: " : "Removed: ") + stEntry.pszName;
            }
        }
        return false;
    }

    std::string GetText(bool bWeapons)
    {
        const auto vEntries = GetEntries(bWeapons);
        const int nSelection = s_anSelection[bWeapons ? 1 : 0];
        const int nFirst = (nSelection / 8) * 8;
        std::ostringstream stText;
        for (int nIndex = nFirst; nIndex < std::min(nFirst + 8, static_cast<int>(vEntries.size())); nIndex++)
        {
            const auto& stEntry = vEntries[nIndex];
            stText << (nIndex == nSelection ? "> " : "  ") << stEntry.pszName << "  [" << GetQuantity(stEntry.nId, bWeapons) << "]\n";
        }
        stText << "\nUp / Down: select   Page Up / Down: skip ten\nRight: add / ammo +1   Left: remove\n\n" << s_sNotice;
        return stText.str();
    }
}

void MG_InventoryMenu::Initialize()
{
    MG_DeveloperMenu::RegisterPage({ "mg.weapons", "Weapons", Available, []() { return GetText(true); }, [](MG_DeveloperMenu::Input eInput) { return HandleInput(true, eInput); } });
    MG_DeveloperMenu::RegisterPage({ "mg.items", "Equipment / Items", Available, []() { return GetText(false); }, [](MG_DeveloperMenu::Input eInput) { return HandleInput(false, eInput); } });
}
