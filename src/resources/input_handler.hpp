#pragma once
#include <functional>

#include <inipp/inipp.h>

class InputHandler
{
public:
    struct Hotkey
    {
        int vkCode;
        std::string name;
        std::function<void()> onPress;
        bool prevState = false;
    };

    void RegisterHotkey(int vkCode, const char* name, std::function<void()> callback);
    void Update();

    // Reads an INI value, validates & converts aliases to a VK, warns on duplicates, logs resolved name.
    // Fatal-exits on invalid/missing config values (same UX as your ConfigHelper).
    static void GetKeybind(const inipp::Ini<char>& ini,
        const std::string& section,
        const std::string& key,
        int& outVk);

    // Returns a human-friendly name for a VK (prefers OS names; falls back to "VK 0xNN").
    static std::string GetKeyNameFromVK(int vkCode);

    bool bCaptureInputsWhileAltTabbed = false;

private:
    std::vector<Hotkey> hotkeys;

    bool KeyAlreadyRegistered(int vkCode, std::string& existingName) const;

    // --- Keybind helpers ---
    // Normalizes user aliases: trims, uppercases, and removes spaces/underscores/hyphens.
    static std::string NormalizeKeyAlias(std::string s);

    // Parses "0xNN"/"0xNNNN" hex VKs.
    static bool TryParseHexVK(const std::string& s, int& outVk);

    // Alias -> VK resolver. Supports INS/VK_INSERT/0x2D/F-keys/A..Z/0..9/NUMPADx/MouseX,
    // unambiguous single-char '*' (numpad multiply), and word forms (ASTERISK, SUBTRACT, etc.).
    static std::optional<int> ParseVirtualKey(const std::string& aliasRaw);

    // Logs error, shows console, and exits the thread (matches your fatal config behavior).
    static void FatalKeyError(const std::string& section,
        const std::string& key,
        const std::string& reason);

    // Tracks duplicates across GetKeybind calls: vk -> "Section:Key".
    static std::unordered_map<int, std::string> s_UsedKeybinds;
};

inline InputHandler g_InputHandler;
