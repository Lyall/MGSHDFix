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

#include "config_keys.hpp"
#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/notebook.h>
#include <wx/fileconf.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/stdpaths.h>
#include <wx/mstream.h>
#include <unordered_map>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <windows.h>

#include "helper.hpp"
#include "version.h"
#include "tab_data.hpp"
#include "updater.hpp"

constexpr int iWindowSizeX = 716;
constexpr int iWindowSizeY = 680;
constexpr const char* sSettingsFileName = "MGSHDFix.settings";
constexpr bool bFullLengthFields = false; //if you want the boxes to span half the window's width.

#define NEXUS_MG1_URL "https://www.nexusmods.com/metalgearandmetalgear2mc/mods/9"
#define NEXUS_MGS2_URL "https://www.nexusmods.com/metalgearsolid2mc/mods/49"
#define NEXUS_MGS3_URL "https://www.nexusmods.com/metalgearsolid3mc/mods/139"

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

class HotkeyCaptureCtrl final : public wxTextCtrl
{
public:
    HotkeyCaptureCtrl(wxWindow* parent, wxWindowID id, const wxString& value = "")
        : wxTextCtrl(parent, id, value, wxDefaultPosition, wxDefaultSize,
            wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER)
    {
        Bind(wxEVT_KEY_DOWN, &HotkeyCaptureCtrl::OnKeyDown, this);
        Bind(wxEVT_MIDDLE_DOWN, &HotkeyCaptureCtrl::OnMouseClick, this);
        Bind(wxEVT_AUX1_DOWN, &HotkeyCaptureCtrl::OnMouseClick, this);
        Bind(wxEVT_AUX2_DOWN, &HotkeyCaptureCtrl::OnMouseClick, this);
    }

private:
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
            name = "Mouse3";
        else if (event.GetButton() == wxMOUSE_BTN_AUX1)
            name = "Mouse4";
        else if (event.GetButton() == wxMOUSE_BTN_AUX2)
            name = "Mouse5";

        SetValue(name);
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
    std::filesystem::path exePath = wxGetCwd().ToStdString();
    exePath = exePath.parent_path(); 
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
    wxLogError("Unable to find any known Master Collection games in %s", exePath.string());

    return IDB_BANNER_MG1;
}

// Custom fixed-size banner panel
class BannerPanel : public wxPanel
{
public:
    BannerPanel(wxWindow* parent, int bannerResId)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(700, 100), wxBORDER_NONE)
    {
        wxImage img;
        wxMemoryInputStream memStream(
            FindResourceData(bannerResId, L"PNG"),
            FindResourceSize(bannerResId, L"PNG")
        );
        if (img.LoadFile(memStream, wxBITMAP_TYPE_PNG) && img.IsOk())
        {
            m_bitmap = wxBitmap(img);
        }

        SetMinSize(wxSize(700, 100));
        SetMaxSize(wxSize(700, 100));
        Bind(wxEVT_PAINT, &BannerPanel::OnPaint, this);
        SetBackgroundStyle(wxBG_STYLE_PAINT); // Needed for buffered paint
    }

private:
    wxBitmap m_bitmap;

    void OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();
        if (m_bitmap.IsOk())
        {
            dc.DrawBitmap(m_bitmap, 0, 0, false); // Draw at native size
        }
    }
};

class ConfigFrame : public wxFrame
{
public:
    ConfigFrame()
        : wxFrame(nullptr, wxID_ANY, FIX_NAME " v" VERSION_STRING " - Universal Config Tool",
            wxDefaultPosition, wxSize(iWindowSizeX, iWindowSizeY),
            wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
    {
        SetMinSize(wxSize(iWindowSizeX, iWindowSizeY));
        SetMaxSize(wxSize(iWindowSizeX, iWindowSizeY));

        HWND hwnd = (HWND)GetHWND();
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, 0);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, 0);

        if (!std::filesystem::exists(std::filesystem::path(m_iniPath.ToStdWstring())))
        {
            m_firstRun = true;
        }
        m_conf = new wxFileConfig("", "", m_iniPath, "", wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);

        m_tabs = new wxNotebook(this, wxID_ANY);

        for (auto& tab : kTabs)
        {
            wxPanel* panel = new wxPanel(m_tabs);
            wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
            wxString currentSection;
            wxStaticBoxSizer* sectionSizer = nullptr;
            wxFlexGridSizer* grid = nullptr;

            for (auto& field : tab.second)
            {
                if (field.section != currentSection)
                {
                    currentSection = field.section;
                    sectionSizer = new wxStaticBoxSizer(wxVERTICAL, panel, currentSection);
                    grid = new wxFlexGridSizer(0, 4, 5, 10);
                    grid->AddGrowableCol(1, 1);
                    grid->AddGrowableCol(3, 1);
                    sectionSizer->Add(grid, 0, wxEXPAND | wxALL, 5);
                    vbox->Add(sectionSizer, 0, wxEXPAND | wxALL, 5);
                }

                if (field.section == "About")
                {
                    wxBoxSizer* aboutSizer = new wxBoxSizer(wxVERTICAL);

                    auto* aboutText = new wxStaticText(
                        sectionSizer->GetStaticBox(),
                        wxID_ANY,
                        "Universal Config Tool, licensed under MIT.\n" //Do not remove this notice.
                        "     Created by Afevis.\n"                            //Do not remove this notice.
                        "\n"
                        "MGSHDFix, licensed under MIT.\n"
                        "     Originally created by Lyall.\n"
                        "     Maintained by Afevis (aka ShizCalev.)\n"
                        "     Additional contributions by Emoose, Cipherxof (aka TriggerHappy), Bud11, and Zenf0."
                    );
                    aboutSizer->Add(aboutText, 0, wxALL, 5);

                    sectionSizer->Add(aboutSizer, 0, wxEXPAND | wxALL, 5);
                    continue; // skip normal control creation
                }


                // Label + optional help stacked vertically
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
                        m_missingKeys = true;
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
                        m_missingKeys = true;
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
                        m_missingKeys = true;
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);
                    ctrl = new wxTextCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v);
                    break;
                }
                case Field::Choice:
                {
                    wxString v = field.defaultString;
                    if (!m_conf->HasEntry(path))
                    {
                        m_missingKeys = true;
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);

                    auto* ch = new wxChoice(sectionSizer->GetStaticBox(), wxID_ANY);
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
                        m_missingKeys = true;
                    }
                    m_conf->Read(path, &v);
                    v = Unquote(v);
                    ctrl = new HotkeyCaptureCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v);
                    break;
                }
                case Field::Spacer:
                {
                    auto* spacer = new wxPanel(sectionSizer->GetStaticBox(), wxID_ANY);
                    spacer->SetMinSize(wxSize(0, 10));
                    grid->Add(spacer, 0, wxEXPAND);
                    continue;
                }
                case Field::Float:
                {
                    double v = field.defaultFloat;
                    if (!m_conf->HasEntry(path))
                    {
                        m_missingKeys = true;
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
                    sp->SetMinSize(wxSize(90, -1));
                    sp->SetSizeHints(90, -1);
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
                            tip = field.tooltip;

                        // Always append default value
                        if (!tip.IsEmpty())
                            tip += "\n\n";
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

                        ctrl->SetToolTip(tip);
                    }

                    int flags = wxALIGN_CENTER_VERTICAL;
                    if (bFullLengthFields)
                    {
                        flags |= wxEXPAND;
                    }

                    grid->Add(ctrl, 0, flags);


                    m_controls[{field.section, field.key}] = ctrl;
                }
            }

            panel->SetSizer(vbox);
            m_tabs->AddPage(panel, tab.first, false);
        }

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        int bannerID = GetBannerResourceID();
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

        ApplyPrerequisites();
        HandleUpdateCheckPreference();
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
    bool m_dirty = false;
    bool m_firstRun = false;
    bool m_missingKeys = false;

    wxNotebook* m_tabs = nullptr;
    void MarkDirty(wxEvent& e)
    {
        m_dirty = true;
        e.Skip();
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
        if (m_dirty || m_firstRun || m_missingKeys)
        {
            wxString message;
            if (m_firstRun)
            {
                message =
                    "This appears to be your first time running the config tool.\n\n"
                    "You must save your settings before starting the game.";
            }
            else if (m_missingKeys)
            {
                message =
                    "Some settings were missing from your config file.\n\n"
                    "You must save your settings before starting the game.";
            }
            else
            {
                message =
                    "You have unsaved changes.\n\n"
                    "Do you want to save them before launching the game?";
            }
            wxMessageDialog dlg(
                this,
                message,
                "Unsaved Changes",
                wxYES_NO | wxCANCEL | wxICON_WARNING
            );
            dlg.SetYesNoCancelLabels("Save", "Discard", "Cancel");

            int choice = dlg.ShowModal();
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

        std::filesystem::path exePath = wxGetCwd().ToStdString();
        exePath = exePath.parent_path() / "launcher.exe";

        if (std::filesystem::exists(exePath))
        {
            wxExecute(exePath.string());
        }
        else
        {
            wxLogError("Launcher.exe not found in %s", wxGetCwd());
        }
        Close();
    }




    void OnClose(wxCloseEvent& event)
    {
        if (m_dirty || m_firstRun || m_missingKeys)
        {
            wxString message;
            if (m_firstRun)
            {
                message =
                    "This appears to be your first time running the config tool.\n\n"
                    "You must save your settings before starting the game.";
            }
            else if (m_missingKeys)
            {
                message =
                    "Some settings were missing from your config file.\n\n"
                    "You must save your settings before starting the game.";
            }
            else
            {
                message =
                    "You have unsaved changes.\n\n"
                    "What would you like to do?";
            }
            wxMessageDialog dlg(
                this,
                message,
                "Unsaved Changes",
                wxYES_NO | wxCANCEL | wxICON_WARNING
            );
            dlg.SetYesNoCancelLabels("Save and Exit", "Exit Without Saving", "Cancel");

            int choice = dlg.ShowModal();
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
                    continue;

                wxWindow* ctrl = it->second;

                if (field.prerequisite.has_value())
                {
                    auto prereq = field.prerequisite.value();
                    auto prereqIt = m_controls.find(prereq);
                    if (prereqIt != m_controls.end())
                    {
                        if (auto* cb = wxDynamicCast(prereqIt->second, wxCheckBox))
                        {
                            bool enabled = cb->GetValue();

                            if (field.prerequisiteNegate)
                                enabled = !enabled;

                            ctrl->Enable(enabled);

                            if (!enabled)
                            {
                                // Reset to saved or default
                                wxString path = field.section + "/" + field.key;
                                wxString strVal;
                                long intVal;
                                bool boolVal;

                                switch (field.type)
                                {
                                case Field::Bool:
                                    boolVal = field.defaultInt != 0;
                                    if (!m_conf->HasEntry(path))
                                    {
                                        m_missingKeys = true;
                                    }
                                    m_conf->Read(path, &boolVal);
                                    if (auto* c = wxDynamicCast(ctrl, wxCheckBox))
                                        c->SetValue(boolVal);
                                    break;

                                case Field::Int:
                                    intVal = field.defaultInt;
                                    if (!m_conf->HasEntry(path))
                                    {
                                        m_missingKeys = true;
                                    }
                                    m_conf->Read(path, &intVal);
                                    if (auto* c = wxDynamicCast(ctrl, wxSpinCtrl))
                                        c->SetValue(intVal);
                                    break;

                                case Field::Str:
                                case Field::Hotkey:
                                    strVal = field.defaultString;
                                    if (!m_conf->HasEntry(path))
                                    {
                                        m_missingKeys = true;
                                    }
                                    m_conf->Read(path, &strVal);
                                    strVal = Unquote(strVal);
                                    if (auto* c = wxDynamicCast(ctrl, wxTextCtrl))
                                        c->SetValue(strVal);
                                    break;

                                case Field::Choice:
                                    strVal = field.defaultString;
                                    if (!m_conf->HasEntry(path))
                                    {
                                        m_missingKeys = true;
                                    }
                                    m_conf->Read(path, &strVal);
                                    strVal = Unquote(strVal);
                                    if (auto* c = wxDynamicCast(ctrl, wxChoice))
                                        if (c->FindString(strVal) != wxNOT_FOUND)
                                            c->SetStringSelection(strVal);
                                    break;
                                }
                            }
                        }
                    }
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
        Close();
    }


    wxFileConfig* m_conf;
    wxString m_iniPath = wxString((Helper::FindASILocation(sFixName) / sSettingsFileName).wstring());
    using Key = std::pair<wxString, wxString>;
    struct KeyHash
    {
        size_t operator()(const Key& k) const
        {
            return std::hash<std::string>()((k.first + k.second).ToStdString());
        }
    };
    std::unordered_map<Key, wxWindow*, KeyHash> m_controls;
};

class MyApp : public wxApp
{
public:
    bool OnInit() override
    {
        wxImage::AddHandler(new wxPNGHandler);
        ConfigFrame* frame = new ConfigFrame();
        frame->Show();
        return true;
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
