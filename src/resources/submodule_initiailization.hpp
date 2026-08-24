#pragma once
#include "stdafx.h"
#include <chrono>
#include <string>
#include "logging.hpp"

/// Stores the last loaded system string
inline std::string lastLoaded;

/// Modern 'Initialize' implementation that accepts a descriptive name and a callable
template <typename Func>
void Initialize(const char* name, Func&& func)
{
    using namespace std::chrono;

    std::string funcName = name;
    time_point<high_resolution_clock> currentInitPhaseStartTime;

    if (funcName == "InitializeSubsystems()")
    {
        spdlog::info("---------- Subsystem initialization started ----------");
        currentInitPhaseStartTime = g_Logging.initStartTime;
    }
    else if (!lastLoaded.empty())
    {
        spdlog::info("---------- {}\tNow loading: {} ----------", lastLoaded, funcName);
        currentInitPhaseStartTime = high_resolution_clock::now();
    }
    else
    {
        spdlog::info("---------- Loading: {} ----------", funcName);
        currentInitPhaseStartTime = high_resolution_clock::now();
    }

    std::forward<Func>(func)();

    auto duration = duration_cast<milliseconds>(
        high_resolution_clock::now() - currentInitPhaseStartTime).count();

    if (funcName == "InitializeSubsystems()")
    {
        if (!lastLoaded.empty())
        {
            spdlog::info("---------- {} ----------", lastLoaded);
        }
        spdlog::info("---------- All initial systems completed loading in: {} ms. ----------", duration);
    }
    else
    {
        lastLoaded = funcName + " loaded in: " + std::to_string(duration) + " ms.";
    }
}

#define INITIALIZE(expr) Initialize(#expr, [&] { expr; })


template <typename Func>
void InitializeDLL(const char* name, Func&& func)
{
    using namespace std::chrono;

    spdlog::info("---------- DLL Callback: {} ----------", name);
    auto start = high_resolution_clock::now();

    std::forward<Func>(func)();

    auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();

    spdlog::info("---------- {} loaded in: {} ms. ----------", name, duration);
}

#define INITIALIZEDLL(expr) InitializeDLL(#expr, [&] { expr; })
