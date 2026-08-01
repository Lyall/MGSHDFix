#pragma once
#include <filesystem>
#include <functional>
#include <string>

namespace Helper
{
    std::filesystem::path FindASILocation(std::string fileName);

    // Runs fn on the main GUI thread now if already on it, otherwise marshals it there.
    void RunOnMainThread(std::function<void()> fn);


    enum class VersionCompareResult
    {
        Older = -1,
        Equal = 0,
        Newer = 1
    };

    // Compare two semantic-style version strings (e.g. "32.0.15.8130" vs "32.0.15.9000")
    VersionCompareResult CompareSemanticVersion(const std::string& currentVersion, const std::string& targetVersion);

    std::string GetFileDescription(const std::string& filePath);

    [[nodiscard]] bool IsSteamOS();

}
