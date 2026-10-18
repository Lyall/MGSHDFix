#include "stdafx.h"
#include "mg_stage_select.hpp"
#include "mg_stage_select_data.hpp"
#include "mg_inventory_menu.hpp"

#include "common.hpp"
#include "game_stages.hpp"
#include "d3d11_text_overlay.hpp"

namespace
{
    MG_StageSelect::Game s_eGame = MG_StageSelect::Game::MG1;
    int s_nArea = 0;
    int s_nEntrance = 0;
    enum class Column
    {
        Area,
        Room,
        Entrance
    };
    Column s_eColumn = Column::Area;
    constexpr int kVisibleRows = 10;
    int s_nCurrentArea = -1;
    int s_nCurrentRoom = -1;
    bool s_bTravelPending = false;
    bool s_bWasPaused = false;

    std::vector<MG_StageSelect::Destination> GetEntrances()
    {
        std::vector<MG_StageSelect::Destination> vEntrances;
        const auto AddArea = [&](const auto& astRooms, const auto& astDestinations)
        {
            for (const auto& stRoom : astRooms)
            {
                if (stRoom.nArea != s_nArea)
                {
                    continue;
                }
                const size_t nBefore = vEntrances.size();
                for (const auto& stDestination : astDestinations)
                {
                    if (stDestination.nArea == s_nArea && stDestination.nRoom == stRoom.nRoom)
                    {
                        vEntrances.push_back(stDestination);
                    }
                }
                if (s_eGame == MG_StageSelect::Game::MG2)
                {
                    for (const auto& stCrawl : mg_stage_select_data::astMG2CrawlEntrances)
                    {
                        if (stCrawl.nArea == s_nArea && stCrawl.nRoom == stRoom.nRoom)
                        {
                            vEntrances.push_back(stCrawl);
                        }
                    }
                }
                if (vEntrances.size() == nBefore)
                {
                    MG_StageSelect::Destination stSpawn { s_nArea, stRoom.nRoom, -1 };
                    if (s_eGame == MG_StageSelect::Game::MG2)
                    {
                        for (const auto& stFallback : mg_stage_select_data::astMG2RoomSpawns)
                        {
                            if (stFallback.nArea == s_nArea && stFallback.nRoom == stRoom.nRoom)
                            {
                                vEntrances.push_back(stFallback);
                            }
                        }
                    }
                    if (vEntrances.size() == nBefore)
                    {
                        vEntrances.push_back(stSpawn);
                    }
                }
            }
        };
        if (s_eGame == MG_StageSelect::Game::MG1)
        {
            AddArea(astMG1RoomNames, mg_stage_select_data::astMG1Destinations);
        }
        else
        {
            AddArea(astMG2RoomNames, mg_stage_select_data::astMG2Destinations);
        }
        const auto GetSortOrder = [](const MG_StageSelect::Destination& stDestination)
        {
            const MGRoomName* pRoom = FindMGRoomName(s_eGame == MG_StageSelect::Game::MG2, stDestination.nArea, stDestination.nRoom);
            return pRoom ? pRoom->nSortOrder : stDestination.nRoom * 10;
        };
        std::stable_sort(vEntrances.begin(), vEntrances.end(), [&](const auto& stLeft, const auto& stRight)
        {
            const int nLeftOrder = GetSortOrder(stLeft);
            const int nRightOrder = GetSortOrder(stRight);
            return nLeftOrder != nRightOrder ? nLeftOrder < nRightOrder : stLeft.nRoom < stRight.nRoom;
        });
        return vEntrances;
    }

    int GetAreaCount()
    {
        const auto CountAreas = [](const auto& astDestinations)
        {
            int nCount = 0;
            for (const auto& stDestination : astDestinations)
            {
                nCount = std::max(nCount, stDestination.nArea + 1);
            }
            return nCount;
        };
        return s_eGame == MG_StageSelect::Game::MG1 ? CountAreas(astMG1RoomNames) : CountAreas(astMG2RoomNames);
    }

    void SelectCurrentRoom(int nArea, int nRoom)
    {
        s_eColumn = Column::Area;
        s_nArea = std::clamp(nArea, 0, GetAreaCount() - 1);
        s_nEntrance = 0;
        const auto vEntrances = GetEntrances();
        for (size_t nIndex = 0; nIndex < vEntrances.size(); ++nIndex)
        {
            if (vEntrances[nIndex].nRoom == nRoom)
            {
                s_nEntrance = static_cast<int>(nIndex);
                break;
            }
        }
    }

    std::vector<int> GetColumnRows(const std::vector<MG_StageSelect::Destination>& vEntrances, Column eColumn)
    {
        std::vector<int> vRows;
        for (int nIndex = 0; nIndex < static_cast<int>(vEntrances.size()); ++nIndex)
        {
            if (eColumn == Column::Room)
            {
                if (nIndex == 0 || vEntrances[nIndex].nRoom != vEntrances[nIndex - 1].nRoom)
                {
                    vRows.push_back(nIndex);
                }
            }
            else if (vEntrances[nIndex].nRoom == vEntrances[s_nEntrance].nRoom)
            {
                vRows.push_back(nIndex);
            }
        }
        return vRows;
    }

    int GetSelectedRow(const std::vector<int>& vRows)
    {
        int nRow = 0;
        for (int nIndex = 0; nIndex < static_cast<int>(vRows.size()); ++nIndex)
        {
            if (vRows[nIndex] <= s_nEntrance)
            {
                nRow = nIndex;
            }
        }
        return nRow;
    }

    bool HandleInput(MG_DeveloperMenu::Input eInput)
    {
        using MG_DeveloperMenu::Input;
        if (eInput == Input::Left)
        {
            s_eColumn = static_cast<Column>(std::max(0, static_cast<int>(s_eColumn) - 1));
            return false;
        }
        if (eInput == Input::Activate && s_eColumn == Column::Room)
        {
            const auto vEntrances = GetEntrances();
            if (vEntrances.empty())
            {
                return false;
            }
            const int nRoom = vEntrances[s_nEntrance].nRoom;
            int nOnlyEntrance = -1;
            for (int nIndex = 0; nIndex < static_cast<int>(vEntrances.size()); ++nIndex)
            {
                const auto& stDestination = vEntrances[nIndex];
                if (stDestination.nRoom != nRoom || (stDestination.nEntrance < 0 && (stDestination.nSpawnX < 0 || stDestination.nSpawnY < 0)))
                {
                    continue;
                }
                if (nOnlyEntrance >= 0)
                {
                    s_eColumn = Column::Entrance;
                    return false;
                }
                nOnlyEntrance = nIndex;
            }
            if (nOnlyEntrance >= 0)
            {
                s_nEntrance = nOnlyEntrance;
                s_bTravelPending = true;
                return true;
            }
        }
        if (eInput == Input::Right || (eInput == Input::Activate && s_eColumn != Column::Entrance))
        {
            s_eColumn = static_cast<Column>(std::min(2, static_cast<int>(s_eColumn) + 1));
            return false;
        }
        int nStep = 0;
        switch (eInput)
        {
        case Input::Up: nStep = -1; break;
        case Input::Down: nStep = 1; break;
        case Input::Previous: nStep = -kVisibleRows; break;
        case Input::Next: nStep = kVisibleRows; break;
        default: break;
        }
        const auto WrapRow = [](int nRow, int nCount)
        {
            return (nRow % nCount + nCount) % nCount;
        };
        if (s_eColumn == Column::Area)
        {
            if (nStep != 0)
            {
                s_nArea = WrapRow(s_nArea + nStep, GetAreaCount());
                s_nEntrance = 0;
            }
            return false;
        }
        const auto vEntrances = GetEntrances();
        if (vEntrances.empty())
        {
            return false;
        }
        if (eInput == Input::Activate)
        {
            const auto& stDestination = vEntrances[s_nEntrance];
            if (stDestination.nEntrance < 0 && stDestination.nSpawnX < 0)
            {
                return false;
            }
            s_bTravelPending = true;
            return true;
        }
        if (nStep != 0)
        {
            const auto vRows = GetColumnRows(vEntrances, s_eColumn);
            s_nEntrance = vRows[WrapRow(GetSelectedRow(vRows) + nStep, static_cast<int>(vRows.size()))];
        }
        return false;
    }

    const std::string& GetAreaName(int nArea)
    {
        const auto BuildNames = [](const auto& astRooms)
        {
            std::vector<std::map<std::string, int>> vLocations;
            for (const auto& stRoom : astRooms)
            {
                vLocations.resize(std::max(vLocations.size(), static_cast<size_t>(stRoom.nArea + 1)));
                if (stRoom.pszLocation[0])
                {
                    ++vLocations[stRoom.nArea][stRoom.pszLocation];
                }
            }
            std::vector<std::string> vNames(vLocations.size(), "wip -> yell at afevis");
            for (size_t nIndex = 0; nIndex < vLocations.size(); ++nIndex)
            {
                int nMostRooms = 0;
                for (const auto& [sLocation, nRooms] : vLocations[nIndex])
                {
                    if (nRooms > nMostRooms)
                    {
                        nMostRooms = nRooms;
                        vNames[nIndex] = sLocation;
                    }
                }
            }
            return vNames;
        };
        static const auto vMG1Names = BuildNames(astMG1RoomNames);
        static const auto vMG2Names = BuildNames(astMG2RoomNames);
        return (s_eGame == MG_StageSelect::Game::MG1 ? vMG1Names : vMG2Names)[nArea];
    }

    std::string GetRoomColumnName(const MGRoomName& stRoom)
    {
        if (stRoom.pszLocation[0] && GetAreaName(stRoom.nArea) != stRoom.pszLocation)
        {
            return std::string(stRoom.pszLocation) + ": " + stRoom.pszName;
        }
        return stRoom.pszName;
    }

    std::string GetEntranceName(const MG_StageSelect::Destination& stDestination)
    {
        if (stDestination.nEntrance >= 0)
        {
            return "Entrance " + std::to_string(stDestination.nEntrance);
        }
        if (stDestination.nSpawnX < 0)
        {
            return "No spawn available";
        }
        return stDestination.pszSpawnName;
    }

    const std::array<float, 3>& GetColumnWidths()
    {
        const auto BuildWidths = [](MG_StageSelect::Game eGame, const auto& astRooms, const auto& astDestinations)
        {
            std::array<float, 3> afWidths {};
            const auto Measure = [&](int nColumn, const std::string& sText)
            {
                afWidths[nColumn] = std::max(afWidths[nColumn], D3D11TextOverlay::GetTextWidth(sText.c_str()));
                std::string sUnselected = sText;
                sUnselected[0] = ' ';
                afWidths[nColumn] = std::max(afWidths[nColumn], D3D11TextOverlay::GetTextWidth(sUnselected.c_str()));
            };
            Measure(0, "> AREA\n999 / 999");
            Measure(1, "> ROOM\n999 / 999");
            Measure(2, "> ENTRANCE\n999 / 999");
            for (int nArea = 0; nArea < GetAreaCount(); ++nArea)
            {
                Measure(0, "> " + std::to_string(nArea) + "  " + GetAreaName(nArea));
            }
            for (const auto& stDestination : astRooms)
            {
                Measure(1, "> " + std::to_string(stDestination.nRoom) + "  " + GetRoomColumnName(stDestination));
            }
            if (eGame == MG_StageSelect::Game::MG2)
            {
                for (const auto& stCrawl : mg_stage_select_data::astMG2CrawlEntrances)
                {
                    Measure(2, "> " + GetEntranceName(stCrawl));
                }
                for (const auto& stSpawn : mg_stage_select_data::astMG2RoomSpawns)
                {
                    Measure(2, "> " + GetEntranceName(stSpawn));
                }
            }
            Measure(2, "> No spawn available");
            for (const auto& stDestination : astDestinations)
            {
                Measure(2, "> " + GetEntranceName(stDestination));
            }
            return afWidths;
        };
        if (s_eGame == MG_StageSelect::Game::MG1)
        {
            static const auto afWidths = BuildWidths(MG_StageSelect::Game::MG1, astMG1RoomNames, mg_stage_select_data::astMG1Destinations);
            return afWidths;
        }
        static const auto afWidths = BuildWidths(MG_StageSelect::Game::MG2, astMG2RoomNames, mg_stage_select_data::astMG2Destinations);
        return afWidths;
    }

    std::vector<MG_DeveloperMenu::Column> GetColumns()
    {
        const auto vEntrances = GetEntrances();
        std::vector<MG_DeveloperMenu::Column> vColumns;
        for (Column eColumn : { Column::Area, Column::Room, Column::Entrance })
        {
            const auto vRows = GetColumnRows(vEntrances, eColumn);
            const int nCount = eColumn == Column::Area ? GetAreaCount() : static_cast<int>(vRows.size());
            const int nSelected = eColumn == Column::Area ? s_nArea : GetSelectedRow(vRows);
            const int nFirst = std::clamp(nSelected - kVisibleRows / 2, 0, std::max(0, nCount - kVisibleRows));
            const char* pszTitle = eColumn == Column::Area ? "AREA" : eColumn == Column::Room ? "ROOM" : "ENTRANCE";
            std::ostringstream stText;
            stText << (eColumn == s_eColumn ? "> " : "  ") << pszTitle << "\n";
            stText << (nCount ? nSelected + 1 : 0) << " / " << nCount << "\n\n";
            for (int nRow = nFirst; nRow < std::min(nCount, nFirst + kVisibleRows); ++nRow)
            {
                stText << (nRow == nSelected ? "> " : "  ");
                if (eColumn == Column::Area)
                {
                    stText << nRow << "  " << GetAreaName(nRow);
                }
                else
                {
                    const auto& stDestination = vEntrances[vRows[nRow]];
                    if (eColumn == Column::Room)
                    {
                        const MGRoomName* pRoom = FindMGRoomName(s_eGame == MG_StageSelect::Game::MG2, s_nArea, stDestination.nRoom);
                        stText << stDestination.nRoom << "  " << (pRoom ? GetRoomColumnName(*pRoom) : "Unnamed room");
                    }
                    else
                    {
                        stText << GetEntranceName(stDestination);
                    }
                }
                stText << "\n";
            }
            const auto eColor = eColumn == Column::Entrance && nCount == 1 ? MG_DeveloperMenu::ColumnColor::SingleEntry : eColumn == s_eColumn ? MG_DeveloperMenu::ColumnColor::Active : MG_DeveloperMenu::ColumnColor::Normal;
            vColumns.push_back({ stText.str(), GetColumnWidths()[static_cast<size_t>(eColumn)], eColor, nCount ? nSelected - nFirst + 3 : -1, eColumn == Column::Area && s_eColumn == Column::Area });
        }
        return vColumns;
    }

    std::string GetPageText()
    {
        std::ostringstream stText;
        stText << "Current: " << GetMGRoomName(s_eGame == MG_StageSelect::Game::MG2, s_nCurrentArea, s_nCurrentRoom) << "\nArea " << s_nCurrentArea << "   Room " << s_nCurrentRoom << "\n\n";
        stText << "Left / Right: column   Up / Down: select\nPage Up / Down: skip ten   Enter: next column / travel";
        return stText.str();
    }
}

MG_StageSelect::Action MG_StageSelect::Service(Game eGame, bool bAvailable, int nCurrentArea, int nCurrentRoom, Destination& stDestination)
{
    static bool s_bRegistered = false;
    if (!s_bRegistered)
    {
        MG_DeveloperMenu::RegisterPage({ "mg.stage_select", "Stage Select", [](int nContext) { return nContext == static_cast<int>(MgSubGame::MG1) || nContext == static_cast<int>(MgSubGame::MG2); }, GetPageText, HandleInput, GetColumns });
        MG_InventoryMenu::Initialize();
        s_bRegistered = true;
        spdlog::info("MG1/MG2: Stage Select : Menu pages registered.");
    }
    const bool bGameChanged = s_eGame != eGame;
    s_eGame = eGame;
    if (bGameChanged || (!s_bWasPaused && (s_nCurrentArea != nCurrentArea || s_nCurrentRoom != nCurrentRoom)))
    {
        SelectCurrentRoom(nCurrentArea, nCurrentRoom);
        s_bTravelPending = false;
    }
    s_nCurrentArea = nCurrentArea;
    s_nCurrentRoom = nCurrentRoom;
    const bool bPaused = MG_DeveloperMenu::Service(static_cast<int>(eGame == Game::MG1 ? MgSubGame::MG1 : MgSubGame::MG2), bAvailable);
    const bool bWasPaused = s_bWasPaused;
    s_bWasPaused = bPaused;
    if (s_bTravelPending)
    {
        s_bTravelPending = false;
        const auto vEntrances = GetEntrances();
        if (bAvailable && s_nEntrance >= 0 && s_nEntrance < static_cast<int>(vEntrances.size()))
        {
            stDestination = vEntrances[s_nEntrance];
            return Action::Travel;
        }
    }
    if (!bPaused && bWasPaused)
    {
        SelectCurrentRoom(nCurrentArea, nCurrentRoom);
    }
    return bPaused ? Action::Pause : Action::Run;
}
