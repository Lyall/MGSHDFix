#include "common.hpp"
#include "reshade_compatibility_checks.hpp"

#include "config.hpp"
#include "logging.hpp"

void ReshadeCompatibility::Check()
{
    if (!std::filesystem::exists(sExePath / "dxgi.dll") || Util::GetFileDescription((sExePath / "dxgi.dll").string()) != "ReShade")
    {
        return;
    }

    spdlog::info("Checking for outdated versions of Reshade (which are known to cause crashing.)");
    // Retrieve the version information of dxgi.dll
    DWORD handle = 0;
    DWORD versionInfoSize = GetFileVersionInfoSize((sExePath / "dxgi.dll").wstring().c_str(), &handle);
    if (versionInfoSize == 0)
    {
        return;
    }

    std::vector<BYTE> versionInfo(versionInfoSize);
    if (!GetFileVersionInfo((sExePath / "dxgi.dll").wstring().c_str(), handle, versionInfoSize, versionInfo.data()))
    {
        return;
    }

    VS_FIXEDFILEINFO* fileInfo = nullptr;
    UINT fileInfoSize = 0;
    if (!VerQueryValue(versionInfo.data(), L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoSize) || !fileInfo)
    {
        return;
    }

    // Extract the version numbers
    DWORD major = HIWORD(fileInfo->dwFileVersionMS);
    DWORD minor = LOWORD(fileInfo->dwFileVersionMS);
    DWORD build = HIWORD(fileInfo->dwFileVersionLS);
    DWORD revision = LOWORD(fileInfo->dwFileVersionLS);

    // Check if the version is lower than 6.5.0.2000
    if (major > 6 || (major == 6 && minor > 5) ||
        (major == 6 && minor == 5 && build > 0) ||
        (major == 6 && minor == 5 && build == 0 && revision >= 2000))
    {
        return;
    }
    bOutdatedReshade = true;

    if (eGameType & LAUNCHER && !bLauncherConfigSkipLauncher)
    {
        bLauncherConfigSkipLauncher = TRUE;
        return;
    }

    spdlog::warn("------ RESHADE COMPATIBILITY WARNING ------");
    spdlog::warn("An outdated version of ReShade (dxgi.dll) is currently installed - (version v{}.{}.{}.{} found.)", major, minor, build, revision);
    spdlog::warn("Versions prior to v6.5.0 (released 30MAY2025) are known to cause intermittent crashing with MGS Master Collection games.");
    spdlog::warn("Please update to the latest version.");
    spdlog::warn("------ RESHADE COMPATIBILITY WARNING ------");

    Logging::ShowConsole();
    std::cout << "RESHADE COMPATIBILITY WARNING\n"
        "An outdated version of ReShade (dxgi.dll) is currently installed - (version v" << major << "." << minor << "." << build << "." << revision << " found.)\n"
        "Versions prior to v6.5.0 (released 30MAY2025) are known to cause intermittent crashing with MGS Master Collection games.\n"
        "Please update to the latest version.\n";
}
