#include "stdafx.h"
#include "windows_preferred_gpu.hpp"

#include "common.hpp"
#include "logging.hpp"

void HighPerformanceGpu::Fix()
{
    if (Util::IsSteamOS() || !(eGameType & (MG | MGS2 | MGS3)))
    {
        return;
    }

    const auto markerFile = sGameSavePath / "MGSHDFix_windows_preferred_gpu.bin";
    const bool shouldApply = bEnabled;
    const bool markerExists = std::filesystem::exists(markerFile);
    const bool shouldRemove = !shouldApply && markerExists;
    if (!shouldApply && !shouldRemove)
    {
        spdlog::info("[Registry Compat Fix] High-performance GPU registry fix not required for {}", (sExePath / sExeName).string());
        return;
    }

    spdlog::info("[Registry Compat Fix] {} high-performance GPU registry fix for {}", shouldApply ? "Applying" : "Reverting", (sExePath / sExeName).string());

    HKEY hKey;
    const char* subKey = R"(Software\Microsoft\DirectX\UserGpuPreferences)";
    LONG result = RegCreateKeyExA(
        HKEY_CURRENT_USER,
        subKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_READ | KEY_WRITE,
        nullptr,
        &hKey,
        nullptr);

    if (result != ERROR_SUCCESS)
    {
        spdlog::error("[Registry Compat Fix] Failed to open registry key: {}", subKey);
        return;
    }

    DWORD type = 0;
    DWORD dataSize = 0;
    result = RegQueryValueExA(hKey, (sExePath / sExeName).string().c_str(), nullptr, &type, nullptr, &dataSize);

    std::string value;
    if (result == ERROR_SUCCESS && dataSize > 0)
    {
        std::vector<char> data(dataSize);
        if (RegQueryValueExA(hKey, (sExePath / sExeName).string().c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(data.data()), &dataSize) == ERROR_SUCCESS)
        {
            value.assign(data.begin(), data.end());
            while (!value.empty() && value.back() == '\0')
                value.pop_back();
        }
    }

    bool modified = false;

    if (shouldApply)
    {
        if (value != "GpuPreference=2;")
        {
            value = "GpuPreference=2;";
            modified = true;
        }
    }
    else if (shouldRemove)
    {
        if (!value.empty())
        {
            value.clear();
            modified = true;
        }
    }

    if (modified)
    {
        if (value.empty())
        {
            if (RegDeleteValueA(hKey, (sExePath / sExeName).string().c_str()) == ERROR_SUCCESS)
                spdlog::info("[Registry Compat Fix] Deleted registry entry for {}", (sExePath / sExeName).string());
            else
                spdlog::error("[Registry Compat Fix] Failed to delete registry entry for {}", (sExePath / sExeName).string());
        }
        else
        {
            const DWORD valueSize = static_cast<DWORD>(value.size() + 1);
            if (RegSetValueExA(hKey, (sExePath / sExeName).string().c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), valueSize) == ERROR_SUCCESS)
                spdlog::info("[Registry Compat Fix] Wrote registry entry for {}: {}", (sExePath / sExeName).string(), value);
            else
                spdlog::error("[Registry Compat Fix] Failed to write registry entry for {}", (sExePath / sExeName).string());
        }
    }
    else
    {
        spdlog::info("[Registry Compat Fix] No registry changes required for {}", (sExePath / sExeName).string());
    }

    RegCloseKey(hKey);

    if (shouldApply)
    {
        if (!markerExists)
        {
            try
            {
                std::ofstream out(markerFile, std::ios::trunc);
                if (out)
                {
                    out << "  ...A surveillance camera?!\n";
                    out << "MGSHDFix wrote this file to track high-performance GPU registry state.\n";
                    out << "Delete this file to prevent the fix from reverting when disabled.\n";
                    out.close();
                    spdlog::info("[Registry Compat Fix] Created marker file: {}", markerFile.string());
                }
            }
            catch (const std::exception& e)
            {
                spdlog::error("[Registry Compat Fix] Failed to create marker file: {} - {}", markerFile.string(), e.what());
            }
        }
    }
    else if (shouldRemove)
    {
        std::error_code ec;
        std::filesystem::remove(markerFile, ec);
        if (!ec)
            spdlog::info("[Registry Compat Fix] Removed marker file: {}", markerFile.string());
        else
            spdlog::warn("[Registry Compat Fix] Failed to remove marker file: {}", markerFile.string());
    }
}

