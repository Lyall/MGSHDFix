#include "common.hpp"
#include "input_handler.hpp"
#include "logging.hpp"

#include "version.h"

std::unordered_map<int, std::string> InputHandler::s_UsedKeybinds {};

void InputHandler::FatalKeyError(const std::string& section,
    const std::string& key,
    const std::string& reason)
{
    const std::string message =
        "[" + sFixName + " Config Helper] Failed to read config key '" + key +
        "' in section '" + section + "': " + reason;

    spdlog::error(message);
    spdlog::error("Please check that you're using the latest version's config file, and that there are no typos in it.");
    Logging::ShowConsole();
    std::cout << message << std::endl;
    std::cout << "Please check that you're using the latest version's config file, and that there are no typos in it." << std::endl;

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

    // Normalize keypad prefixes: KP*, KEYPAD*, NP* -> NUMPAD*
    if (s.rfind("KP", 0) == 0)
        s = "NUMPAD" + s.substr(2);
    else if (s.rfind("KEYPAD", 0) == 0)
        s = "NUMPAD" + s.substr(6);
    else if (s.rfind("NP", 0) == 0)
        s = "NUMPAD" + s.substr(2);

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
    default: break;
    }

    UINT scanCode = MapVirtualKey((UINT)vkCode, MAPVK_VK_TO_VSC);
    switch (vkCode)
    {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
    case VK_PRIOR: case VK_NEXT:
    case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE:
    case VK_DIVIDE:
    case VK_NUMLOCK:
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
}

void InputHandler::Update()
{
    for (auto& hk : hotkeys)
    {
        const bool keyDown = (GetAsyncKeyState(hk.vkCode) & 0x8000) != 0;

        if (keyDown && !hk.prevState)
        {
            if (hk.onPress)
                hk.onPress();
        }

        hk.prevState = keyDown;
    }
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

    alias = keyIt->second;
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
