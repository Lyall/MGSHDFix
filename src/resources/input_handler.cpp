#include "stdafx.h"

#include "common.hpp"
#include "input_handler.hpp"

#include <Xinput.h> //not actually using xinput, as steam input blocks it out - just using the VK defs.

#include "d3d11_api.hpp"
#include "logging.hpp"
#include "steamworks_api.hpp"

#include "version.h"

std::unordered_map<int, std::string> InputHandler::s_UsedKeybinds {};


namespace
{
    constexpr int VK_WHEELUP = 0x1000;
    constexpr int VK_WHEELDOWN = 0x1001;


    constexpr BYTE kPadTriggerThreshold = 30;
    constexpr SHORT kPadStickThreshold = 12000;

    bool g_WheelUpPressed = false;
    bool g_WheelDownPressed = false;

    struct HeldHotkeyState
    {
        bool repeatWhileHeld = false;
        DWORD repeatDelayMs = 0;
        ULONGLONG nextRepeatTick = 0;
    };

    constexpr DWORD kDefaultHeldHotkeyRepeatDelayMs = 100;
    std::vector<HeldHotkeyState> g_HeldHotkeyStates;
    WNDPROC g_InputHandlerOriginalWndProc = nullptr;

    LRESULT CALLBACK InputHandlerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_MOUSEWHEEL)
        {
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);

            if (delta > 0)
            {
                g_WheelUpPressed = true;
            }
            else if (delta < 0)
            {
                g_WheelDownPressed = true;
            }
        }

        return CallWindowProc(g_InputHandlerOriginalWndProc, hwnd, msg, wParam, lParam);
    }

    void InstallMouseWheelCapture(HWND hwnd)
    {
        if (hwnd == nullptr)
        {
            return;
        }

        if (g_InputHandlerOriginalWndProc != nullptr)
        {
            return;
        }

        g_InputHandlerOriginalWndProc = reinterpret_cast<WNDPROC>(
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(InputHandlerWndProc)));
    }

    bool IsGamepadButtonDown(const XINPUT_GAMEPAD& pad, int vkCode)
    {
        switch (vkCode)
        {
        case VK_PAD_A: return (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
        case VK_PAD_B: return (pad.wButtons & XINPUT_GAMEPAD_B) != 0;
        case VK_PAD_X: return (pad.wButtons & XINPUT_GAMEPAD_X) != 0;
        case VK_PAD_Y: return (pad.wButtons & XINPUT_GAMEPAD_Y) != 0;
        case VK_PAD_LSHOULDER: return (pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        case VK_PAD_RSHOULDER: return (pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        case VK_PAD_BACK: return (pad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
        case VK_PAD_START: return (pad.wButtons & XINPUT_GAMEPAD_START) != 0;
        case VK_PAD_LTHUMB_PRESS: return (pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
        case VK_PAD_RTHUMB_PRESS: return (pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        case VK_PAD_DPAD_UP: return (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
        case VK_PAD_DPAD_DOWN: return (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        case VK_PAD_DPAD_LEFT: return (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        case VK_PAD_DPAD_RIGHT: return (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
        case VK_PAD_LTRIGGER: return pad.bLeftTrigger >= kPadTriggerThreshold;
        case VK_PAD_RTRIGGER: return pad.bRightTrigger >= kPadTriggerThreshold;
        case VK_PAD_LTHUMB_UP: return pad.sThumbLY >= kPadStickThreshold;
        case VK_PAD_LTHUMB_DOWN: return pad.sThumbLY <= -kPadStickThreshold;
        case VK_PAD_LTHUMB_LEFT: return pad.sThumbLX <= -kPadStickThreshold;
        case VK_PAD_LTHUMB_RIGHT: return pad.sThumbLX >= kPadStickThreshold;
        case VK_PAD_RTHUMB_UP: return pad.sThumbRY >= kPadStickThreshold;
        case VK_PAD_RTHUMB_DOWN: return pad.sThumbRY <= -kPadStickThreshold;
        case VK_PAD_RTHUMB_LEFT: return pad.sThumbRX <= -kPadStickThreshold;
        case VK_PAD_RTHUMB_RIGHT: return pad.sThumbRX >= kPadStickThreshold;
        default: return false;
        }
    }

    bool IsGamepadInput(int vkCode)
    {
        switch (vkCode)
        {
        case VK_PAD_A:
        case VK_PAD_B:
        case VK_PAD_X:
        case VK_PAD_Y:
        case VK_PAD_LSHOULDER:
        case VK_PAD_RSHOULDER:
        case VK_PAD_LTRIGGER:
        case VK_PAD_RTRIGGER:
        case VK_PAD_DPAD_UP:
        case VK_PAD_DPAD_DOWN:
        case VK_PAD_DPAD_LEFT:
        case VK_PAD_DPAD_RIGHT:
        case VK_PAD_START:
        case VK_PAD_BACK:
        case VK_PAD_LTHUMB_PRESS:
        case VK_PAD_RTHUMB_PRESS:
        case VK_PAD_LTHUMB_UP:
        case VK_PAD_LTHUMB_DOWN:
        case VK_PAD_LTHUMB_LEFT:
        case VK_PAD_LTHUMB_RIGHT:
        case VK_PAD_RTHUMB_UP:
        case VK_PAD_RTHUMB_DOWN:
        case VK_PAD_RTHUMB_LEFT:
        case VK_PAD_RTHUMB_RIGHT:
            return true;
        default:
            return false;
        }
    }

        const char* GetSteamInputActionNameForGamepadInput(int vkCode)
    {
        if (eGameType & MGS2)
        {
            switch (vkCode)
            {
            case VK_PAD_Y: return "ingame_cmn_action_btn";
            case VK_PAD_B: return "ingame_cmn_punch";
            case VK_PAD_X: return "ingame_cmn_weapon";
            case VK_PAD_A: return "ingame_cmn_sneaking";
            case VK_PAD_DPAD_UP: return "ingame_cmn_move_up";
            case VK_PAD_DPAD_RIGHT: return "ingame_cmn_move_right";
            case VK_PAD_DPAD_DOWN: return "ingame_cmn_move_down";
            case VK_PAD_DPAD_LEFT: return "ingame_cmn_move_left";
            case VK_PAD_LTHUMB_PRESS: return "ingame_cmn_hold_weapon";
            case VK_PAD_RTHUMB_PRESS: return "ingame_cmn_blade_stab";
            case VK_PAD_LSHOULDER: return "ingame_cmn_lock_on";
            case VK_PAD_RSHOULDER: return "ingame_cmn_pov_cam";
            case VK_PAD_LTRIGGER: return "ingame_cmn_equip_window";
            case VK_PAD_RTRIGGER: return "ingame_cmn_weapon_window";
            case VK_PAD_BACK: return "ingame_cmn_radio_menu";
            case VK_PAD_START: return "ingame_cmn_pause_menu";
            default: return nullptr;
            }
        }

        if (eGameType & (MGS3 | MG))
        {
            switch (vkCode)
            {
            case VK_PAD_Y: return "ingame_cmn_action_btn";
            case VK_PAD_B: return "ingame_cmn_punch";
            case VK_PAD_X: return "ingame_cmn_weapon";
            case VK_PAD_A: return "ingame_cmn_sneaking";
            case VK_PAD_DPAD_UP: return "ingame_cmn_move_up";
            case VK_PAD_DPAD_RIGHT: return "ingame_cmn_move_right";
            case VK_PAD_DPAD_DOWN: return "ingame_cmn_move_down";
            case VK_PAD_DPAD_LEFT: return "ingame_cmn_move_left";
            case VK_PAD_LTHUMB_PRESS: return "ingame_cmn_interrogate";
            case VK_PAD_RTHUMB_PRESS: return "ingame_cmn_change_view";
            case VK_PAD_LSHOULDER: return "ingame_cmn_weapon_aim";
            case VK_PAD_RSHOULDER: return "ingame_cmn_pov_cam";
            case VK_PAD_LTRIGGER: return (eGameType & MG) ? "ingame_cmn_equip_menu" : "ingame_cmn_equip_window";
            case VK_PAD_RTRIGGER: return (eGameType & MG) ? "ingame_cmn_weapon_menu" : "ingame_cmn_weapon_window";
            case VK_PAD_BACK: return "ingame_cmn_radio_menu";
            case VK_PAD_START: return (eGameType & MG) ? "ingame_cmn_pause_menu" : "ingame_cmn_survival_viwer";
            default: return nullptr;
            }
        }

        return nullptr;
    }

    bool IsSteamInputGamepadInputDown(int vkCode)
    {
        if (!g_SteamAPI.bInitialized)
        {
            return false;
        }

        const char* actionName = GetSteamInputActionNameForGamepadInput(vkCode);
        if (actionName == nullptr)
        {
            return false;
        }

        ISteamInput* steamInput = SteamInput();
        if (steamInput == nullptr)
        {
            return false;
        }

        steamInput->RunFrame();

        const InputDigitalActionHandle_t actionHandle = steamInput->GetDigitalActionHandle(actionName);
        if (actionHandle == 0)
        {
            return false;
        }

        InputHandle_t controllerHandles[STEAM_INPUT_MAX_COUNT] = {};
        const int controllerCount = steamInput->GetConnectedControllers(controllerHandles);

        for (int i = 0; i < controllerCount; ++i)
        {
            const InputHandle_t handle = controllerHandles[i];
            if (handle == 0)
            {
                continue;
            }

            const InputDigitalActionData_t actionData = steamInput->GetDigitalActionData(handle, actionHandle);
            if (actionData.bActive && actionData.bState)
            {
                return true;
            }
        }

        return false;
    }

    void EnsureHeldHotkeyStateCount(size_t count)
    {
        if (g_HeldHotkeyStates.size() >= count)
        {
            return;
        }

        g_HeldHotkeyStates.resize(count);
    }

}

void InputHandler::FatalKeyError(const std::string& section,
    const std::string& key,
    const std::string& reason)
{
    const std::string message =
        "[" + sFixName + " Config Helper] Failed to read config key '" + key +
        "' in section '" + section + "': " + reason;

    spdlog::error(message);
    spdlog::error("Please run the MGSHDFix Config Tool to update your settings file to the latest version.");
    spdlog::error("If this issue persists after updating your settings file, you can find our Discord support channel at the Metal Gear Network Discord - #HDFix: {}", DISCORD_URL);
    Logging::ShowConsole();
    std::cout << message << std::endl;
    std::cout << "Please run the MGSHDFix Config Tool to update your settings file to the latest version." << std::endl;
    std::cout << "If this issue persists after updating your settings file, you can find our Discord support channel at the Metal Gear Network Discord - #HDFix: " << DISCORD_URL << std::endl;

    FreeLibraryAndExitThread(baseModule, 1);
}

std::string InputHandler::NormalizeKeyAlias(std::string s)
{
    auto is_space = [](unsigned char c)
        {
            return std::isspace(c) != 0;
        };

    size_t a = 0, b = s.size();
    while (a < b && is_space((unsigned char)s[a])) ++a;
    while (b > a && is_space((unsigned char)s[b - 1])) --b;
    s.assign(s.begin() + a, s.begin() + b);

    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
    {
        if (c == ' ' || c == '_' || c == '-') continue;
        out.push_back((char)std::toupper(c));
    }
    return out;
}

bool InputHandler::TryParseHexVK(const std::string& s, int& outVk)
{
    if (s.size() >= 3 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        unsigned int v = 0;
        std::stringstream ss;
        ss << std::hex << s;
        if (ss >> v)
        {
            outVk = (int)v;
            return true;
        }
    }
    return false;
}

std::optional<int> InputHandler::ParseVirtualKey(const std::string& aliasRaw)
{
    if (aliasRaw.empty())
        return std::nullopt;

    int vkHex = 0;
    if (TryParseHexVK(aliasRaw, vkHex))
        return vkHex;

    std::string s = NormalizeKeyAlias(aliasRaw);

    // Strip "VK" prefix (VK_INSERT, vk-delete, etc.)
    if (s.size() > 2 && s.rfind("VK", 0) == 0)
        s = s.substr(2);

    // Normalize keypad prefixes: KP*, KEYPAD*, NP*, NUM* -> NUMPAD*
    if (s.rfind("KP", 0) == 0)
    {
        s = "NUMPAD" + s.substr(2);
    }
    else if (s.rfind("KEYPAD", 0) == 0)
    {
        s = "NUMPAD" + s.substr(6);
    }
    else if (s.rfind("NP", 0) == 0)
    {
        s = "NUMPAD" + s.substr(2);
    }
    else if (s.rfind("NUM", 0) == 0 && s.rfind("NUMPAD", 0) != 0)
    {
        s = "NUMPAD" + s.substr(3);
    }

    // Unambiguous single-char: only '*' (maps to numpad multiply).
    if (s.size() == 1 && s[0] == '*')
        return VK_MULTIPLY;

    // NUMPAD family: digits/operators/enter (with colloquialisms)
    if (s.rfind("NUMPAD", 0) == 0)
    {
        const std::string rest = s.substr(6);

        if (rest.size() == 1 && std::isdigit(static_cast<unsigned char>(rest[0])) != 0)
            return VK_NUMPAD0 + (rest[0] - '0');

        if (rest == "*" || rest == "STAR" || rest == "ASTERISK" || rest == "MULTIPLY" || rest == "MUL")
            return VK_MULTIPLY;
        if (rest == "/" || rest == "DIVIDE" || rest == "DIV" || rest == "SLASH" || rest == "FORWARDSLASH" || rest == "FSLASH")
            return VK_DIVIDE;
        if (rest == "+" || rest == "PLUS" || rest == "ADD")
            return VK_ADD;
        if (rest == "-" || rest == "MINUS" || rest == "SUBTRACT" || rest == "SUB" || rest == "DASH" || rest == "HYPHEN")
            return VK_SUBTRACT;
        if (rest == "." || rest == "DOT" || rest == "DECIMAL" || rest == "PERIOD" || rest == "POINT")
            return VK_DECIMAL;

        // Windows does not expose a distinct VK for Numpad Enter; it's VK_RETURN.
        if (rest == "ENTER" || rest == "RETURN")
            return VK_RETURN;
    }

    // F-keys
    if (!s.empty() && s[0] == 'F')
    {
        int n = 0;
        try
        {
            n = std::stoi(s.substr(1));
        }
        catch (...)
        {
            n = 0;
        }
        if (n >= 1 && n <= 24)
            return VK_F1 + (n - 1);
    }

    // Main-row letters/digits
    if (s.size() == 1 && s[0] >= 'A' && s[0] <= 'Z')
        return static_cast<int>(s[0]);
    if (s.size() == 1 && s[0] >= '0' && s[0] <= '9')
        return static_cast<int>(s[0]);

    // Mouse buttons
    if (s.rfind("MOUSE", 0) == 0 && s.size() == 6 && std::isdigit(static_cast<unsigned char>(s[5])) != 0)
    {
        switch (s[5])
        {
        case '1': return VK_LBUTTON;
        case '2': return VK_RBUTTON;
        case '3': return VK_MBUTTON;
        case '4': return VK_XBUTTON1;
        case '5': return VK_XBUTTON2;
        default: break;
        }
    }

    if (s == "WHEELUP" || s == "MWHEELUP" || s == "MOUSEWHEELUP")
    {
        return VK_WHEELUP;
    }

    if (s == "WHEELDOWN" || s == "MWHEELDOWN" || s == "MOUSEWHEELDOWN")
    {
        return VK_WHEELDOWN;
    }

    if (s == "PADA" || s == "CONTROLLERA" || s == "GAMEPADA") return VK_PAD_A;
    if (s == "PADB" || s == "CONTROLLERB" || s == "GAMEPADB") return VK_PAD_B;
    if (s == "PADX" || s == "CONTROLLERX" || s == "GAMEPADX") return VK_PAD_X;
    if (s == "PADY" || s == "CONTROLLERY" || s == "GAMEPADY") return VK_PAD_Y;
    if (s == "PADLB" || s == "PADL1" || s == "PADLEFTBUMPER") return VK_PAD_LSHOULDER;
    if (s == "PADRB" || s == "PADR1" || s == "PADRIGHTBUMPER") return VK_PAD_RSHOULDER;
    if (s == "PADLT" || s == "PADL2" || s == "PADLEFTTRIGGER") return VK_PAD_LTRIGGER;
    if (s == "PADRT" || s == "PADR2" || s == "PADRIGHTTRIGGER") return VK_PAD_RTRIGGER;
    if (s == "PADBACK" || s == "PADSELECT" || s == "PADVIEW") return VK_PAD_BACK;
    if (s == "PADSTART" || s == "PADMENU") return VK_PAD_START;
    if (s == "PADLSTICK" || s == "PADLS" || s == "PADLEFTSTICK") return VK_PAD_LTHUMB_PRESS;
    if (s == "PADRSTICK" || s == "PADRS" || s == "PADRIGHTSTICK") return VK_PAD_RTHUMB_PRESS;
    if (s == "PADDUP" || s == "PADDPADUP") return VK_PAD_DPAD_UP;
    if (s == "PADDOWN" || s == "PADDPADDOWN") return VK_PAD_DPAD_DOWN;
    if (s == "PADLEFT" || s == "PADDPADLEFT") return VK_PAD_DPAD_LEFT;
    if (s == "PADRIGHT" || s == "PADDPADRIGHT") return VK_PAD_DPAD_RIGHT;
    if (s == "PADLUP" || s == "PADLEFTSTICKUP") return VK_PAD_LTHUMB_UP;
    if (s == "PADLDOWN" || s == "PADLEFTSTICKDOWN") return VK_PAD_LTHUMB_DOWN;
    if (s == "PADLLEFT" || s == "PADLEFTSTICKLEFT") return VK_PAD_LTHUMB_LEFT;
    if (s == "PADLRIGHT" || s == "PADLEFTSTICKRIGHT") return VK_PAD_LTHUMB_RIGHT;
    if (s == "PADRUP" || s == "PADRIGHTSTICKUP") return VK_PAD_RTHUMB_UP;
    if (s == "PADRDOWN" || s == "PADRIGHTSTICKDOWN") return VK_PAD_RTHUMB_DOWN;
    if (s == "PADRLEFT" || s == "PADRIGHTSTICKLEFT") return VK_PAD_RTHUMB_LEFT;
    if (s == "PADRRIGHT" || s == "PADRIGHTSTICKRIGHT") return VK_PAD_RTHUMB_RIGHT;

    // Named keys (main-row/system/modifiers/OEM; explicit word forms only for ambiguous symbols)
    static const std::unordered_map<std::string, int> lut =
    {
        { "INS", VK_INSERT }, { "INSERT", VK_INSERT },
        { "DEL", VK_DELETE }, { "DELETE", VK_DELETE },
        { "HOME", VK_HOME },  { "END", VK_END },
        { "PGUP", VK_PRIOR }, { "PAGEUP", VK_PRIOR },
        { "PGDN", VK_NEXT },  { "PAGEDOWN", VK_NEXT },

        { "LEFT", VK_LEFT }, { "RIGHT", VK_RIGHT },
        { "UP", VK_UP },     { "DOWN", VK_DOWN },

        { "SPACE", VK_SPACE }, { "SPACEBAR", VK_SPACE },
        { "ENTER", VK_RETURN }, { "RETURN", VK_RETURN },
        { "ESC", VK_ESCAPE },   { "ESCAPE", VK_ESCAPE },
        { "TAB", VK_TAB }, { "BACKSPACE", VK_BACK }, { "BKSP", VK_BACK },

        { "SHIFT", VK_SHIFT },   { "LSHIFT", VK_LSHIFT },   { "RSHIFT", VK_RSHIFT },
        { "CTRL", VK_CONTROL },  { "LCTRL", VK_LCONTROL },  { "RCTRL", VK_RCONTROL },
        { "ALT", VK_MENU },      { "LALT", VK_LMENU },      { "RALT", VK_RMENU },
        { "CAPS", VK_CAPITAL },  { "CAPSLOCK", VK_CAPITAL },

        { "PRINTSCREEN", VK_SNAPSHOT }, { "PRTSC", VK_SNAPSHOT }, { "PRTSCR", VK_SNAPSHOT },
        { "SCROLLLOCK", VK_SCROLL }, { "SCROLL", VK_SCROLL },
        { "PAUSE", VK_PAUSE }, { "BREAK", VK_PAUSE },
        { "APPS", VK_APPS }, { "MENU", VK_APPS },
        { "WIN", VK_LWIN }, { "LWIN", VK_LWIN }, { "RWIN", VK_RWIN },

        { "PLUS", VK_OEM_PLUS }, { "MINUS", VK_OEM_MINUS },
        { "SEMICOLON", VK_OEM_1 }, { "QUOTE", VK_OEM_7 }, { "APOSTROPHE", VK_OEM_7 },
        { "COMMA", VK_OEM_COMMA }, { "PERIOD", VK_OEM_PERIOD }, { "DOT", VK_OEM_PERIOD },
        { "SLASH", VK_OEM_2 }, { "BACKSLASH", VK_OEM_5 },
        { "LBRACKET", VK_OEM_4 }, { "RBRACKET", VK_OEM_6 },
        { "TILDE", VK_OEM_3 }, { "GRAVE", VK_OEM_3 },

        // Numpad ops (word forms; also allow "ASTERISK"/"STAR" for multiply)
        { "MULTIPLY", VK_MULTIPLY }, { "ASTERISK", VK_MULTIPLY }, { "STAR", VK_MULTIPLY },
        { "DIVIDE", VK_DIVIDE },
        { "ADD", VK_ADD },
        { "SUBTRACT", VK_SUBTRACT },
        { "DECIMAL", VK_DECIMAL },
    };

    if (auto it = lut.find(s); it != lut.end())
        return it->second;

    return std::nullopt;
}

std::string InputHandler::GetKeyNameFromVK(int vkCode)
{
    switch (vkCode)
    {
    case VK_LBUTTON:  return "Mouse1";
    case VK_RBUTTON:  return "Mouse2";
    case VK_MBUTTON:  return "Mouse3";
    case VK_XBUTTON1: return "Mouse4";
    case VK_XBUTTON2: return "Mouse5";

    case VK_WHEELUP:   return "WheelUp";
    case VK_WHEELDOWN: return "WheelDown";

    case VK_PAD_A: return "Pad_A";
    case VK_PAD_B: return "Pad_B";
    case VK_PAD_X: return "Pad_X";
    case VK_PAD_Y: return "Pad_Y";
    case VK_PAD_LSHOULDER: return "Pad_LB";
    case VK_PAD_RSHOULDER: return "Pad_RB";
    case VK_PAD_BACK: return "Pad_Back";
    case VK_PAD_START: return "Pad_Start";
    case VK_PAD_LTHUMB_PRESS: return "Pad_LStick";
    case VK_PAD_RTHUMB_PRESS: return "Pad_RStick";
    case VK_PAD_DPAD_UP: return "Pad_DPad_Up";
    case VK_PAD_DPAD_DOWN: return "Pad_DPad_Down";
    case VK_PAD_DPAD_LEFT: return "Pad_DPad_Left";
    case VK_PAD_DPAD_RIGHT: return "Pad_DPad_Right";
    case VK_PAD_LTRIGGER: return "Pad_LT";
    case VK_PAD_RTRIGGER: return "Pad_RT";
    case VK_PAD_LTHUMB_UP: return "Pad_LThumb_Up";
    case VK_PAD_LTHUMB_DOWN: return "Pad_LThumb_Down";
    case VK_PAD_LTHUMB_LEFT: return "Pad_LThumb_Left";
    case VK_PAD_LTHUMB_RIGHT: return "Pad_LThumb_Right";
    case VK_PAD_RTHUMB_UP: return "Pad_RThumb_Up";
    case VK_PAD_RTHUMB_DOWN: return "Pad_RThumb_Down";
    case VK_PAD_RTHUMB_LEFT: return "Pad_RThumb_Left";
    case VK_PAD_RTHUMB_RIGHT: return "Pad_RThumb_Right";

    case VK_LMENU:    return "LAlt";
    case VK_RMENU:    return "RAlt";
    case VK_LCONTROL: return "LCtrl";
    case VK_RCONTROL: return "RCtrl";
    case VK_LSHIFT:   return "LShift";
    case VK_RSHIFT:   return "RShift";
    case VK_LWIN:     return "LWin";
    case VK_RWIN:     return "RWin";

    case VK_NUMPAD0:  return "Num0";
    case VK_NUMPAD1:  return "Num1";
    case VK_NUMPAD2:  return "Num2";
    case VK_NUMPAD3:  return "Num3";
    case VK_NUMPAD4:  return "Num4";
    case VK_NUMPAD5:  return "Num5";
    case VK_NUMPAD6:  return "Num6";
    case VK_NUMPAD7:  return "Num7";
    case VK_NUMPAD8:  return "Num8";
    case VK_NUMPAD9:  return "Num9";

    case VK_DECIMAL:  return "NumDecimal";
    case VK_DIVIDE:   return "NumDivide";
    case VK_MULTIPLY: return "NumMultiply";
    case VK_SUBTRACT: return "NumMinus";
    case VK_ADD:      return "NumPlus";
    case VK_NUMLOCK:  return "NumLock";

    case VK_CAPITAL:  return "CapsLock";
    default: break;
    }

    UINT scanCode = MapVirtualKey((UINT)vkCode, MAPVK_VK_TO_VSC);
    switch (vkCode)
    {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT:
    case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE:
        scanCode |= 0x100;
        break;
    default:
        break;
    }

    char keyName[64] = {};
    if (GetKeyNameTextA((LONG)scanCode << 16, keyName, sizeof(keyName)) > 0 && keyName[0] != '\0')
        return std::string(keyName);

    char buf[32] = {};
    std::snprintf(buf, sizeof(buf), "VK 0x%02X", vkCode & 0xFF);
    return std::string { buf };
}

bool InputHandler::KeyAlreadyRegistered(int vkCode, std::string& existingName) const
{
    for (const auto& hk : hotkeys)
    {
        if (hk.vkCode == vkCode)
        {
            existingName = hk.name;
            return true;
        }
    }
    return false;
}

void InputHandler::RegisterHotkey(int vkCode, const char* name, std::function<void()> callback)
{
    std::string existingName;
    if (KeyAlreadyRegistered(vkCode, existingName))
    {
        spdlog::warn("InputHandler: Key '{}' is already bound to '{}'. Overwriting with '{}'.",
            GetKeyNameFromVK(vkCode), existingName, name);
    }

    spdlog::info("InputHandler: Registering '{}' on key '{}'.", name, GetKeyNameFromVK(vkCode));

    hotkeys.push_back({ vkCode, name, std::move(callback), false });

    HeldHotkeyState heldState = {};
    heldState.repeatWhileHeld = false;
    heldState.repeatDelayMs = 0;
    heldState.nextRepeatTick = 0;
    g_HeldHotkeyStates.push_back(heldState);
}

void InputHandler::RegisterHeldHotkey(int vkCode, const char* name, std::function<void()> callback, DWORD repeatDelayMs)
{
    std::string existingName;
    if (KeyAlreadyRegistered(vkCode, existingName))
    {
        spdlog::warn("InputHandler: Key '{}' is already bound to '{}'. Overwriting with held '{}'.",
            GetKeyNameFromVK(vkCode), existingName, name);
    }

    if (repeatDelayMs == 0)
    {
        repeatDelayMs = kDefaultHeldHotkeyRepeatDelayMs;
    }

    spdlog::info("InputHandler: Registering held '{}' on key '{}' with {}ms repeat delay.",
        name, GetKeyNameFromVK(vkCode), repeatDelayMs);

    hotkeys.push_back({ vkCode, name, std::move(callback), false });

    HeldHotkeyState heldState = {};
    heldState.repeatWhileHeld = true;
    heldState.repeatDelayMs = repeatDelayMs;
    heldState.nextRepeatTick = 0;
    g_HeldHotkeyStates.push_back(heldState);
}

void InputHandler::Update()
{

    if (!g_InputHandler.bCaptureInputsWhileAltTabbed && (GetForegroundWindow() != g_D3D11Hooks.MainHwnd))
    {
        return;
    }

    InstallMouseWheelCapture(g_D3D11Hooks.MainHwnd);
    EnsureHeldHotkeyStateCount(hotkeys.size());

    const ULONGLONG currentTick = GetTickCount64();

    for (size_t i = 0; i < hotkeys.size(); ++i)
    {
        auto& hk = hotkeys[i];
        HeldHotkeyState& heldState = g_HeldHotkeyStates[i];

        bool keyDown = false;

        if (hk.vkCode == VK_WHEELUP)
        {
            if (g_WheelUpPressed && hk.onPress)
            {
                hk.onPress();
            }

            hk.prevState = false;
            heldState.nextRepeatTick = 0;
            continue;
        }

        if (hk.vkCode == VK_WHEELDOWN)
        {
            if (g_WheelDownPressed && hk.onPress)
            {
                hk.onPress();
            }

            hk.prevState = false;
            heldState.nextRepeatTick = 0;
            continue;
        }

        if (IsGamepadInput(hk.vkCode))
        {
            keyDown = IsSteamInputGamepadInputDown(hk.vkCode);
        }
        else
        {
            keyDown = (GetAsyncKeyState(hk.vkCode) & 0x8000) != 0;
        }

        if (!keyDown)
        {
            hk.prevState = false;
            heldState.nextRepeatTick = 0;
            continue;
        }

        if (!hk.prevState)
        {
            if (hk.onPress)
            {
                hk.onPress();
            }

            hk.prevState = true;

            if (heldState.repeatWhileHeld)
            {
                heldState.nextRepeatTick = currentTick + heldState.repeatDelayMs;
            }

            continue;
        }

        if (!heldState.repeatWhileHeld)
        {
            continue;
        }

        if (currentTick < heldState.nextRepeatTick)
        {
            continue;
        }

        if (hk.onPress)
        {
            hk.onPress();
        }

        heldState.nextRepeatTick = currentTick + heldState.repeatDelayMs;
    }

    g_WheelUpPressed = false;
    g_WheelDownPressed = false;
}

void InputHandler::GetKeybind(const inipp::Ini<char>& ini,
    const std::string& section,
    const std::string& key,
    int& outVk)
{
    std::string alias;

    const auto secIt = ini.sections.find(section);
    if (secIt == ini.sections.end())
    {
        FatalKeyError(section, key, "Section not found");
        return;
    }

    const auto& kv = secIt->second;
    const auto keyIt = kv.find(key);
    if (keyIt == kv.end())
    {
        FatalKeyError(section, key, "Key not found");
        return;
    }

    alias = Util::StripQuotes(keyIt->second);
    if (alias.empty())
    {
        FatalKeyError(section, key, "Empty value");
        return;
    }

    const auto vkOpt = ParseVirtualKey(alias);
    if (!vkOpt.has_value())
    {
        FatalKeyError(section, key, "Invalid key alias '" + alias + "'");
        return;
    }

    const int vk = *vkOpt;
    const std::string nice = GetKeyNameFromVK(vk);

    if (const auto found = s_UsedKeybinds.find(vk); found != s_UsedKeybinds.end())
    {
        spdlog::warn("Config Parse: [{}] '{}' ({}) duplicates [{}]",
            section, key, nice, found->second);
    }
    else
    {
        s_UsedKeybinds[vk] = section + ":" + key;
    }

    outVk = vk;

    spdlog::info("Config Parse: [{}] '{}' resolved to '{}'",
        section, key, nice);
}
