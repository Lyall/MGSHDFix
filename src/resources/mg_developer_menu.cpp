#include "stdafx.h"
#include "mg_developer_menu.hpp"

#include "d3d11_api.hpp"
#include "d3d11_text_overlay.hpp"

namespace
{
    std::mutex s_stMutex;
    std::vector<MG_DeveloperMenu::Page> s_vPages;
    std::vector<size_t> s_vAvailablePages;
    std::array<bool, 9> s_abHeld {};
    int s_nContext = -1;
    int s_nSelection = 0;
    int s_nPage = -1;
    bool s_bAvailable = false;
    bool s_bOpen = false;
    bool s_bWaitForRelease = false;
    bool s_bCloseAfterRelease = false;
    bool s_bInputPending = false;
    MG_DeveloperMenu::Input s_eInput = MG_DeveloperMenu::Input::Activate;
    ULONGLONG s_nLastGameTick = 0;
    ULONGLONG s_nNoticeUntil = 0;
    std::string s_sPageText;
    std::vector<MG_DeveloperMenu::Column> s_vColumns;

    void CloseMenu()
    {
        s_bOpen = false;
        s_bWaitForRelease = false;
        s_bCloseAfterRelease = false;
        s_bInputPending = false;
        s_nPage = -1;
        s_sPageText.clear();
        s_vColumns.clear();
    }
}

void MG_DeveloperMenu::RegisterPage(Page stPage)
{
    const std::lock_guard<std::mutex> stLock(s_stMutex);
    if (stPage.sId.empty() || !stPage.fnAvailable || !stPage.fnText || !stPage.fnInput)
    {
        return;
    }
    for (const auto& stExisting : s_vPages)
    {
        if (stExisting.sId == stPage.sId)
        {
            return;
        }
    }
    s_vPages.push_back(std::move(stPage));
}

bool MG_DeveloperMenu::Service(int nContext, bool bAvailable)
{
    const std::lock_guard<std::mutex> stLock(s_stMutex);
    if (s_nContext != nContext)
    {
        CloseMenu();
        s_nContext = nContext;
        s_nSelection = 0;
    }
    s_nLastGameTick = GetTickCount64();
    std::vector<size_t> vAvailablePages;
    if (bEnabled && bAvailable)
    {
        for (size_t nIndex = 0; nIndex < s_vPages.size(); nIndex++)
        {
            if (s_vPages[nIndex].fnAvailable(nContext))
            {
                vAvailablePages.push_back(nIndex);
            }
        }
    }
    if (vAvailablePages != s_vAvailablePages)
    {
        CloseMenu();
        s_nSelection = 0;
        s_vAvailablePages = std::move(vAvailablePages);
    }
    s_bAvailable = bEnabled && bAvailable && !s_vAvailablePages.empty();
    if (!s_bAvailable)
    {
        CloseMenu();
        return false;
    }
    if (s_bOpen && s_nPage >= 0)
    {
        auto& stPage = s_vPages[s_vAvailablePages[s_nPage]];
        if (s_bInputPending && !s_bWaitForRelease)
        {
            s_bInputPending = false;
            if (stPage.fnInput(s_eInput))
            {
                CloseMenu();
                return false;
            }
        }
        s_sPageText = stPage.fnText();
        s_vColumns = stPage.fnColumns ? stPage.fnColumns() : std::vector<Column> {};
    }
    return s_bOpen;
}

std::string MG_DeveloperMenu::Tick(std::vector<Column>* pvColumns)
{
    if (pvColumns)
    {
        pvColumns->clear();
    }
    if (!bEnabled || !D3D11TextOverlay::bShaderLoaded)
    {
        return {};
    }
    const std::lock_guard<std::mutex> stLock(s_stMutex);
    const ULONGLONG nNow = GetTickCount64();
    const bool bForeground = GetForegroundWindow() == g_D3D11Hooks.MainHwnd;
    const std::array<int, 9> anKeys = { nHotkey, VK_ESCAPE, VK_RETURN, VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_PRIOR, VK_NEXT };
    std::array<bool, 9> abPressed {};
    bool bAnyHeld = false;
    for (size_t nIndex = 0; nIndex < anKeys.size(); nIndex++)
    {
        const bool bHeld = (GetAsyncKeyState(anKeys[nIndex]) & 0x8000) != 0;
        abPressed[nIndex] = bForeground && bHeld && !s_abHeld[nIndex];
        s_abHeld[nIndex] = bHeld;
        bAnyHeld |= bHeld;
    }
    if (nNow - s_nLastGameTick > 2000)
    {
        CloseMenu();
        s_bAvailable = false;
    }
    if (s_bWaitForRelease)
    {
        if (bForeground && !bAnyHeld)
        {
            s_bWaitForRelease = false;
            if (s_bCloseAfterRelease)
            {
                CloseMenu();
            }
        }
    }
    else if (abPressed[0])
    {
        if (s_bOpen)
        {
            s_bInputPending = false;
            s_bWaitForRelease = true;
            s_bCloseAfterRelease = true;
        }
        else if (s_bAvailable)
        {
            s_bOpen = true;
            s_nPage = -1;
            s_nNoticeUntil = 0;
        }
        else
        {
            s_nNoticeUntil = nNow + 3000;
        }
    }
    else if (s_bOpen && abPressed[1])
    {
        s_bInputPending = false;
        if (s_nPage >= 0)
        {
            s_nPage = -1;
            s_sPageText.clear();
            s_vColumns.clear();
        }
        else
        {
            s_bWaitForRelease = true;
            s_bCloseAfterRelease = true;
        }
    }
    else if (s_bOpen && !s_bInputPending)
    {
        if (s_nPage < 0)
        {
            const int nCount = static_cast<int>(s_vAvailablePages.size());
            s_nSelection = (s_nSelection + nCount + static_cast<int>(abPressed[6]) - static_cast<int>(abPressed[5])) % nCount;
            if (abPressed[2])
            {
                s_nPage = s_nSelection;
                s_sPageText.clear();
                s_vColumns.clear();
            }
        }
        else
        {
            const std::array<Input, 7> aeInputs = { Input::Activate, Input::Left, Input::Right, Input::Up, Input::Down, Input::Previous, Input::Next };
            for (size_t nIndex = 0; nIndex < aeInputs.size(); nIndex++)
            {
                if (abPressed[nIndex + 2])
                {
                    s_eInput = aeInputs[nIndex];
                    s_bInputPending = true;
                    s_bWaitForRelease = s_eInput == Input::Activate;
                    s_bCloseAfterRelease = false;
                    break;
                }
            }
        }
    }
    if (!s_bOpen)
    {
        return nNow < s_nNoticeUntil ? "Developer Menu: available during normal gameplay." : "";
    }
    std::ostringstream stText;
    stText << "DEVELOPER MENU\n\n";
    if (s_nPage < 0)
    {
        for (size_t nIndex = 0; nIndex < s_vAvailablePages.size(); nIndex++)
        {
            stText << (static_cast<int>(nIndex) == s_nSelection ? "> " : "  ") << s_vPages[s_vAvailablePages[nIndex]].sTitle << "\n";
        }
        stText << "\nUp / Down: select   Enter: open   Esc: close";
    }
    else
    {
        stText << s_vPages[s_vAvailablePages[s_nPage]].sTitle << "\n\n" << s_sPageText << "\n\nEsc: back";
    }
    if (s_bWaitForRelease)
    {
        stText << "\nRelease the menu keys to continue.";
    }
    if (pvColumns && s_nPage >= 0)
    {
        *pvColumns = s_vColumns;
    }
    return stText.str();
}
