#pragma once
#include <string>
#include <chrono>
#include <filesystem>
#include "version.h"

namespace VersionCheck
{

    enum class VersionType : std::uint8_t
    {
        File,
        Product
    };
    std::string GetModuleVersion(HMODULE module, VersionType type, bool fourDigit);

    std::string GetFileVersion(const std::filesystem::path& filePath, VersionType type = VersionType::File, bool fourDigit = false);

    enum class CompareResult  // NOLINT(performance-enum-size)
    {
        Older = -1,
        Equal = 0,
        Newer = 1
    };

    // Compare two semantic-style version strings (e.g. "32.0.15.8130" vs "32.0.15.9000")
    CompareResult CompareSemanticVersion(const std::string& currentVersion, const std::string& targetVersion);

}

class LatestVersionChecker
{
public:
    explicit LatestVersionChecker(const std::filesystem::path& cacheFile = "version_cache.txt");

    bool checkForUpdates();
private:
    std::filesystem::path m_cacheFile;

#if defined(PRIMARY_REPO_URL) || defined(FALLBACK_REPO_URL)

    struct RepoInfo
    {
        std::wstring apiHost;
        std::wstring apiPath;
        std::string displayName;
    };

    bool loadCache(std::string& cachedLatest, std::string& warnedVersion, bool& cacheIsFresh);
    void saveCache(const std::string& latestVersion, const std::string& warnedVersion);
    bool queryLatestVersion(const RepoInfo& repoInfo, std::string& latestVersion);
    std::wstring buildUserAgent() const;
    RepoInfo parseRepoUrl(const std::string& url) const;

    static std::string currentTimeISO8601();
    static std::chrono::system_clock::time_point parseISO8601(const std::string& timeStr);
    static constexpr int iCacheTTLHours = 24; // Cache TTL in hours

#endif
};

inline bool bShouldCheckForUpdates;
inline bool bConsoleUpdateNotifications;
inline bool bNewUpdateAvailable = false;
inline bool bDebugBuild = false;

void CheckForUpdates();
