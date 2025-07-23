#include "common.hpp"
#include "version_checking.hpp"

#include "logging.hpp"

void CheckForUpdates()
{
    if (!bShouldCheckForUpdates)
    {
        spdlog::info("Mod update checking disabled via config.");
        return;
    }
    std::filesystem::path cacheFilePath = sGameSavePath / (sFixName + "_version_check.txt");
    LatestVersionChecker checker(cacheFilePath);
    checker.checkForUpdates();
}

LatestVersionChecker::LatestVersionChecker(const std::filesystem::path& cacheFile)
    : m_cacheFile(cacheFile)
{
}

#if !defined(PRIMARY_REPO_URL) && !defined(FALLBACK_REPO_URL)


bool LatestVersionChecker::checkForUpdates()
{
    spdlog::error("Update Checker called but no repository URLs are defined (PRIMARY_REPO_URL and FALLBACK_REPO_URL missing).");
    throw std::invalid_argument("Update Checker called but no repository URLs are defined (PRIMARY_REPO_URL and FALLBACK_REPO_URL missing).");
}

#else // At least one repo URL is defined

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

inline std::string to_lower(std::string str)
{
    std::ranges::transform(str, str.begin(),
        [](unsigned char c)
        {
            return std::tolower(c);
        });
    return str;
}

bool LatestVersionChecker::checkForUpdates()
{
    std::string cachedLatest, warnedVersion;
    bool cacheIsFresh = false;

    bool didCheck = false;

    if (!loadCache(cachedLatest, warnedVersion, cacheIsFresh))
    {
#if defined(PRIMARY_REPO_URL)
        RepoInfo repoInfo = parseRepoUrl(PRIMARY_REPO_URL);
        spdlog::info("Version Check: No cache found. Contacting {} API for latest version.", repoInfo.displayName);

        if (queryLatestVersion(repoInfo, cachedLatest))
        {
            didCheck = true;
        }
        else
#endif
#if defined(FALLBACK_REPO_URL)
        {
            RepoInfo repoInfo = parseRepoUrl(FALLBACK_REPO_URL);
            spdlog::info("Version Check: Primary API failed or missing. Trying fallback {}.", repoInfo.displayName);

            if (queryLatestVersion(repoInfo, cachedLatest))
            {
                didCheck = true;
            }
        }
#endif
        if (!didCheck)
        {
            spdlog::info("Version Check: Unable to contact Repo API. Skipping version check.");
            return false;
        }

        saveCache(cachedLatest, "");
    }
    else if (!cacheIsFresh)
    {
#if defined(PRIMARY_REPO_URL)
        RepoInfo primaryRepoInfo = parseRepoUrl(PRIMARY_REPO_URL);
        spdlog::info("Version Check: Cache stale. Refreshing from {} API.", primaryRepoInfo.displayName);

        std::string latestFromApi;
        if (queryLatestVersion(primaryRepoInfo, latestFromApi))
        {
            cachedLatest = latestFromApi;
            saveCache(cachedLatest, warnedVersion);
            didCheck = true;
        }
#endif

#if defined(FALLBACK_REPO_URL)
        // If primary didn't check or failed, try fallback
        if (!didCheck)
        {
            RepoInfo fallbackRepoInfo = parseRepoUrl(FALLBACK_REPO_URL);
            spdlog::info("Version Check: Primary API failed or missing on stale cache. Trying fallback {}.", fallbackRepoInfo.displayName);

            std::string latestFromFallback;
            if (queryLatestVersion(fallbackRepoInfo, latestFromFallback))
            {
                cachedLatest = latestFromFallback;
                saveCache(cachedLatest, warnedVersion);
                didCheck = true;
            }
        }
#endif

        if (!didCheck)
        {
            spdlog::info("Version Check: Unable to contact Repo API on stale cache. Skipping version check.");
            return false;
        }
    }
    else
    {
        spdlog::info("Version Check: Under {} hours since last update check. Skipping update check.", iCacheTTLHours);
    }

    int cmp = compareSemVer(VERSION_STRING, cachedLatest);

    if (cmp < 0)
    {
        spdlog::warn("Version Check: A new version of {} is available.", FIX_NAME);
        spdlog::warn("Version Check - Current Version: {}, Latest Version: {}", VERSION_STRING, cachedLatest);

        if (warnedVersion != cachedLatest)
        {
            if (bConsoleUpdateNotifications)
            {
                Logging::ShowConsole();
                std::cout << FIX_NAME << " Update Notice: New version of " << FIX_NAME << " is available.\nCurrent Version: "
                    << VERSION_STRING << ", Latest Version: " << cachedLatest << std::endl;
            }
            saveCache(cachedLatest, cachedLatest);
            return true;
        }
        return false;
    }
    else if (cmp > 0)
    {
        spdlog::info("Version Check: Welcome back, Commander! You're running a development build of {}!", FIX_NAME);
        spdlog::info("Version Check - Current Version: {}, Latest Release: {}", VERSION_STRING, cachedLatest);
        return false;
    }

    spdlog::info("Version Check: {} is up to date.", FIX_NAME);
    return false;
}

bool LatestVersionChecker::loadCache(std::string& cachedLatest, std::string& warnedVersion, bool& cacheIsFresh)
{
    std::ifstream file(m_cacheFile);
    if (!file) return false;

    std::string versionLine, timeLine;
    if (!std::getline(file, versionLine) || !std::getline(file, timeLine))
        return false;

    cachedLatest = versionLine;

    std::getline(file, warnedVersion);

    auto cachedTime = parseISO8601(timeLine);
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - cachedTime);
    cacheIsFresh = (age.count() <= iCacheTTLHours);

    return true;
}

void LatestVersionChecker::saveCache(const std::string& latestVersion, const std::string& warnedVersion)
{
    std::ofstream file(m_cacheFile);
    if (!file) return;

    file << latestVersion << "\n";
    file << currentTimeISO8601() << "\n";
    file << warnedVersion << "\n";
}

std::wstring LatestVersionChecker::buildUserAgent() const
{
    std::string ua = std::string(FIX_NAME) + "/" + VERSION_STRING;
    return std::wstring(ua.begin(), ua.end());
}

LatestVersionChecker::RepoInfo LatestVersionChecker::parseRepoUrl(const std::string& url) const
{
    std::regex re(R"(https://([^/]+)/([^/]+)/([^/]+))");
    std::smatch m;

    if (!std::regex_match(url, m, re))
    {
        spdlog::error("Version Check: Invalid repository URL format: {}", url);
        throw std::invalid_argument("Invalid repository URL: " + url);
    }


    std::string host = m[1];
    std::string owner = m[2];
    std::string repo = m[3];

    RepoInfo info;

    if (host == "github.com")
    {
        info.displayName = "GitHub.com";
        info.apiHost = L"api.github.com";
        info.apiPath = L"/repos/" + std::wstring(owner.begin(), owner.end()) +
            L"/" + std::wstring(repo.begin(), repo.end()) + L"/releases/latest";
    }
    else if (host == "codeberg.org")
    {
        info.displayName = "Codeberg.org";
        info.apiHost = L"codeberg.org";
        info.apiPath = L"/api/v1/repos/" + std::wstring(owner.begin(), owner.end()) +
            L"/" + std::wstring(repo.begin(), repo.end()) + L"/releases/latest";
    }
    else if (host == "gitlab.com")
    {
        info.displayName = "GitLab.com";
        info.apiHost = L"gitlab.com";
        info.apiPath = L"/api/v4/projects/" +
            std::wstring(owner.begin(), owner.end()) + L"%2F" +
            std::wstring(repo.begin(), repo.end()) + L"/releases";
    }
    else
    {
        spdlog::error("Version Check: Unsupported host: {}", host);
        throw std::invalid_argument("Unsupported host: " + host);
    }

    return info;
}

bool LatestVersionChecker::queryLatestVersion(const RepoInfo& repoInfo, std::string& latestVersion)
{
    spdlog::info("Version Check: Contacting {} API...", repoInfo.displayName);

    HINTERNET hSession = WinHttpOpen(
        buildUserAgent().c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        nullptr,
        nullptr,
        0);

    if (!hSession)
    {
        spdlog::error("Version Check: WinHttpOpen failed with error: {}", GetLastError());
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        repoInfo.apiHost.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0);

    if (!hConnect)
    {
        spdlog::error("Version Check: WinHttpConnect failed with error: {}", GetLastError());
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        repoInfo.apiPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest)
    {
        spdlog::error("Version Check: WinHttpOpenRequest failed with error: {}", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring userAgentHeader = L"User-Agent: " + buildUserAgent();
    if (!WinHttpAddRequestHeaders(hRequest, userAgentHeader.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD))
    {
        spdlog::error("Version Check: WinHttpAddRequestHeaders failed with error: {}", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::string response;

    if (WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr))
    {
        DWORD size = 0;
        do
        {
            DWORD downloaded = 0;
            WinHttpQueryDataAvailable(hRequest, &size);
            if (!size) break;

            std::string buffer(size, 0);
            WinHttpReadData(hRequest, &buffer[0], size, &downloaded);
            response.append(buffer, 0, downloaded);
        } while (size > 0);
    }
    else
    {
        spdlog::error("Version Check: WinHttpSendRequest or WinHttpReceiveResponse failed with error: {}", GetLastError());
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    std::smatch m;
    std::regex re(R"delim("tag_name"\s*:\s*"\s*v?([^"]+)")delim");
    if (std::regex_search(response, m, re) && m.size() > 1)
    {
        latestVersion = m[1];
        return true;
    }

    return false;
}

int LatestVersionChecker::compareSemVer(const std::string& a, const std::string& b)
{
    std::istringstream sa(a), sb(b);
    int va[3] = { 0 }, vb[3] = { 0 };
    char dot;
    sa >> va[0] >> dot >> va[1] >> dot >> va[2];
    sb >> vb[0] >> dot >> vb[1] >> dot >> vb[2];

    for (int i = 0; i < 3; ++i)
    {
        if (va[i] < vb[i]) return -1;
        if (va[i] > vb[i]) return 1;
    }
    return 0;
}

std::string LatestVersionChecker::currentTimeISO8601()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point LatestVersionChecker::parseISO8601(const std::string& timeStr)
{
    std::tm tm = {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

#endif // Repo URL defined check
