#include "config_keys.hpp"
#include <wx/wx.h>
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

#include "version.h"

constexpr int iWindowSizeX = 716;
constexpr int iWindowSizeY = 760;

struct Field
{
    wxString section;
    wxString key;
    enum Type
    {
        Bool,
        Int,
        Str,
        Choice,
        Hotkey // new type for key/mouse capture
    } type;
    wxString defaultString;
    int defaultInt = 0;
    std::vector<wxString> choices;
};

// Custom control for capturing hotkeys
class HotkeyCaptureCtrl : public wxTextCtrl
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
    void OnKeyDown(wxKeyEvent& event)
    {
        int code = event.GetKeyCode();
        int raw = event.GetRawKeyCode(); // Windows VK code

        // Left/right modifiers
        if (GetKeyState(VK_LMENU) & 0x8000)
        {
            SetValue("LAlt"); return;
        }
        if (GetKeyState(VK_RMENU) & 0x8000)
        {
            SetValue("RAlt"); return;
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
            SetValue("LWin"); return;
        }
        if (GetKeyState(VK_RWIN) & 0x8000)
        {
            SetValue("RWin"); return;
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

// ----------------- FULL SCHEMA -----------------
static const std::vector<std::pair<wxString, std::vector<Field>>> kTabs = {
    {"General",{
        { ConfigKeys::EffectSpeedFixes_Section, ConfigKeys::EffectSpeedFixes_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::FixAimingAfterEquip_Section, ConfigKeys::FixAimingAfterEquip_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::DisableMouseCursor_Section, ConfigKeys::DisableMouseCursor_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::FixAimingFullTilt_Section, ConfigKeys::FixAimingFullTilt_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::AchievementPersistence_Section, ConfigKeys::AchievementPersistence_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Section, ConfigKeys::PauseOnFocusLoss_SpeedrunnerBugfixOverride_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::ForceStereoAudio_Section, ConfigKeys::ForceStereoAudio_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Section, ConfigKeys::KeepAimingAfterFiring_InFirstPerson_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::KeepAimingAfterFiring_Always_Section, ConfigKeys::KeepAimingAfterFiring_Always_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::KeepAimingAfterFiring_OnLockOn_Section, ConfigKeys::KeepAimingAfterFiring_OnLockOn_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::EnablePauseOnFocusLoss_Section, ConfigKeys::EnablePauseOnFocusLoss_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::CheckForUpdates_Section, ConfigKeys::CheckForUpdates_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::UpdateConsoleNotifications_Section, ConfigKeys::UpdateConsoleNotifications_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::VerboseLogging_Section, ConfigKeys::VerboseLogging_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::Region_Section, ConfigKeys::Region_Setting, Field::Choice, "US", 0, { std::begin(kLauncherConfigRegions), std::end(kLauncherConfigRegions) } },
        { ConfigKeys::Language_Section, ConfigKeys::Language_Setting, Field::Choice, "EN", 0,{ std::begin(kLauncherConfigLanguages), std::end(kLauncherConfigLanguages) } },
    }},
    { "Graphics", {
        { ConfigKeys::ForceWindowSize_Section, ConfigKeys::ForceWindowSize_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::WindowWidth_Section, ConfigKeys::WindowWidth_Setting, Field::Int, "", 0, {} },
        { ConfigKeys::WindowedMode_Section, ConfigKeys::WindowedMode_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::WindowHeight_Section, ConfigKeys::WindowHeight_Setting, Field::Int, "", 0, {} },
        { ConfigKeys::BorderlessWindowed_Section, ConfigKeys::BorderlessWindowed_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::RenderScaleWidth_Section, ConfigKeys::RenderScaleWidth_Setting, Field::Int, "", 0, {} },
        { ConfigKeys::RenderScaleHeight_Section, ConfigKeys::RenderScaleHeight_Setting, Field::Int, "", 0, {} },
        { ConfigKeys::AnisotropicFiltering_Section, ConfigKeys::AnisotropicFiltering_Setting, Field::Int, "", 16, {} },
        { ConfigKeys::DisableTextureFiltering_Section, ConfigKeys::DisableTextureFiltering_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::FixAspectRatio_Section, ConfigKeys::FixAspectRatio_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::FixHUD_Section, ConfigKeys::FixHUD_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::FixFOV_Section, ConfigKeys::FixFOV_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::FramebufferFix_Section, ConfigKeys::FramebufferFix_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::FixVectorRain_Section, ConfigKeys::FixVectorRain_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::VectorLineScale_Section, ConfigKeys::VectorLineScale_Setting, Field::Int, "", 360, {} },
        { ConfigKeys::FixVectorUI_Section, ConfigKeys::FixVectorUI_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::CtrlType_Section, ConfigKeys::CtrlType_Setting, Field::Choice, "XBOX", 0,{ std::begin(kLauncherConfigCtrlTypes), std::end(kLauncherConfigCtrlTypes) } },
    }},
    { "Tweaks", {
        { ConfigKeys::LauncherJumpStart_Section, ConfigKeys::LauncherJumpStart_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::SkipIntroLogos_Section, ConfigKeys::SkipIntroLogos_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::SkipLauncher_Section, ConfigKeys::SkipLauncher_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::SkipLauncherMSXGame_Section, ConfigKeys::SkipLauncherMSXGame_Setting, Field::Choice, "MG1", 0, {"MG1","MG2"} },
        { ConfigKeys::MSXWallType_Section, ConfigKeys::MSXWallType_Setting, Field::Int, "", 0, {} },
        { ConfigKeys::MSXWallAlign_Section, ConfigKeys::MSXWallAlign_Setting, Field::Choice, "Center", 0, {"Left","Right","Center"} },
        { ConfigKeys::MuteWarning_Section, ConfigKeys::MuteWarning_Setting, Field::Bool, "", 1, {} },
        { ConfigKeys::MGS2Sunglasses_Section, ConfigKeys::MGS2Sunglasses_Setting, Field::Choice, "Normal", 0, {"Normal","Always","Never"} },
    }},
    { "Controls | Hotkeys", {
        { ConfigKeys::ToggleRainShader_Section, ConfigKeys::ToggleRainShader_Setting, Field::Hotkey, "Insert", 0, {} },
        { ConfigKeys::ToggleUIShader_Section, ConfigKeys::ToggleUIShader_Setting, Field::Hotkey, "Delete", 0, {} },
        { ConfigKeys::CycleWireframeMode_Section, ConfigKeys::CycleWireframeMode_Setting, Field::Hotkey, "End", 0, {} },
        { ConfigKeys::OverrideMouseSensitivity_Section, ConfigKeys::OverrideMouseSensitivity_Setting, Field::Bool, "", 0, {} },
        { ConfigKeys::MouseSensitivity_XMultiplier_Section, ConfigKeys::MouseSensitivity_XMultiplier_Setting, Field::Int, "", 1, {} },
        { ConfigKeys::MouseSensitivity_YMultiplier_Section, ConfigKeys::MouseSensitivity_YMultiplier_Setting, Field::Int, "", 1, {} },
    }},
    { "Achievements", {
        { ConfigKeys::ResetAllAchievements_Section, ConfigKeys::ResetAllAchievements_Setting, Field::Bool, "", 0, {} },
    }},
};

// -------- Resource Loader Helpers --------
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
    if (!hRes) return 0;
    return SizeofResource(nullptr, hRes);
}

static int GetBannerResourceID()
{
    namespace fs = std::filesystem;
    fs::path exePath = wxGetCwd().ToStdString();
    fs::path parentPath = exePath.parent_path();

    if (fs::exists(parentPath / "METAL GEAR.exe"))
        return IDB_BANNER_MG1;
    if (fs::exists(parentPath / "METAL GEAR SOLID2.exe"))
        return IDB_BANNER_MGS2;
    if (fs::exists(parentPath / "METAL GEAR SOLID3.exe"))
        return IDB_BANNER_MGS3;

    return IDB_BANNER_MG1;
}

// ... all your existing includes ...
#include <wx/dcbuffer.h> // For wxAutoBufferedPaintDC

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
        : wxFrame(nullptr, wxID_ANY, "MGSHDFix Settings",
            wxDefaultPosition, wxSize(iWindowSizeX, iWindowSizeY),
            wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
    {
        SetMinSize(wxSize(iWindowSizeX, iWindowSizeY));
        SetMaxSize(wxSize(iWindowSizeX, iWindowSizeY));

        HWND hwnd = (HWND)GetHWND();
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, 0);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, 0);

        wxString cwd = wxGetCwd();
        m_iniPath = cwd + "\\MGSHDFix.settings";
        m_conf = new wxFileConfig("", "", m_iniPath, "", wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);

        wxNotebook* tabs = new wxNotebook(this, wxID_ANY);

        for (auto& tab : kTabs)
        {
            wxPanel* panel = new wxPanel(tabs);
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

                grid->Add(new wxStaticText(sectionSizer->GetStaticBox(), wxID_ANY, field.key),
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

                wxWindow* ctrl = nullptr;
                wxString path = field.section + "/" + field.key;

                switch (field.type)
                {
                case Field::Bool:
                {
                    bool v = field.defaultInt != 0;
                    m_conf->Read(path, &v);
                    auto* cb = new wxCheckBox(sectionSizer->GetStaticBox(), wxID_ANY, "");
                    cb->SetValue(v);
                    ctrl = cb;
                    break;
                }
                case Field::Int:
                {
                    long v = field.defaultInt;
                    m_conf->Read(path, &v);
                    auto* sp = new wxSpinCtrl(sectionSizer->GetStaticBox(), wxID_ANY, std::to_string(v),
                        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, v);
                    ctrl = sp;
                    break;
                }
                case Field::Str:
                {
                    wxString v = field.defaultString;
                    m_conf->Read(path, &v);
                    ctrl = new wxTextCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v);
                    break;
                }
                case Field::Choice:
                {
                    wxString v = field.defaultString;
                    m_conf->Read(path, &v);
                    auto* ch = new wxChoice(sectionSizer->GetStaticBox(), wxID_ANY);
                    for (auto& c : field.choices) ch->Append(c);
                    if (ch->FindString(v) == wxNOT_FOUND) ch->Append(v);
                    ch->SetStringSelection(v);
                    ctrl = ch;
                    break;
                }
                case Field::Hotkey:
                {
                    wxString v = field.defaultString;
                    m_conf->Read(path, &v);
                    ctrl = new HotkeyCaptureCtrl(sectionSizer->GetStaticBox(), wxID_ANY, v);
                    break;
                }
                }

                grid->Add(ctrl, 0, wxEXPAND);
                m_controls[{field.section, field.key}] = ctrl;
            }

            panel->SetSizer(vbox);
            tabs->AddPage(panel, tab.first, false);
        }

        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        int bannerID = GetBannerResourceID();
        BannerPanel* banner = new BannerPanel(this, bannerID);
        banner->SetCursor(wxCursor(wxCURSOR_HAND));
        banner->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&)
            {
#ifdef PRIMARY_REPO_URL
                wxLaunchDefaultBrowser(PRIMARY_REPO_URL);
#endif
            });
        mainSizer->Add(banner, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 0);


        mainSizer->Add(tabs, 1, wxEXPAND | wxALL, 5);

        wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        btnSizer->AddStretchSpacer();
        auto* saveBtn = new wxButton(this, wxID_SAVE, "Save");
        auto* exitBtn = new wxButton(this, wxID_EXIT, "Exit");
        btnSizer->Add(saveBtn, 0, wxRIGHT, 5);
        btnSizer->Add(exitBtn, 0);
        mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 5);

        SetSizer(mainSizer);
        Centre();

        Bind(wxEVT_BUTTON, &ConfigFrame::OnSave, this, wxID_SAVE);
        Bind(wxEVT_BUTTON, [&](wxCommandEvent&)
            {
                Close();
            }, wxID_EXIT);
    }

private:
    void OnSave(wxCommandEvent&)
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
                value = tc->GetValue();
            else if (auto* ch = wxDynamicCast(ctrl, wxChoice))
                value = ch->GetStringSelection();
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
        Close();
    }

    wxFileConfig* m_conf;
    wxString m_iniPath;

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
