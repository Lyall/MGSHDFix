#pragma once

namespace MG_DeveloperMenu
{
    enum class Input
    {
        Left,
        Right,
        Up,
        Down,
        Previous,
        Next,
        Activate
    };

    enum class ColumnColor
    {
        Normal,
        Active,
        SingleEntry
    };

    struct Column
    {
        std::string sText;
        float fWidth = 0.0f;
        ColumnColor eColor = ColumnColor::Normal;
        int nSelectedLine = -1;
        bool bYellowSelection = false;
    };

    struct Page
    {
        std::string sId;
        std::string sTitle;
        std::function<bool(int)> fnAvailable;
        std::function<std::string()> fnText;
        std::function<bool(Input)> fnInput; // Return true to close the menu.
        std::function<std::vector<Column>()> fnColumns;
    };

    void RegisterPage(Page stPage);
    // Game thread only; returns whether to pause gameplay.
    // Callbacks hold the menu lock, so re-entry would deadlock.
    bool Service(int nContext, bool bAvailable);
    std::string Tick(std::vector<Column>* pvColumns = nullptr);

    inline bool bEnabled = false;
    inline int nHotkey = VK_F8;
}
