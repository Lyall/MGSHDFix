#pragma once
#include <spdlog/spdlog.h>
#include <chrono>

class Logging final
{
private:
    static std::string GetSteamOSVersion();
    bool bConsoleShown = false;
public:
    static void ShowConsole();
    static void Initialize();
    static void LogSysInfo();

    std::chrono::time_point<std::chrono::high_resolution_clock> initStartTime;
    bool bLoaded = false;
    bool bIsSteamDeck = false;
    bool bCheckedSteamDeck = false;
};

/// Global logging instance
inline Logging g_Logging;
