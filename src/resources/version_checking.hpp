#pragma once
#include "version.h"

#include <string>
#include <chrono>
#include <filesystem>

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

    static int compareSemVer(const std::string& a, const std::string& b);
    static std::string currentTimeISO8601();
    static std::chrono::system_clock::time_point parseISO8601(const std::string& timeStr);
    static constexpr int iCacheTTLHours = 24; // Cache TTL in hours

#endif
};
