// ============================================================================
// Project:   Universal Config Tool
// File:      main.cpp
//
// Copyright (c) 2025 Afevis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// ============================================================================


#include "pch.h"
#include "config_keys.hpp"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/stdpaths.h>
#include <wx/mstream.h>
#include <SDL3/SDL.h>

#include "helper.hpp"
#include "version.h"
#include "tab_data.hpp"
#include "updater.hpp"

#if !defined(MGSHDFIX_SPECIFIC)
#define MGSHDFIX_SPECIFIC // MGSHDFix has unique regional/language options. Avoid conflicts with other projects using the same codebase.
#endif


constexpr int iWindowSizeX = 716;
constexpr int iWindowSizeY = 700;
constexpr const char* sSettingsFileName = "MGSHDFix.settings";
constexpr bool bFullLengthFields = false; //if you want the boxes to span half the window's width.

#define NEXUS_MG1_URL "https://www.nexusmods.com/metalgearandmetalgear2mc/mods/9"
#define NEXUS_MGS2_URL "https://www.nexusmods.com/metalgearsolid2mc/mods/49"
#define NEXUS_MGS3_URL "https://www.nexusmods.com/metalgearsolid3mc/mods/139"


#include <wx/log.h>
#include <wx/msgdlg.h>

class wxLogErrorsOnlyGui final : public wxLog
{
public:
    explicit wxLogErrorsOnlyGui(wxWindow* parent = nullptr)
        : m_parent(parent)
    {
    }

protected:
    void DoLogTextAtLevel(wxLogLevel level, const wxString& msg) override
    {
        // ONLY errors (and fatal errors if wxWidgets uses them)
        if (level != wxLOG_Error && level != wxLOG_FatalError)
        {
            return;
        }

        wxMessageBox(
            msg,
            "MGSHDFix Error",
            wxOK | wxICON_ERROR,
            m_parent
        );
    }

private:
    wxWindow* m_parent = nullptr;
};


// ---------------------------------------------------------------------------
// Quote/unquote helpers for INI-safe string persistence
// ---------------------------------------------------------------------------
static wxString QuoteIfNeeded(const wxString& value)
{
    wxString escaped = value;
    escaped.Replace("\"", "\\\""); // escape internal quotes
    return "\"" + escaped + "\""; // always quote
}

static wxString Unquote(const wxString& value)
{
    wxString s = value;
    if (s.StartsWith("\"") && s.EndsWith("\"") && s.length() >= 2)
    {
        s = s.Mid(1, s.length() - 2);
        s.Replace("\\\"", "\""); // unescape
    }
    return s;
}


namespace
{
    constexpr int kPadTriggerThreshold = 8192;
    constexpr int kPadStickThreshold = 12000;

    bool g_SDLGamepadInitialized = false;
    std::vector<SDL_Gamepad*> g_OpenGamepads;

    bool EnsureSDLGamepadInitialized()
    {
        if (g_SDLGamepadInitialized)
        {
            return true;
        }

        g_SDLGamepadInitialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
        if (!g_SDLGamepadInitialized)
        {
            wxLogError("Failed to initialize SDL Gamepad support:\n\n%s", SDL_GetError());
            return false;
        }

        return true;
    }

    void CloseOpenGamepads()
    {
        for (SDL_Gamepad* gamepad : g_OpenGamepads)
        {
            if (gamepad != nullptr)
            {
                SDL_CloseGamepad(gamepad);
            }
        }

        g_OpenGamepads.clear();
    }

    void RefreshOpenGamepads()
    {
        if (!EnsureSDLGamepadInitialized())
        {
            return;
        }

        CloseOpenGamepads();

        SDL_PumpEvents();
        SDL_UpdateGamepads();

        int gamepadCount = 0;
        SDL_JoystickID* gamepadIds = SDL_GetGamepads(&gamepadCount);
        if (gamepadIds == nullptr)
        {
            return;
        }

        for (int i = 0; i < gamepadCount; ++i)
        {
            SDL_Gamepad* gamepad = SDL_OpenGamepad(gamepadIds[i]);
            if (gamepad != nullptr)
            {
                g_OpenGamepads.push_back(gamepad);
            }
        }

        SDL_free(gamepadIds);
    }

    bool TryGetGamepadButtonInputName(SDL_Gamepad* gamepad, wxString& outName)
    {
        if (gamepad == nullptr)
        {
            return false;
        }

        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH)) { outName = "Pad_A"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST)) { outName = "Pad_B"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST)) { outName = "Pad_X"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH)) { outName = "Pad_Y"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) { outName = "Pad_LB"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) { outName = "Pad_RB"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK)) { outName = "Pad_Back"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START)) { outName = "Pad_Start"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK)) { outName = "Pad_LStick"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK)) { outName = "Pad_RStick"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP)) { outName = "Pad_DPad_Up"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) { outName = "Pad_DPad_Down"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) { outName = "Pad_DPad_Left"; return true; }
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) { outName = "Pad_DPad_Right"; return true; }
        if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >= kPadTriggerThreshold) { outName = "Pad_LT"; return true; }
        if (SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) >= kPadTriggerThreshold) { outName = "Pad_RT"; return true; }

        return false;
    }

    bool TryGetGamepadStickInputName(SDL_Gamepad* gamepad, wxString& outName)
    {
        if (gamepad == nullptr)
        {
            return false;
        }

        const int leftX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        const int leftY = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        const int rightX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        const int rightY = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);

        if (leftY <= -kPadStickThreshold) { outName = "Pad_LThumb_Up"; return true; }
        if (leftY >= kPadStickThreshold) { outName = "Pad_LThumb_Down"; return true; }
        if (leftX <= -kPadStickThreshold) { outName = "Pad_LThumb_Left"; return true; }
        if (leftX >= kPadStickThreshold) { outName = "Pad_LThumb_Right"; return true; }
        if (rightY <= -kPadStickThreshold) { outName = "Pad_RThumb_Up"; return true; }
        if (rightY >= kPadStickThreshold) { outName = "Pad_RThumb_Down"; return true; }
        if (rightX <= -kPadStickThreshold) { outName = "Pad_RThumb_Left"; return true; }
        if (rightX >= kPadStickThreshold) { outName = "Pad_RThumb_Right"; return true; }

        return false;
    }
}

class HotkeyCaptureCtrl final : public wxTextCtrl
{
public:
    HotkeyCaptureCtrl(wxWindow* parent, wxWindowID id, const wxString& value = "", bool captureStickInputs = false)
        : wxTextCtrl(parent, id, value, wxDefaultPosition, wxDefaultSize,
                     wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER),
          m_gamepadCaptureTimer(this),
          m_captureStickInputs(captureStickInputs)
    {
        Bind(wxEVT_KEY_DOWN, &HotkeyCaptureCtrl::OnKeyDown, this);
        Bind(wxEVT_MIDDLE_DOWN, &HotkeyCaptureCtrl::OnMouseClick, this);
        Bind(wxEVT_AUX1_DOWN, &HotkeyCaptureCtrl::OnMouseClick, this);
        Bind(wxEVT_AUX2_DOWN, &HotkeyCaptureCtrl::OnMouseClick, this);
        Bind(wxEVT_MOUSEWHEEL, &HotkeyCaptureCtrl::OnMouseWheel, this);
        Bind(wxEVT_SET_FOCUS, &HotkeyCaptureCtrl::OnFocus, this);
        Bind(wxEVT_KILL_FOCUS, &HotkeyCaptureCtrl::OnKillFocus, this);
        Bind(wxEVT_TIMER, &HotkeyCaptureCtrl::OnGamepadTimer, this, m_gamepadCaptureTimer.GetId());
    }

    ~HotkeyCaptureCtrl() override
    {
        StopGamepadCapture();
    }

    void StopGamepadCapture()
    {
        if (m_gamepadCaptureTimer.IsRunning())
        {
            m_gamepadCaptureTimer.Stop();
        }
    }

    void CancelGamepadCaptureFocus()
    {
        StopGamepadCapture();
        SetInsertionPointEnd();
        SetSelection(GetLastPosition(), GetLastPosition());
    }

private:
    wxTimer m_gamepadCaptureTimer;
    bool m_captureStickInputs = false;

    void OnFocus(wxFocusEvent& event)
    {
        if (EnsureSDLGamepadInitialized())
        {
            if (g_OpenGamepads.empty())
            {
                RefreshOpenGamepads();
            }

            m_gamepadCaptureTimer.Start(16);
        }

        event.Skip();
    }

    void OnKillFocus(wxFocusEvent& event)
    {
        StopGamepadCapture();
        event.Skip();
    }

    void OnGamepadTimer(wxTimerEvent&)
    {
        if (!EnsureSDLGamepadInitialized())
        {
            return;
        }

        if (g_OpenGamepads.empty())
        {
            RefreshOpenGamepads();
            if (g_OpenGamepads.empty())
            {
                return;
            }
        }

        SDL_PumpEvents();
        SDL_UpdateGamepads();

        bool hasDisconnectedGamepad = false;

        for (SDL_Gamepad* gamepad : g_OpenGamepads)
        {
            if (gamepad == nullptr || !SDL_GamepadConnected(gamepad))
            {
                hasDisconnectedGamepad = true;
                continue;
            }

            wxString name;
            const bool hasInput = m_captureStickInputs
                ? TryGetGamepadStickInputName(gamepad, name)
                : TryGetGamepadButtonInputName(gamepad, name);

            if (!hasInput)
            {
                continue;
            }

            SetValue(name);
            return;
        }

        if (hasDisconnectedGamepad)
        {
            RefreshOpenGamepads();
        }
    }

    void OnKeyDown(const wxKeyEvent& event)
    {
        const int code = event.GetKeyCode();
        const int raw = event.GetRawKeyCode(); // Windows VK code

        // Left/right modifiers
        if (GetKeyState(VK_LMENU) & 0x8000)
        {
            SetValue("LAlt");  return;
        }
        if (GetKeyState(VK_RMENU) & 0x8000)
        {
            SetValue("RAlt");  return;
        }
        if (GetKeyState(VK_LCONTROL) & 0x8000)
        {
            SetValue("LCtrl"); return;
        }
        if (GetKeyState(VK_RCONTROL) & 0x8000)
        {
            SetValue("RCtrl"); return;
        }
        if (GetKeyState(VK_LSHIFT) & 0x8000)
        {
            SetValue("LShift"); return;
        }
        if (GetKeyState(VK_RSHIFT) & 0x8000)
        {
            SetValue("RShift"); return;
        }
        if (GetKeyState(VK_LWIN) & 0x8000)
        {
            SetValue("LWin");  return;
        }
        if (GetKeyState(VK_RWIN) & 0x8000)
        {
            SetValue("RWin");  return;
        }

        // Function keys
        if (code >= WXK_F1 && code <= WXK_F24)
        {
            SetValue(wxString::Format("F%d", code - WXK_F1 + 1));
            return;
        }

        // Letters
        if (code >= 'A' && code <= 'Z')
        {
            SetValue(wxString::Format("%c", code));
            return;
        }

        // Numbers
        if (code >= '0' && code <= '9')
        {
            SetValue(wxString::Format("%c", code));
            return;
        }

        // Special key mapping table
        switch (raw)
        {

        case VK_MBUTTON:   SetValue("Mouse3"); return;
        case VK_XBUTTON1:  SetValue("Mouse4"); return;
        case VK_XBUTTON2:  SetValue("Mouse5"); return;

        case VK_CAPITAL:   SetValue("CapsLock"); return;
        case VK_SHIFT:     SetValue("Shift"); return;
        case VK_CONTROL:   SetValue("Ctrl"); return;
        case VK_MENU:      SetValue("Alt"); return;
        case VK_LMENU:     SetValue("LAlt"); return;
        case VK_RMENU:     SetValue("RAlt"); return;
        case VK_LCONTROL:  SetValue("LCtrl"); return;
        case VK_RCONTROL:  SetValue("RCtrl"); return;
        case VK_LSHIFT:    SetValue("LShift"); return;
        case VK_RSHIFT:    SetValue("RShift"); return;
        case VK_LWIN:      SetValue("LWin"); return;
        case VK_RWIN:      SetValue("RWin"); return;
        case VK_BACK:      SetValue("Backspace"); return;
        case VK_TAB:       SetValue("Tab"); return;
        case VK_RETURN:    SetValue("Enter"); return;
        case VK_ESCAPE:    SetValue("Esc"); return;
        case VK_SPACE:     SetValue("Space"); return;
        case VK_PRIOR:     SetValue("PageUp"); return;
        case VK_NEXT:      SetValue("PageDown"); return;
        case VK_END:       SetValue("End"); return;
        case VK_HOME:      SetValue("Home"); return;
        case VK_LEFT:      SetValue("Left"); return;
        case VK_UP:        SetValue("Up"); return;
        case VK_RIGHT:     SetValue("Right"); return;
        case VK_DOWN:      SetValue("Down"); return;
        case VK_INSERT:    SetValue("Insert"); return;
        case VK_DELETE:    SetValue("Delete"); return;
        case VK_SNAPSHOT:  SetValue("PrintScreen"); return;
        case VK_SCROLL:    SetValue("ScrollLock"); return;
        case VK_PAUSE:     SetValue("Pause"); return;
        case VK_APPS:      SetValue("Menu"); return;
        case VK_OEM_MINUS: SetValue("-"); return;
        case VK_OEM_PLUS:  SetValue("="); return;
        case VK_OEM_4:     SetValue("["); return;
        case VK_OEM_6:     SetValue("]"); return;
        case VK_OEM_1:     SetValue(";"); return;
        case VK_OEM_7:     SetValue("'"); return;
        case VK_OEM_COMMA: SetValue(","); return;
        case VK_OEM_PERIOD:SetValue("."); return;
        case VK_OEM_2:     SetValue("/"); return;
        case VK_OEM_5:     SetValue("\\"); return;
        case VK_OEM_3:     SetValue("`"); return;

            // Numpad
        case VK_NUMPAD0:   SetValue("Num0"); return;
        case VK_NUMPAD1:   SetValue("Num1"); return;
        case VK_NUMPAD2:   SetValue("Num2"); return;
        case VK_NUMPAD3:   SetValue("Num3"); return;
        case VK_NUMPAD4:   SetValue("Num4"); return;
        case VK_NUMPAD5:   SetValue("Num5"); return;
        case VK_NUMPAD6:   SetValue("Num6"); return;
        case VK_NUMPAD7:   SetValue("Num7"); return;
        case VK_NUMPAD8:   SetValue("Num8"); return;
        case VK_NUMPAD9:   SetValue("Num9"); return;
        case VK_DECIMAL:   SetValue("NumDecimal"); return;
        case VK_DIVIDE:    SetValue("NumDivide"); return;
        case VK_MULTIPLY:  SetValue("NumMultiply"); return;
        case VK_SUBTRACT:  SetValue("NumMinus"); return;
        case VK_ADD:       SetValue("NumPlus"); return;
        case VK_NUMLOCK:   SetValue("NumLock"); return;
        }

        // Fallback to VK code
        SetValue(wxString::Format("VK_%02X", raw));
    }

    void OnMouseClick(wxMouseEvent& event)
    {
        wxString name;

        if (event.GetButton() == wxMOUSE_BTN_MIDDLE)
        {
            name = "Mouse3";
        }
        else if (event.GetButton() == wxMOUSE_BTN_AUX1)
        {
            name = "Mouse4";
        }
        else if (event.GetButton() == wxMOUSE_BTN_AUX2)
        {
            name = "Mouse5";
        }

        if (name.IsEmpty())
        {
            event.Skip();
            return;
        }

        SetValue(name);
    }

    void OnMouseWheel(wxMouseEvent& event)
    {
        const int wheelRotation = event.GetWheelRotation();

        if (wheelRotation > 0)
        {
            SetValue("WheelUp");
            return;
        }

        if (wheelRotation < 0)
        {
            SetValue("WheelDown");
            return;
        }

        event.Skip();
    }
};

// Resource Loader Helpers
static const void* FindResourceData(int resID, const wchar_t* resType)
{
    HRSRC hRes = FindResourceW(nullptr, MAKEINTRESOURCEW(resID), resType);
    if (!hRes) return nullptr;
    HGLOBAL hData = LoadResource(nullptr, hRes);
    return LockResource(hData);
}

static size_t FindResourceSize(int resID, const wchar_t* resType)
{
    HRSRC hRes = FindResourceW(nullptr, MAKEINTRESOURCEW(resID), resType);
    if (!hRes)
    {
        return 0;
    }
    return SizeofResource(nullptr, hRes);
}

static int GetBannerResourceID()
{
    const std::filesystem::path exePath = Helper::FindASILocation(sFixName).parent_path();


#pragma region CrashWarnings
    //These crash warnings are also in src\warnings\asi_loader_checks.cpp / ASILoaderCompatibility::Check(), make sure to keep them in sync.
    if (std::filesystem::exists(exePath / "d3d11.dll") && (Helper::GetFileDescription((exePath / "d3d11.dll").string()) == kAsiLoaderDescription))
    {
        wxLogError("DUPLICATE MOD LOADER ERROR: Multiple ASI Loader .dll installations detected! This can cause inconsistent bugs and crashes.\n"
            "\n"
            "Please delete d3d11.dll, it has been replaced by winhttp.dll & wininet.dll.");
        if (Helper::IsSteamOS())
        {
            wxLogError("\nSteam Deck / Linux users must also replace their Steam game launch paramaters with the following command:\n"
                "\n"
                "WINEDLLOVERRIDES=\"wininet,winhttp=n,b\" % command %");
        }
    }

    if (std::filesystem::exists(exePath / "dxgi.dll") &&
        Helper::GetFileDescription((exePath / "dxgi.dll").string()) == "File description not found.")
    {
        wxLogError("DUPLICATE MOD LOADER ERROR: Multiple ASI Loader .dll installations detected! This can cause inconsistent bugs and crashes.\n"
            "\n"
            "Please delete dxgi.dll, it has been replaced by winhttp.dll & wininet.dll.");
    }
#pragma endregion

    if (std::filesystem::exists(exePath / "METAL GEAR.exe"))
    {
        iTargetGame = TARGET_GAME_MG1;
        return IDB_BANNER_MG1;
    }
    if (std::filesystem::exists(exePath / "METAL GEAR SOLID2.exe"))
    {
        iTargetGame = TARGET_GAME_MGS2;
        return IDB_BANNER_MGS2;
    }
    if (std::filesystem::exists(exePath / "METAL GEAR SOLID3.exe"))
    {
        iTargetGame = TARGET_GAME_MGS3;
        return IDB_BANNER_MGS3;
    }

    wxLogError("MGSHDFix was found, but no supported game executable exists in:\n\n%s", exePath.string());
    ExitProcess(1);
    return IDB_BANNER_MG1;
}


// Custom fixed-size banner panel
class BannerPanel : public wxPanel
{
public:
    BannerPanel(wxWindow* parent, int bannerResId)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition,
                  parent->FromDIP(wxSize(700, 100)), wxBORDER_NONE)
    {
        wxImage img;
        wxMemoryInputStream memStream(
            FindResourceData(bannerResId, L"PNG"),
            FindResourceSize(bannerResId, L"PNG")
        );
        if (img.LoadFile(memStream, wxBITMAP_TYPE_PNG) && img.IsOk())
        {
            m_image = img;            // keep the source image for rescaling
            m_bitmap = wxBitmap(img);
        }

        const wxSize banner = FromDIP(wxSize(700, 100));
        SetMinSize(banner);
        SetMaxSize(banner);
        Bind(wxEVT_PAINT, &BannerPanel::OnPaint, this);
        SetBackgroundStyle(wxBG_STYLE_PAINT); // Needed for buffered paint
    }

private:
    wxImage  m_image;   // unscaled source
    wxBitmap m_bitmap;  // cached bitmap scaled to the current client size
    wxSize   m_cachedFor = wxSize(-1, -1);

    void OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        const wxSize sz = GetClientSize();
        if (m_image.IsOk() && sz.x > 0 && sz.y > 0)
        {
            if (sz != m_cachedFor)
            {
                m_bitmap = wxBitmap(
                    m_image.Scale(sz.x, sz.y, wxIMAGE_QUALITY_HIGH));
                m_cachedFor = sz;
            }
            if (m_bitmap.IsOk())
            {
                dc.DrawBitmap(m_bitmap, 0, 0, false);
            }
        }
    }
};

class ConfigFrame : public wxFrame
{
public:
    ConfigFrame()
        : wxFrame(nullptr, wxID_ANY, FIX_NAME " v" VERSION_STRING " - Universal Config Tool",
                  wxDefaultPosition, wxDefaultSize,
                  wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
    {
        const wxSize clientSize = FromDIP(wxSize(iWindowSizeX, iWindowSizeY));
        SetClientSize(clientSize);
        SetMinClientSize(clientSize);
        SetMaxClientSize(clientSize);

        HWND hwnd = (HWND)GetHWND();
        HINSTANCE instance = GetModuleHandleW(nullptr);

        m_iconSmall = (HICON)LoadImageW(
            instance,
            MAKEINTRESOURCEW(IDI_ICON1),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON),
            LR_DEFAULTCOLOR
        );

        m_iconBig = (HICON)LoadImageW(
            instance,
            MAKEINTRESOURCEW(IDI_ICON1),
            IMAGE_ICON,
            GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON),
            LR_DEFAULTCOLOR
        );

        if (m_iconSmall)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_iconSmall);
        }

        if (m_iconBig)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_iconBig);
        }

        if (!std::filesystem::exists(std::filesystem::path(m_iniPath.ToStdWstring())))
        {
            m_firstRun = true;
        }
        m_conf = new wxFileConfig("", "", m_iniPath, "", wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);

        m_focusSink = new wxTextCtrl(this, wxID_ANY, "", wxPoint(-10000, -10000), wxSize(1, 1), wxTE_READONLY | wxBORDER_NONE);

        const int bannerID = GetBannerResourceID();
        const int targetGameFlag =
            iTargetGame == TARGET_GAME_MG1 ? MG :
            iTargetGame == TARGET_GAME_MGS2 ? MGS2 :
            iTargetGame == TARGET_GAME_MGS3 ? MGS3 : 0;

        m_tabs = new wxNotebook(this, wxID_ANY);
        m_tabs->Bind(wxEVT_NOTEBOOK_PAGE_CHANGING, [this](wxBookCtrlEvent& event)
                     {
                         ClearHotkeyCaptureFocus();
                         event.Skip();
                     });
        m_tabs->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& event)
                     {
                         ClearHotkeyCaptureFocus();
                         event.Skip();
                     });

        for (auto& tab : kTabs)
        {
            wxPanel* panel = new wxPanel(m_tabs);
            wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
            wxString currentSection;
            wxStaticBoxSizer* sectionSizer = nullptr;
            wxFlexGridSizer* grid = nullptr;

            for (auto& field : tab.second)
            {
                const bool fieldVisible = (field.gameFlags & targetGameFlag) != 0;

                if (field.section != currentSection)
                {
                    currentSection = field.section;
                    sectionSizer = new wxStaticBoxSizer(wxVERTICAL, panel, currentSection);
                    grid = new wxFlexGridSizer(0, 4, 5, 10);
                    grid->AddGrowableCol(1, 1);
                    grid->AddGrowableCol(3, 1);
                    sectionSizer->Add(grid, 0, wxEXPAND | wxALL, 5);
                    vbox->Add(sectionSizer, 0, wxEXPAND | wxALL, 5);

                    const bool sectionVisible = std::any_of(tab.second.begin(), tab.second.end(), [&](const Field& sectionField)
                    {
                        return sectionField.section == currentSection && (sectionField.gameFlags & targetGameFlag) != 0;
                    });
                    vbox->Show(sectionSizer, sectionVisible);
                }

                if (field.section == "About")
                {
                    if (!fieldVisible)
                    {
                        continue;
                    }

                    wxBoxSizer* aboutSizer = new wxBoxSizer(wxVERTICAL);

                    auto* aboutText = new wxStaticText(
                        sectionSizer->GetStaticBox(),
                        wxID_ANY,
                        "Universal Config Tool, licensed under MIT.\n" //Do not remove this notice.
                        "     Created by Afevis.\n"                            //Do not remove this notice.
                        "     Gamepad support powered by SDL3, licensed under zlib.\n" //Do not remove this notice.
                        "\n"
                        "MGSHDFix, licensed under MIT.\n"
                        "     Maintained by Afevis (aka ShizCalev.)\n"
                        "     Originally created by Lyall.\n"
                        "     Contributors: Emoose, Cipherxof (aka TriggerHappy), Bud11, SpaceCore (aka Jacky720), gibletto, Zenf0."
                    );
                    aboutSizer->Add(aboutText, 0, wxALL, 5);

                    sectionSizer->Add(aboutSizer, 0, wxEXPAND | wxALL, 5);
                    continue; // skip normal control creation
                }

                // Label + optional help stacked vertically
                if (fieldVisible)
                {
                    wxBoxSizer* labelBox = new wxBoxSizer(wxVERTICAL);
                    labelBox->Add(new wxStaticText(sectionSizer->GetStaticBox(), wxID_ANY, field.key),
                                  0, wxALIGN_LEFT | wxBOTTOM, 2);
                    if (!field.help.IsEmpty())
                    {
                        auto* helpText = new wxStaticText(sectionSizer->GetStaticBox(), wxID_ANY, field.help);
                        helpText->SetForegroundColour(*wxBLUE);
                        labelBox->Add(helpText, 0, wxALIGN_LEFT);
                    }
                    grid->Add(labelBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
                }

                // Create control
                wxWindow* ctrl = nullptr;
                wxString path = field.section + "/" + field.key;

                switch (field.type)
                {
                case Field::Bool:
                {
                    bool v = field.defaultInt != 0;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);

                    auto* cb = new wxCheckBox(sectionSizer->GetStaticBox(), wxID_ANY, "");
                    cb->SetValue(v);

                    // Prevent it from accepting inputs from the WHOLE grid cell
                    cb->SetMinSize(cb->GetBestSize());
                    cb->SetSizeHints(cb->GetBestSize());

                    ctrl = cb;

                    // If this checkbox is a prerequisite, hook it
                    cb->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&)
                             {
                                 ApplyPrerequisites();
                             });
                    break;
                }
                case Field::Int:
                {
                    int v = field.defaultInt;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);

                    if (int clamped = std::clamp(v, field.minInt, field.maxInt); clamped != v)
                    {
                        wxLogWarning("Out-of-range value %d for [%s/%s], clamped to %d",
                                     v, field.section, field.key, clamped);
                        v = clamped;
                        m_missingKeys = true;
                    }

                    auto* sp = new wxSpinCtrl(sectionSizer->GetStaticBox(), wxID_ANY,
                                              std::to_string(v),
                                              wxDefaultPosition, wxDefaultSize,
                                              wxSP_ARROW_KEYS,
                                              field.minInt, field.maxInt, v);

                    ctrl = sp;
                    ctrl->Bind(wxEVT_ANY, &ConfigFrame::MarkDirty, this);
                    break;
                }
                case Field::Str:
                {
                    wxString v = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);
                    ctrl = new wxTextCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v);
                    break;
                }
                case Field::Choice:
                {
                    const bool isRegionField =
                        (field.section == ConfigKeys::Region_Section) &&
                        (field.key == ConfigKeys::Region_Setting);

                    const bool isLanguageField =
                        (field.section == ConfigKeys::Language_Section) &&
                        (field.key == ConfigKeys::Language_Setting);

                    if (isRegionField)
                    {
                        auto* ch = new wxChoice(sectionSizer->GetStaticBox(), wxID_ANY);
                        ctrl = ch;
                        m_regionChoice = ch;

                        ctrl->Bind(wxEVT_ANY, &ConfigFrame::MarkDirty, this);

                        ch->Bind(wxEVT_CHOICE, [this](wxCommandEvent&)
                                 {
                                     if (!m_regionChoice || !m_languageChoice)
                                     {
                                         return;
                                     }

                                     const wxString regionName = m_regionChoice->GetStringSelection();
                                     const wxString currentLanguage = m_languageChoice->GetStringSelection(); //prevents langauge selection from defaulting back to the first entry if user reselects the same region.

                                     PopulateLanguageChoices(regionName, currentLanguage);

                                     m_dirty = true;
                                 });

                        break;
                    }

                    if (isLanguageField)
                    {
                        auto* ch = new wxChoice(sectionSizer->GetStaticBox(), wxID_ANY);
                        ctrl = ch;
                        m_languageChoice = ch;

                        ctrl->Bind(wxEVT_ANY, &ConfigFrame::MarkDirty, this);
                        break;
                    }

                    // Normal Choice behavior (static options)
                    wxString v = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);

                    auto* ch = new wxChoice(sectionSizer->GetStaticBox(), wxID_ANY);
                    ch->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e)
                             {
                                 m_dirty = true;
                                 ApplyPrerequisites();
                                 e.Skip();
                             });

                    for (auto& c : field.choices)
                        ch->Append(c);

                    if (int idx = ch->FindString(v); idx != wxNOT_FOUND)
                    {
                        ch->SetSelection(idx);
                    }
                    else
                    {
                        wxLogWarning("Invalid value '%s' for [%s/%s], resetting to default '%s'",
                                     v, field.section, field.key, field.defaultString);

                        if (int defIdx = ch->FindString(field.defaultString); !field.defaultString.IsEmpty() && defIdx != wxNOT_FOUND)
                        {
                            ch->SetSelection(defIdx);
                        }
                        else if (!field.choices.empty())
                        {
                            ch->SetSelection(0);
                        }
                        m_dirty = true;
                    }

                    ctrl = ch;
                    ctrl->Bind(wxEVT_ANY, &ConfigFrame::MarkDirty, this);
                    break;
                }

                case Field::Hotkey:
                {
                    wxString v = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);
                    ctrl = new HotkeyCaptureCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v);
                    break;
                }
                case Field::StickHotkey:
                {
                    wxString v = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);
                    ctrl = new HotkeyCaptureCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v, true);
                    break;
                }
                case Field::Spacer:
                {
                    auto* spacer = new wxPanel(sectionSizer->GetStaticBox(), wxID_ANY);
                    spacer->SetMinSize(FromDIP(wxSize(0, 10)));
                    if (fieldVisible)
                    {
                        grid->Add(spacer, 0, wxEXPAND);
                    }
                    else
                    {
                        spacer->Hide();
                    }
                    continue;
                }
                case Field::Float:
                {
                    double v = field.defaultFloat;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &v);

                    if (double clamped = std::clamp(v, field.minFloat, field.maxFloat); clamped != v)
                    {
                        wxLogWarning("Out-of-range float %f for [%s/%s], clamped to %f",
                                     v, field.section, field.key, clamped);
                        v = clamped;
                        m_missingKeys = true;
                    }

                    auto* sp = new wxSpinCtrlDouble(
                        sectionSizer->GetStaticBox(), wxID_ANY,
                        wxString::Format("%.2f", v),
                        wxDefaultPosition, wxDefaultSize,
                        wxSP_ARROW_KEYS,
                        field.minFloat, field.maxFloat, v, 0.01 // increment step
                    );

                    ctrl = sp;
                    sp->SetMinSize(FromDIP(wxSize(90, -1)));
                    sp->SetSizeHints(FromDIP(wxSize(90, -1)));
                    ctrl->Bind(wxEVT_ANY, &ConfigFrame::MarkDirty, this);
                    break;
                }
                }

                if (ctrl)
                {
                    if (auto* cb = wxDynamicCast(ctrl, wxCheckBox))
                        cb->Bind(wxEVT_CHECKBOX, &ConfigFrame::MarkDirty, this);
                    else if (auto* sp = wxDynamicCast(ctrl, wxSpinCtrl))
                        sp->Bind(wxEVT_SPINCTRL, &ConfigFrame::MarkDirty, this);
                    else if (auto* tc = wxDynamicCast(ctrl, wxTextCtrl))
                        tc->Bind(wxEVT_TEXT, &ConfigFrame::MarkDirty, this);
                    else if (auto* ch = wxDynamicCast(ctrl, wxChoice))
                        ch->Bind(wxEVT_CHOICE, &ConfigFrame::MarkDirty, this);

                    // Always show a tooltip (except for spacers)
                    if (field.type != Field::Spacer)
                    {
                        wxString tip;

                        // Use provided tooltip if any
                        if (!field.tooltip.IsEmpty())
                        {
                            tip = field.tooltip;
                        }

                        const bool isRegionField =
                            (field.section == ConfigKeys::Region_Section) &&
                            (field.key == ConfigKeys::Region_Setting);

                        const bool isLanguageField =
                            (field.section == ConfigKeys::Language_Section) &&
                            (field.key == ConfigKeys::Language_Setting);

                        const bool isDynamicRegionLanguage = (isRegionField || isLanguageField);

                        // For Region/Language, do NOT show the DEFAULT VALUE footer
                        if (!isDynamicRegionLanguage)
                        {
                            // Always append default value
                            if (!tip.IsEmpty())
                            {
                                tip += "\n\n";
                            }

                            tip += "DEFAULT VALUE: ";

                            switch (field.type)
                            {
                            case Field::Bool:
                                tip += (field.defaultInt != 0) ? "Enabled" : "Disabled";
                                break;

                            case Field::Int:
                                tip += wxString::Format("%d", field.defaultInt);
                                break;

                            case Field::Float:
                                tip += wxString::Format("%f", field.defaultFloat);
                                break;

                            case Field::Str:
                            case Field::Hotkey:
                            case Field::StickHotkey:
                                if (!field.defaultString.IsEmpty())
                                    tip += "\"" + field.defaultString + "\"";
                                else
                                    tip += "\"\"";
                                break;

                            case Field::Choice:
                                if (!field.defaultString.IsEmpty())
                                    tip += "\"" + field.defaultString + "\"";
                                else if (!field.choices.empty())
                                    tip += "\"" + field.choices[0] + "\""; // fallback to first option
                                else
                                    tip += "(none)";
                                break;

                            default:
                                break;
                            }
                        }

                        // Only apply tooltip if we actually have something to show
                        if (!tip.IsEmpty())
                        {
                            ctrl->SetToolTip(tip);
                        }
                        else
                        {
                            ctrl->UnsetToolTip();
                        }
                    }


                    int flags = wxALIGN_CENTER_VERTICAL;
                    if (bFullLengthFields)
                    {
                        flags |= wxEXPAND;
                    }

                    if (fieldVisible)
                    {
                        grid->Add(ctrl, 0, flags);
                    }
                    else
                    {
                        ctrl->Hide();
                    }

                    m_controls[{field.section, field.key}] = ctrl;
                }
            }

            panel->SetSizer(vbox);

            const bool tabVisible = std::any_of(tab.second.begin(), tab.second.end(), [&](const Field& field)
            {
                return (field.gameFlags & targetGameFlag) != 0;
            });

            if (tabVisible)
            {
                m_tabs->AddPage(panel, tab.first, false);
            }
            else
            {
                panel->Hide();
            }
        }

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        BannerPanel* banner = new BannerPanel(this, bannerID);
        banner->SetCursor(wxCursor(wxCURSOR_HAND));
        banner->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&)
                     {
                         wxLaunchDefaultBrowser(iTargetGame == TARGET_GAME_MG1 ? NEXUS_MG1_URL :
                                                iTargetGame == TARGET_GAME_MGS2 ? NEXUS_MGS2_URL :
                                                iTargetGame == TARGET_GAME_MGS3 ? NEXUS_MGS3_URL :
                                                PRIMARY_REPO_URL);
                     });
        mainSizer->Add(banner, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 0);

        mainSizer->Add(m_tabs, 1, wxEXPAND | wxALL, 5);

        // Bugfix Compilation status
        if (iTargetGame == TARGET_GAME_MGS2 || iTargetGame == TARGET_GAME_MGS3)
        {
            const std::filesystem::path exePath = Helper::FindASILocation(sFixName).parent_path();

            const char* asiName = (iTargetGame == TARGET_GAME_MGS2)
                ? "MGS2-Community-Bugfix-Compilation.asi"
                : "MGS3-Community-Bugfix-Compilation.asi";

            const char* compilationName = (iTargetGame == TARGET_GAME_MGS2)
                ? "MGS2 Community Bugfix Compilation"
                : "MGS3 Community Bugfix Compilation";

            const bool bugfixInstalled = std::filesystem::exists(
                exePath / "plugins" / asiName);

            auto* statusPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
            statusPanel->SetBackgroundColour(m_tabs->GetPage(0)->GetBackgroundColour());

            auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);

            auto* icon = new wxStaticText(statusPanel, wxID_ANY,
                                          bugfixInstalled ? L"\u2714" : L"\u2718");
            icon->SetForegroundColour(bugfixInstalled ? wxColour(0, 128, 0) : wxColour(192, 0, 0));
            wxFont iconFont = icon->GetFont();
            iconFont.SetPointSize(iconFont.GetPointSize() + 2);
            icon->SetFont(iconFont);

            auto* label = new wxStaticText(statusPanel, wxID_ANY,
                                           wxString::Format("%s: ", compilationName));
            label->SetForegroundColour(bugfixInstalled ? wxColour(0, 128, 0) : wxColour(192, 0, 0));

            auto* statusLabel = new wxStaticText(statusPanel, wxID_ANY,
                                                 bugfixInstalled ? "Installed" : "Not Installed");
            statusLabel->SetForegroundColour(bugfixInstalled ? wxColour(0, 128, 0) : wxColour(192, 0, 0));
            statusLabel->SetFont(statusLabel->GetFont().Bold());

            statusSizer->AddStretchSpacer();
            statusSizer->Add(icon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
            statusSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL);
            statusSizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

            if (!bugfixInstalled)
            {
                const char* nexusUrl = (iTargetGame == TARGET_GAME_MGS2)
                    ? "https://www.nexusmods.com/metalgearsolid2mc/mods/52"
                    : "https://www.nexusmods.com/metalgearsolid3mc/mods/189";

                const char* githubUrl = (iTargetGame == TARGET_GAME_MGS2)
                    ? "https://github.com/ShizCalev/MGS2-Community-Bugfix-Compilation"
                    : "https://github.com/ShizCalev/MGS3-Community-Bugfix-Compilation";

                auto* nexusBtn = new wxButton(statusPanel, wxID_ANY, "Nexus Page");
                nexusBtn->Bind(wxEVT_BUTTON, [nexusUrl](wxCommandEvent&) {
                    wxLaunchDefaultBrowser(nexusUrl);
                               });
                statusSizer->Add(nexusBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

                auto* githubBtn = new wxButton(statusPanel, wxID_ANY, "GitHub Page");
                githubBtn->Bind(wxEVT_BUTTON, [githubUrl](wxCommandEvent&) {
                    wxLaunchDefaultBrowser(githubUrl);
                                });
                statusSizer->Add(githubBtn, 0, wxALIGN_CENTER_VERTICAL);

                const char* tooltip = (iTargetGame == TARGET_GAME_MGS2)
                    ? "This mod fixes nearly 14,000 texture issues, hundreds of transparent textures/models, missing audio/music, and countless localization errors/typos introduced by the 2011 Bluepoint HD remaster."
                    : "This mod fixes nearly 4000 texture issues, over 500 transparent textures/models, restores missing regional content, and corrects countless localization errors/typos introduced by the 2011 Bluepoint HD remaster.";

                //statusPanel->SetToolTip(tooltip);
                icon->SetToolTip(tooltip);
                label->SetToolTip(tooltip);
                statusLabel->SetToolTip(tooltip);
                nexusBtn->SetToolTip(tooltip);
                githubBtn->SetToolTip(tooltip);
            }

            statusSizer->AddStretchSpacer();

            statusPanel->SetSizer(statusSizer);
            mainSizer->Add(statusPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
        }
        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* resetBtn = new wxButton(this, wxID_ANY, "Reset to Defaults");
        btnSizer->Add(resetBtn, 0, wxRIGHT, 5);

        btnSizer->AddStretchSpacer();

        auto* LaunchBtn = new wxButton(this, wxID_ANY, "Launch Game");
        auto* saveBtn = new wxButton(this, wxID_SAVE, "Save and Exit");
        auto* exitBtn = new wxButton(this, wxID_EXIT, "Exit");

        btnSizer->Add(LaunchBtn, 0, wxRIGHT, 5);
        btnSizer->Add(saveBtn, 0, wxRIGHT, 5);
        btnSizer->Add(exitBtn, 0);

        mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

        SetSizer(mainSizer);
        Bind(wxEVT_CLOSE_WINDOW, &ConfigFrame::OnClose, this);
        Centre();

        Bind(wxEVT_BUTTON, &ConfigFrame::OnSave, this, wxID_SAVE);
        Bind(wxEVT_BUTTON, [&](wxCommandEvent&)
             {
                 Close();
             }, wxID_EXIT);
        LaunchBtn->Bind(wxEVT_BUTTON, &ConfigFrame::OnSaveAndLaunch, this);
        resetBtn->Bind(wxEVT_BUTTON, &ConfigFrame::OnResetDefaults, this);

        if (m_regionChoice && m_languageChoice)
        {
            InitRegionLanguageFromConfig(); // defaults to FIRST entry in pairing if missing/invalid
            FitChoiceToWidestItem(m_regionChoice);
            FitChoiceToWidestItem(m_languageChoice);
            RelayoutAfterDynamicChoiceChange();
        }

        ApplyPrerequisites();
        HandleUpdateCheckPreference();

        SnapshotCurrentValues();
    }

    ~ConfigFrame() override
    {
        if (m_iconSmall)
        {
            DestroyIcon(m_iconSmall);
            m_iconSmall = nullptr;
        }

        if (m_iconBig)
        {
            DestroyIcon(m_iconBig);
            m_iconBig = nullptr;
        }
    }

    void HandleUpdateCheckPreference()
    {
        const wxString section = ConfigKeys::CheckForUpdates_Section;
        const wxString key = ConfigKeys::CheckForUpdates_Setting;
        const wxString path = section + "/" + key;

        bool hasValue = m_conf->HasEntry(path);
        long v = 0;
        if (hasValue)
        {
            m_conf->Read(path, &v);
        }

        const bool shouldPrompt = m_firstRun || !hasValue;
        if (shouldPrompt)
        {
            wxMessageDialog dlg(
                this,
                "Do you want to enable automatic update checks?\n\n"
                "You can change this later in the settings.",
                "MGSHDFix - Universal Config Tool",
                wxYES_NO | wxICON_QUESTION
            );
            dlg.SetYesNoLabels("Enable", "Disable");

            const bool enable = (dlg.ShowModal() == wxID_YES);

            m_conf->Write(path, enable ? "1" : "0");

            if (auto it = m_controls.find({ section, key }); it != m_controls.end())
            {
                if (auto* cb = wxDynamicCast(it->second, wxCheckBox))
                {
                    cb->SetValue(enable);
                }
            }

            if (!enable || !hasValue)
            {
                m_dirty = true;
            }

            v = enable ? 1 : 0;
            hasValue = true;
            ApplyPrerequisites();
        }

        wxLogDebug("Update check setting: %s (value=%ld)", hasValue ? "exists" : "missing", v);

        if (hasValue && v == 0)
        {
            return;
        }

        if (v != 0)
        {
            CheckForUpdates();
        }
    }

private:
    using Key = std::pair<wxString, wxString>;

    struct KeyHash
    {
        size_t operator()(const Key& k) const
        {
            return std::hash<std::string>()((k.first + k.second).ToStdString());
        }
    };

    wxStaticText* m_bugfixStatus = nullptr;
    HICON m_iconSmall = nullptr;
    HICON m_iconBig = nullptr;
    bool m_dirty = false;
    bool m_firstRun = false;
    bool m_missingKeys = false;
    std::vector<Key> m_missingKeyList;

    wxNotebook* m_tabs = nullptr;
    wxTextCtrl* m_focusSink = nullptr;

    wxChoice* m_regionChoice = nullptr;
    wxChoice* m_languageChoice = nullptr;

    void MarkDirty(wxEvent& e)
    {
        m_dirty = true;
        e.Skip();
    }

    void StopAllHotkeyCaptures()
    {
        for (const auto& kv : m_controls)
        {
            if (auto* hotkey = dynamic_cast<HotkeyCaptureCtrl*>(kv.second))
            {
                hotkey->StopGamepadCapture();
            }
        }
    }

    void ClearHotkeyCaptureFocus()
    {
        for (const auto& kv : m_controls)
        {
            if (auto* hotkey = dynamic_cast<HotkeyCaptureCtrl*>(kv.second))
            {
                hotkey->CancelGamepadCaptureFocus();
            }
        }

        if (m_focusSink != nullptr)
        {
            m_focusSink->SetFocus();
        }
    }

    void MarkMissingKey(const wxString& section, const wxString& key)
    {
        m_missingKeys = true;

        const Key missingKey{ section, key };
        if (std::find(m_missingKeyList.begin(), m_missingKeyList.end(), missingKey) == m_missingKeyList.end())
        {
            m_missingKeyList.push_back(missingKey);
        }
    }

    bool IsMissingKey(const wxString& section, const wxString& key) const
    {
        const Key missingKey{ section, key };
        return std::find(m_missingKeyList.begin(), m_missingKeyList.end(), missingKey) != m_missingKeyList.end();
    }

    static wxString GetControlDisplayValue(wxWindow* ctrl)
    {
        if (!ctrl)
            return wxString();

        if (auto* cb = wxDynamicCast(ctrl, wxCheckBox))
            return cb->GetValue() ? "Enabled" : "Disabled";
        if (auto* sp = wxDynamicCast(ctrl, wxSpinCtrl))
            return wxString::Format("%d", sp->GetValue());
        if (auto* spd = wxDynamicCast(ctrl, wxSpinCtrlDouble))
            return wxString::Format("%g", spd->GetValue());
        if (auto* tc = wxDynamicCast(ctrl, wxTextCtrl))
            return "\"" + tc->GetValue() + "\"";
        if (auto* ch = wxDynamicCast(ctrl, wxChoice))
            return "\"" + ch->GetStringSelection() + "\"";

        return wxString();
    }

    void SnapshotCurrentValues()
    {
        m_snapshot.clear();
        for (const auto& kv : m_controls)
            m_snapshot[kv.first] = GetControlDisplayValue(kv.second);
    }

    wxString BuildChangeList() const
    {
        wxString out;
        for (const auto& tab : kTabs)
        {
            // wxWidgets freaks the fuck out with &'s, lets normalize them
            wxString tabTitle = tab.first;
            tabTitle.Replace("&&", "\x01");      
            tabTitle.Replace("&", "");           
            tabTitle.Replace("\x01", "&");       
            wxString tabChunk;
            wxString currentSection;
            wxString sectionChunk;

            auto flushSection = [&]()
                {
                    if (!sectionChunk.IsEmpty())
                    {
                        if (!currentSection.IsEmpty())
                            tabChunk += "  [" + currentSection + "]\n";
                        tabChunk += sectionChunk;
                        sectionChunk.Clear();
                    }
                };

            for (const auto& field : tab.second)
            {
                auto it = m_controls.find({ field.section, field.key });
                if (it == m_controls.end())
                    continue;

                wxString now = GetControlDisplayValue(it->second);
                auto snapIt = m_snapshot.find({ field.section, field.key });
                wxString before = (snapIt != m_snapshot.end()) ? snapIt->second : wxString();

                if (now == before)
                    continue;

                if (field.section != currentSection)
                {
                    flushSection();
                    currentSection = field.section;
                }
                sectionChunk += wxString::Format("    %s: %s -> %s\n",
                                                 field.key.c_str(),
                                                 before.c_str(),
                                                 now.c_str());
            }
            flushSection();

            if (!tabChunk.IsEmpty())
            {
                if (!out.IsEmpty())
                    out += "\n";
                out += tabTitle + "\n" + tabChunk;
            }
        }
        return out;
    }

    wxString BuildMissingKeyList() const
    {
        wxString out;

        for (const auto& tab : kTabs)
        {
            wxString tabTitle = tab.first;
            tabTitle.Replace("&&", "\x01");
            tabTitle.Replace("&", "");
            tabTitle.Replace("\x01", "&");

            wxString tabChunk;
            wxString currentSection;
            wxString sectionChunk;

            auto flushSection = [&]()
                {
                    if (!sectionChunk.IsEmpty())
                    {
                        if (!currentSection.IsEmpty())
                            tabChunk += "  [" + currentSection + "]\n";
                        tabChunk += sectionChunk;
                        sectionChunk.Clear();
                    }
                };

            for (const auto& field : tab.second)
            {
                if (!IsMissingKey(field.section, field.key))
                    continue;

                auto it = m_controls.find({ field.section, field.key });
                if (it == m_controls.end())
                    continue;

                if (field.section != currentSection)
                {
                    flushSection();
                    currentSection = field.section;
                }

                sectionChunk += wxString::Format("    %s: <missing> -> %s\n",
                                                 field.key.c_str(),
                                                 GetControlDisplayValue(it->second).c_str());
            }
            flushSection();

            if (!tabChunk.IsEmpty())
            {
                if (!out.IsEmpty())
                    out += "\n";
                out += tabTitle + "\n" + tabChunk;
            }
        }

        return out;
    }

    wxString BuildUnsavedChangeList() const
    {
        wxString changeList = BuildChangeList();
        const wxString missingList = BuildMissingKeyList();

        if (!missingList.IsEmpty())
        {
            if (!changeList.IsEmpty())
                changeList += "\n\n";

            changeList += "Missing settings:\n";
            changeList += missingList;
        }

        return changeList;
    }


    int ShowUnsavedChangesDialog(const wxString& prompt,
                                 const wxString& yesLabel,
                                 const wxString& noLabel,
                                 const wxString& changeList)
    {
        // No diff to show (first run, missing keys, or odd state) -> simple dialog.
        if (changeList.IsEmpty())
        {
            wxMessageDialog dlg(this, prompt, "Unsaved Changes",
                                wxYES_NO | wxCANCEL | wxICON_WARNING);
            dlg.SetYesNoCancelLabels(yesLabel, noLabel, "Cancel");
            return dlg.ShowModal();
        }

        wxDialog dlg(this, wxID_ANY, "Unsaved Changes",
                     wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        dlg.SetClientSize(dlg.FromDIP(wxSize(560, 420)));

        auto* sizer = new wxBoxSizer(wxVERTICAL);

        auto* msg = new wxStaticText(&dlg, wxID_ANY, prompt);
        sizer->Add(msg, 0, wxALL, 12);

        auto* listLabel = new wxStaticText(&dlg, wxID_ANY, "Changes since last save:");
        sizer->Add(listLabel, 0, wxLEFT | wxRIGHT, 12);

        auto* list = new wxTextCtrl(&dlg, wxID_ANY, changeList,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);

        wxFont mono(wxFontInfo().Family(wxFONTFAMILY_TELETYPE));
        if (mono.IsOk())
            list->SetFont(mono);
        sizer->Add(list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

        auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* yesBtn = new wxButton(&dlg, wxID_YES, yesLabel);
        auto* noBtn = new wxButton(&dlg, wxID_NO, noLabel);
        auto* cancelBtn = new wxButton(&dlg, wxID_CANCEL, "Cancel");
        btnSizer->AddStretchSpacer();
        btnSizer->Add(yesBtn, 0, wxRIGHT, 5);
        btnSizer->Add(noBtn, 0, wxRIGHT, 5);
        btnSizer->Add(cancelBtn, 0);
        sizer->Add(btnSizer, 0, wxEXPAND | wxALL, 12);

        dlg.SetSizer(sizer);
        dlg.SetAffirmativeId(wxID_YES);
        dlg.SetEscapeId(wxID_CANCEL);

        auto routeBtn = [&dlg](wxCommandEvent& e) { dlg.EndModal(e.GetId()); };
        yesBtn->Bind(wxEVT_BUTTON, routeBtn);
        noBtn->Bind(wxEVT_BUTTON, routeBtn);
        cancelBtn->Bind(wxEVT_BUTTON, routeBtn);

        return dlg.ShowModal();
    }

    // ----------------------------
    // Region/Language dynamic lists
    // ----------------------------
    static std::span<const Game_Language_Pair_View> GetActiveLanguagePairs()
    {
#if defined(MGSHDFIX_SPECIFIC)
        if (iTargetGame == TARGET_GAME_MGS3)
        {
            return std::span<const Game_Language_Pair_View>(MGS3_LanguagePairs.data(), MGS3_LanguagePairs.size());
        }

        return std::span<const Game_Language_Pair_View>(MG1_MG2_MGS2_LanguagePairs.data(), MG1_MG2_MGS2_LanguagePairs.size());
#endif
    }

    static bool IsValidRegionLanguagePair(std::span<const Game_Language_Pair_View> pairs, std::string_view region, std::string_view language)
    {
        for (const auto& p : pairs)
        {
            if (p.Game_Region == region && p.Game_Language == language)
            {
                return true;
            }
        }
        return false;
    }

    static bool ResolveRegionLanguageNames(std::span<const Game_Language_Pair_View> pairs,
                                           std::string_view game_region,
                                           std::string_view game_language,
                                           std::string& out_region_name,
                                           std::string& out_language_name)
    {
        for (const auto& p : pairs)
        {
            if (p.Game_Region != game_region)
            {
                continue;
            }

            if (p.Game_Language != game_language)
            {
                continue;
            }

            out_region_name.assign(p.Region_Name);
            out_language_name.assign(p.Language_Name);
            return true;
        }

        return false;
    }

    static bool ResolveRegionLanguageCodes(std::span<const Game_Language_Pair_View> pairs,
                                           std::string_view region_name,
                                           std::string_view language_name,
                                           std::string& out_game_region,
                                           std::string& out_game_language)
    {
        for (const auto& p : pairs)
        {
            if (p.Region_Name != region_name)
            {
                continue;
            }

            if (p.Language_Name != language_name)
            {
                continue;
            }

            out_game_region.assign(p.Game_Region);
            out_game_language.assign(p.Game_Language);
            return true;
        }

        return false;
    }

    void FitChoiceToWidestItem(wxChoice* choice)
    {
        if (!choice)
        {
            return;
        }

        wxClientDC dc(choice);
        dc.SetFont(choice->GetFont());

        int maxW = 0;
        int maxH = 0;

        for (unsigned int i = 0; i < choice->GetCount(); ++i)
        {
            wxCoord w = 0;
            wxCoord h = 0;
            dc.GetTextExtent(choice->GetString(i), &w, &h);

            if ((int)w > maxW) maxW = (int)w;
            if ((int)h > maxH) maxH = (int)h;
        }

        const int extraW = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, choice);
        const int padW = FromDIP(32);
        const int padH = FromDIP(10);

        wxSize best = choice->GetBestSize();
        const int wantedW = std::max(best.GetWidth(), maxW + extraW + padW);
        const int wantedH = std::max(best.GetHeight(), maxH + padH);

        choice->SetMinSize(wxSize(wantedW, wantedH));
        choice->SetSizeHints(wxSize(wantedW, wantedH));
    }

    void RelayoutAfterDynamicChoiceChange()
    {
        Layout();
        if (m_tabs)
        {
            m_tabs->Layout();
        }
        Refresh();
    }

    void PopulateRegionChoices()
    {
        if (!m_regionChoice)
        {
            return;
        }

        m_regionChoice->Clear();

        const auto pairs = GetActiveLanguagePairs();

        std::vector<std::string_view> regions;
        regions.reserve(8);

        for (const auto& p : pairs)
        {
            bool exists = false;
            for (const auto& r : regions)
            {
                if (r == p.Region_Name)
                {
                    exists = true;
                    break;
                }
            }

            if (!exists)
            {
                regions.push_back(p.Region_Name);
                m_regionChoice->Append(wxString(p.Region_Name));
            }
        }

        // Default: first entry
        if (m_regionChoice->GetCount() > 0)
        {
            m_regionChoice->SetSelection(0);
        }

        FitChoiceToWidestItem(m_regionChoice);
        RelayoutAfterDynamicChoiceChange();
    }

    void PopulateLanguageChoices(const wxString& regionName, const wxString& preferredLanguageName = wxString())
    {
        if (!m_languageChoice)
        {
            return;
        }

        const auto pairs = GetActiveLanguagePairs();

        m_languageChoice->Clear();

        const std::string regionStd = regionName.ToStdString();

        for (const auto& p : pairs)
        {
            if (p.Region_Name != regionStd)
            {
                continue;
            }

            m_languageChoice->Append(wxString(p.Language_Name));
        }

        if (!preferredLanguageName.IsEmpty())
        {
            const int idx = m_languageChoice->FindString(preferredLanguageName);
            if (idx != wxNOT_FOUND)
            {
                m_languageChoice->SetSelection(idx);

                FitChoiceToWidestItem(m_languageChoice);
                RelayoutAfterDynamicChoiceChange();
                return;
            }
        }

        // Default: first entry for this region
        if (m_languageChoice->GetCount() > 0)
        {
            m_languageChoice->SetSelection(0);
        }

        FitChoiceToWidestItem(m_languageChoice);
        RelayoutAfterDynamicChoiceChange();
    }

    void InitRegionLanguageFromConfig()
    {
        if (!m_regionChoice || !m_languageChoice)
        {
            return;
        }

        const wxString sectionRegion = ConfigKeys::Region_Section;
        const wxString keyRegion = ConfigKeys::Region_Setting;
        const wxString pathRegion = sectionRegion + "/" + keyRegion;

        const wxString sectionLang = ConfigKeys::Language_Section;
        const wxString keyLang = ConfigKeys::Language_Setting;
        const wxString pathLang = sectionLang + "/" + keyLang;

        const auto pairs = GetActiveLanguagePairs();

        // Default is ALWAYS the first entry of the pairing
        std::string defaultRegionCode = "eu";
        std::string defaultLangCode = "en";
        if (!pairs.empty())
        {
            defaultRegionCode.assign(pairs[0].Game_Region);
            defaultLangCode.assign(pairs[0].Game_Language);
        }

        wxString regionCode(defaultRegionCode);
        wxString langCode(defaultLangCode);

        bool hasRegion = m_conf->HasEntry(pathRegion);
        bool hasLang = m_conf->HasEntry(pathLang);

        if (!hasRegion)
        {
            MarkMissingKey(sectionRegion, keyRegion);
        }

        if (!hasLang)
        {
            MarkMissingKey(sectionLang, keyLang);
        }

        if (hasRegion)
        {
            m_conf->Read(pathRegion, &regionCode);
            regionCode = Unquote(regionCode);
        }

        if (hasLang)
        {
            m_conf->Read(pathLang, &langCode);
            langCode = Unquote(langCode);
        }

        std::string regionCodeStd = regionCode.ToStdString();
        std::string langCodeStd = langCode.ToStdString();

        // If invalid, force to first entry in pairing
        if (!IsValidRegionLanguagePair(pairs, regionCodeStd, langCodeStd))
        {
            regionCodeStd = defaultRegionCode;
            langCodeStd = defaultLangCode;
            MarkMissingKey(sectionRegion, keyRegion);
            MarkMissingKey(sectionLang, keyLang);
        }

        std::string regionName;
        std::string langName;

        if (!ResolveRegionLanguageNames(pairs, regionCodeStd, langCodeStd, regionName, langName))
        {
            // Hard fallback to first entry names
            if (!pairs.empty())
            {
                regionName.assign(pairs[0].Region_Name);
                langName.assign(pairs[0].Language_Name);
            }
            MarkMissingKey(sectionRegion, keyRegion);
            MarkMissingKey(sectionLang, keyLang);
        }

        // Fill region list (select correct region name if present, else first)
        m_regionChoice->Clear();
        {
            std::vector<std::string_view> regions;
            regions.reserve(8);

            for (const auto& p : pairs)
            {
                bool exists = false;
                for (const auto& r : regions)
                {
                    if (r == p.Region_Name)
                    {
                        exists = true;
                        break;
                    }
                }

                if (!exists)
                {
                    regions.push_back(p.Region_Name);
                    m_regionChoice->Append(wxString(p.Region_Name));
                }
            }
        }

        int regionIdx = m_regionChoice->FindString(wxString(regionName));
        if (regionIdx == wxNOT_FOUND)
        {
            regionIdx = 0; // default: first region entry
        }
        if (m_regionChoice->GetCount() > 0)
        {
            m_regionChoice->SetSelection(regionIdx);
        }

        // Fill language list based on selected region, prefer resolved language name, else first
        PopulateLanguageChoices(m_regionChoice->GetStringSelection(), wxString(langName));
    }

    int FindFocusTab()
    {
        if (m_tabs)
            return m_tabs->GetSelection();
        return -1;
    }

    void ResetTabToDefaults(int tabIndex)
    {
        if (tabIndex < 0 || tabIndex >= (int)kTabs.size())
            return;

        auto& fields = kTabs[tabIndex].second;

        for (auto& field : fields)
        {
            auto it = m_controls.find({ field.section, field.key });
            if (it == m_controls.end())
                continue;

            wxWindow* ctrl = it->second;
            switch (field.type)
            {
            case Field::Bool:
                if (auto* c = wxDynamicCast(ctrl, wxCheckBox))
                {
                    c->SetValue(field.defaultInt != 0);
                    wxCommandEvent ev(wxEVT_CHECKBOX, c->GetId());
                    ev.SetEventObject(c);
                    wxPostEvent(c, ev);
                }
                break;

            case Field::Int:
                if (auto* c = wxDynamicCast(ctrl, wxSpinCtrl))
                {
                    c->SetValue(field.defaultInt);
                    wxCommandEvent ev(wxEVT_SPINCTRL, c->GetId());
                    ev.SetEventObject(c);
                    wxPostEvent(c, ev);
                }
                break;

            case Field::Float:
                if (auto* c = wxDynamicCast(ctrl, wxSpinCtrlDouble))
                {
                    c->SetValue(field.defaultFloat);
                    wxCommandEvent ev(wxEVT_SPINCTRLDOUBLE, c->GetId());
                    ev.SetEventObject(c);
                    wxPostEvent(c, ev);
                }
                break;

            case Field::Str:
            case Field::Hotkey:
            case Field::StickHotkey:
                if (auto* c = wxDynamicCast(ctrl, wxTextCtrl))
                {
                    c->SetValue(field.defaultString);
                    wxCommandEvent ev(wxEVT_TEXT, c->GetId());
                    ev.SetEventObject(c);
                    wxPostEvent(c, ev);
                }
                break;

            case Field::Choice:
                if (auto* c = wxDynamicCast(ctrl, wxChoice))
                {
                    // For dynamic Region/Language, reset to first entry in pairing
                    const bool isRegionField =
                        (field.section == ConfigKeys::Region_Section) &&
                        (field.key == ConfigKeys::Region_Setting);

                    const bool isLanguageField =
                        (field.section == ConfigKeys::Language_Section) &&
                        (field.key == ConfigKeys::Language_Setting);

                    if (isRegionField || isLanguageField)
                    {
                        const auto pairs = GetActiveLanguagePairs();
                        if (!pairs.empty())
                        {
                            // Force UI to first entry in pairing
                            InitRegionLanguageFromConfig();
                            m_dirty = true;
                        }
                        break;
                    }

                    int idx = c->FindString(field.defaultString);
                    if (idx != wxNOT_FOUND)
                        c->SetSelection(idx);
                    else if (!field.choices.empty())
                        c->SetSelection(0);

                    wxCommandEvent ev(wxEVT_CHOICE, c->GetId());
                    ev.SetEventObject(c);
                    wxPostEvent(c, ev);
                }
                break;

            default:
                break;
            }
        }

        ApplyPrerequisites(); // update dependent fields
    }

    void OnResetDefaults(wxCommandEvent&)
    {
        wxMessageDialog dlg(
            this,
            "Do you want to reset just this tab, or all tabs, to their default values?",
            "Reset to Defaults",
            wxYES_NO | wxCANCEL | wxICON_WARNING
        );
        dlg.SetYesNoCancelLabels("Reset Tab", "Reset All", "Cancel");

        int choice = dlg.ShowModal();

        if (choice == wxID_YES) // Reset Tab
        {
            int sel = FindFocusTab();
            if (sel >= 0)
                ResetTabToDefaults(sel);
        }
        else if (choice == wxID_NO) // Reset All
        {
            for (size_t i = 0; i < kTabs.size(); ++i)
                ResetTabToDefaults((int)i);
        }
        else
        {
            return; // Cancel
        }

        m_dirty = true;
    }

    void OnSaveAndLaunch(wxCommandEvent& event)
    {
        wxString changeList = BuildUnsavedChangeList();
        const bool hasRealChanges = !changeList.IsEmpty();

        if (hasRealChanges || m_firstRun || m_missingKeys)
        {
            wxString message;
            if (m_firstRun)
            {
                message =
                    "This appears to be your first time running the config tool.\n\n"
                    "You must save your settings before starting the game.";
                changeList.Clear(); // first-run baseline isn't meaningful to diff
            }
            else if (m_missingKeys)
            {
                message =
                    "Some settings were missing from your config file and will be restored.\n\n"
                    "Review the changes below and save before starting the game.";
            }
            else
            {
                message =
                    "You have unsaved changes.\n\n"
                    "Do you want to save them before launching the game?";
            }

            int choice = ShowUnsavedChangesDialog(message, "Save", "Discard", changeList);
            if (choice == wxID_YES)
            {
                wxCommandEvent dummy;
                OnSave(dummy);
            }
            else if (choice == wxID_NO)
            {
                m_dirty = false;
                m_firstRun = false; // discarding counts as acknowledging first-run
            }
            else
            {
                return;
            }
        }
        else
        {
            m_dirty = false;
        }

        std::wstring wGameToLaunch = iTargetGame == TARGET_GAME_MG1 ? L"steam://launch/2131680" : iTargetGame == TARGET_GAME_MGS2 ? L"steam://launch/2131640" : iTargetGame == TARGET_GAME_MGS3 ? L"steam://launch/2131650" : L"";
        if (!wGameToLaunch.empty())
        {
            HINSTANCE result = ShellExecuteW(
                nullptr,          // parent window
                L"open",          // operation
                wGameToLaunch.c_str(), // file/URL to open
                nullptr,          // parameters
                nullptr,          // default directory
                SW_SHOWNORMAL     // show command
            );

            if ((INT_PTR)result <= 32)
            {
                MessageBoxW(nullptr, L"Failed to launch Steam game.", L"Error", MB_ICONERROR);
            }
        }
        Close();
    }

    void OnClose(wxCloseEvent& event)
    {
        wxString changeList = BuildUnsavedChangeList();
        const bool hasRealChanges = !changeList.IsEmpty();

        if (hasRealChanges || m_firstRun || m_missingKeys)
        {
            wxString message;
            if (m_firstRun)
            {
                message =
                    "This appears to be your first time running the config tool.\n\n"
                    "You must save your settings before starting the game.";
                changeList.Clear();
            }
            else if (m_missingKeys)
            {
                message =
                    "Some settings were missing from your config file and will be restored.\n\n"
                    "Review the changes below and save before starting the game.";
            }
            else
            {
                message =
                    "You have unsaved changes.\n\n"
                    "What would you like to do?";
            }

            int choice = ShowUnsavedChangesDialog(message, "Save and Exit", "Exit Without Saving", changeList);
            if (choice == wxID_YES)
            {
                wxCommandEvent dummy;
                OnSave(dummy);
                event.Skip();
                return;
            }
            else if (choice == wxID_NO)
            {
                event.Skip();
                return;
            }
            else
            {
                event.Veto();
                return;
            }
        }

        event.Skip();
    }

    void ApplyPrerequisites()
    {
        for (auto& tab : kTabs)
        {
            for (auto& field : tab.second)
            {
                auto it = m_controls.find({ field.section, field.key });
                if (it == m_controls.end())
                {
                    continue;
                }

                wxWindow* ctrl = it->second;

                if (!field.prerequisite.has_value())
                {
                    continue;
                }

                auto prereq = field.prerequisite.value();
                auto prereqIt = m_controls.find(prereq);
                if (prereqIt == m_controls.end())
                {
                    continue;
                }

                wxWindow* prereqCtrl = prereqIt->second;

                bool enabled = false;

                // NEW: if matches provided, prerequisite is a wxChoice match
                if (!field.prerequisiteChoiceMatches.empty())
                {
                    if (auto* ch = wxDynamicCast(prereqCtrl, wxChoice))
                    {
                        const wxString sel = ch->GetStringSelection();

                        bool match = false;
                        for (const auto& allowed : field.prerequisiteChoiceMatches)
                        {
                            if (sel == allowed)
                            {
                                match = true;
                                break;
                            }
                        }

                        enabled = match;
                    }
                    else
                    {
                        // Misconfigured prerequisite: matches provided but prerequisite control is not a wxChoice.
                        enabled = false;
                    }
                }
                else
                {
                    // OLD: prerequisite treated as a wxCheckBox bool
                    if (auto* cb = wxDynamicCast(prereqCtrl, wxCheckBox))
                    {
                        enabled = cb->GetValue();
                    }
                    else
                    {
                        // Misconfigured prerequisite: expected checkbox.
                        enabled = false;
                    }
                }

                if (field.prerequisiteNegate)
                {
                    enabled = !enabled;
                }

                ctrl->Enable(enabled);

                if (enabled)
                {
                    continue;
                }

                // Reset to saved or default when disabling
                wxString path = field.section + "/" + field.key;
                wxString strVal;
                long intVal = 0;
                bool boolVal = false;
                double dblVal = 0.0;

                switch (field.type)
                {
                case Field::Bool:
                    boolVal = field.defaultInt != 0;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &boolVal);
                    if (auto* c = wxDynamicCast(ctrl, wxCheckBox))
                    {
                        c->SetValue(boolVal);
                    }
                    break;

                case Field::Int:
                    intVal = field.defaultInt;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &intVal);
                    if (auto* c = wxDynamicCast(ctrl, wxSpinCtrl))
                    {
                        c->SetValue((int)intVal);
                    }
                    break;

                case Field::Float:
                    dblVal = field.defaultFloat;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &dblVal);
                    if (auto* c = wxDynamicCast(ctrl, wxSpinCtrlDouble))
                    {
                        c->SetValue(dblVal);
                    }
                    break;

                case Field::Str:
                case Field::Hotkey:
                case Field::StickHotkey:
                    strVal = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &strVal);
                    strVal = Unquote(strVal);
                    if (auto* c = wxDynamicCast(ctrl, wxTextCtrl))
                    {
                        c->SetValue(strVal);
                    }
                    break;

                case Field::Choice:
                    strVal = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        MarkMissingKey(field.section, field.key);
                    }
                    m_conf->Read(path, &strVal);
                    strVal = Unquote(strVal);
                    if (auto* c = wxDynamicCast(ctrl, wxChoice))
                    {
                        if (c->FindString(strVal) != wxNOT_FOUND)
                        {
                            c->SetStringSelection(strVal);
                        }
                    }
                    break;

                default:
                    break;
                }
            }
        }
    }


    void OnSave(const wxCommandEvent&)
    {
        std::map<wxString, std::map<wxString, wxString>> iniData;

        for (auto& kv : m_controls)
        {
            const wxString& section = kv.first.first;
            const wxString& key = kv.first.second;
            wxWindow* ctrl = kv.second;

            const bool isRegionField =
                (section == ConfigKeys::Region_Section) &&
                (key == ConfigKeys::Region_Setting);

            const bool isLanguageField =
                (section == ConfigKeys::Language_Section) &&
                (key == ConfigKeys::Language_Setting);

            if (isRegionField || isLanguageField)
            {
                if (!isRegionField)
                {
                    // write both when we hit Region, skip Language pass
                    continue;
                }

                const auto pairs = GetActiveLanguagePairs();

                wxString regionNameWx;
                wxString languageNameWx;

                if (m_regionChoice)
                {
                    regionNameWx = m_regionChoice->GetStringSelection();
                }

                if (m_languageChoice)
                {
                    languageNameWx = m_languageChoice->GetStringSelection();
                }

                std::string outRegionCode;
                std::string outLangCode;

                if (!ResolveRegionLanguageCodes(
                    pairs,
                    regionNameWx.ToStdString(),
                    languageNameWx.ToStdString(),
                    outRegionCode,
                    outLangCode))
                {
                    if (!pairs.empty())
                    {
                        outRegionCode.assign(pairs[0].Game_Region);
                        outLangCode.assign(pairs[0].Game_Language);
                    }
                }

                iniData[ConfigKeys::Region_Section][ConfigKeys::Region_Setting] = QuoteIfNeeded(wxString(outRegionCode));
                iniData[ConfigKeys::Language_Section][ConfigKeys::Language_Setting] = QuoteIfNeeded(wxString(outLangCode));
                continue;
            }

            wxString value;

            if (auto* cb = wxDynamicCast(ctrl, wxCheckBox))
                value = cb->GetValue() ? "1" : "0";
            else if (auto* sp = wxDynamicCast(ctrl, wxSpinCtrl))
                value = wxString::Format("%d", sp->GetValue());
            else if (auto* tc = wxDynamicCast(ctrl, wxTextCtrl))
                value = QuoteIfNeeded(tc->GetValue());
            else if (auto* spd = wxDynamicCast(ctrl, wxSpinCtrlDouble))
                value = wxString::Format("%f", spd->GetValue());
            else if (auto* ch = wxDynamicCast(ctrl, wxChoice))
                value = QuoteIfNeeded(ch->GetStringSelection());

            iniData[section][key] = value;
        }

        std::ofstream out(m_iniPath.ToStdString(), std::ios::trunc);
        if (out)
        {
            for (auto& sec : iniData)
            {
                out << "[" << sec.first.ToStdString() << "]\n";
                for (auto& kv : sec.second)
                    out << kv.first.ToStdString() << "=" << kv.second.ToStdString() << "\n";
                out << "\n";
            }
        }

        m_dirty = false;
        m_firstRun = false;
        m_missingKeys = false;
        m_missingKeyList.clear();
        SnapshotCurrentValues();
        Close();
    }

    wxFileConfig* m_conf;
    wxString m_iniPath = wxString((Helper::FindASILocation(sFixName) / sSettingsFileName).wstring());

    std::unordered_map<Key, wxWindow*, KeyHash> m_controls;
    std::unordered_map<Key, wxString, KeyHash> m_snapshot;
};

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
        // Install logger BEFORE any wxLogError can fire (ConfigFrame ctor calls GetBannerResourceID)
        wxLog::SetActiveTarget(new wxLogErrorsOnlyGui(nullptr));
        wxLog::SetLogLevel(wxLOG_Error);

        wxImage::AddHandler(new wxPNGHandler);
        ConfigFrame* frame = new ConfigFrame();
        frame->Show();
        return true;
    }

    int OnExit() override
    {
        CloseOpenGamepads();

        if (g_SDLGamepadInitialized)
        {
            SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
            SDL_Quit();
            g_SDLGamepadInitialized = false;
        }

        return wxApp::OnExit();
    }
};

wxIMPLEMENT_APP(MyApp);

int main(int argc, char** argv)
{
    wxEntryStart(argc, argv);
    wxTheApp->CallOnInit();
    wxTheApp->OnRun();
    wxEntryCleanup();
    return 0;
}

#if defined(MGSHDFIX_SPECIFIC)
#undef MGSHDFIX_SPECIFIC
#endif
