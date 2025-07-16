#include "common.hpp"
#include "version_checking.hpp"

#include <windows.h>
#include <winhttp.h>

#include <iostream>
#include <fstream>
#include <regex>
#include <iomanip>
#include <sstream>

#include "logging.hpp"
#include "version.h"

#if (defined(REPO_GITHUB) + defined(REPO_CODEBERG) + defined(REPO_GITLAB)) > 1
#error "Only one of REPO_GITHUB, REPO_CODEBERG, or REPO_GITLAB may be defined at a time."
#elif !(defined(REPO_GITHUB) || defined(REPO_CODEBERG) || defined(REPO_GITLAB))
#error "You must define one of REPO_GITHUB, REPO_CODEBERG, or REPO_GITLAB."
#endif

#pragma comment(lib, "winhttp.lib")

extern bool bConsoleNotifications;

LatestVersionChecker::LatestVersionChecker(const std::string& dllVersion,
    const std::string& repoOwner,
    const std::string& repoName,
    const std::filesystem::path& cacheFile,
    int cacheTTLHours)
    : m_dllVersion(dllVersion),
    m_repoOwner(repoOwner),
    m_repoName(repoName),
    m_cacheFile(cacheFile),
    m_cacheTTLHours(cacheTTLHours)
{
}

bool LatestVersionChecker::checkForUpdates()
{
    // Determine proper-cased apiHostStr for logging
    const std::string apiHostStr =
#ifdef REPO_GITHUB
        "GitHub.com";
#elif defined(REPO_CODEBERG)
        "Codeberg.org";
#elif defined(REPO_GITLAB)
        "GitLab.com";
#else
#error "No repository API defined (REPO_GITHUB / REPO_CODEBERG / REPO_GITLAB)"
#endif

        std::string cachedLatest, warnedVersion;
    bool cacheIsFresh = false;

    if (!loadCache(cachedLatest, warnedVersion, cacheIsFresh))
    {
        // No cache at all -> query API
        spdlog::info("Version Check: No cache found. Contacting {} API for latest version.", apiHostStr);

        if (!queryGitHubLatestVersion(cachedLatest))
        {
            spdlog::info("Version Check: Unable to contact {} API. Skipping version check.", apiHostStr);
            return false;
        }

        saveCache(cachedLatest, "");
    }
    else if (!cacheIsFresh)
    {
        spdlog::info("Version Check: Cache stale. Refreshing from {} API.", apiHostStr);
        std::string latestFromApi;
        if (queryGitHubLatestVersion(latestFromApi))
        {
            cachedLatest = latestFromApi;
            saveCache(cachedLatest, warnedVersion);
        }
    }
    else
    {
        spdlog::info("Version Check: Under {} hours since last update check. Skipping {} API call.", m_cacheTTLHours, apiHostStr);
    }

    int cmp = compareSemVer(m_dllVersion, cachedLatest);

    if (cmp < 0)
    {
        spdlog::warn("Version Check: A new version of {} is available from {}.", sFixName, apiHostStr);
        spdlog::warn("Version Check - Current Version: {}, Latest Release: {}", m_dllVersion, cachedLatest);

        if (warnedVersion != cachedLatest)
        {
            if (bConsoleNotifications)
            {
                Logging::ShowConsole();
                std::cout << sFixName << " Update Notice: A new version of " << sFixName << " is available for download.\nCurrent Version: "
                    << m_dllVersion << ", Latest Version: " << cachedLatest << std::endl;
            }
            saveCache(cachedLatest, cachedLatest);
            return true;
        }
        return false;
    }
    else if (cmp > 0)
    {
        spdlog::info("Version Check: Welcome back, Commander! You're running a development build of {}!", sFixName);
        spdlog::info("Version Check - Current Version: {}, Latest Release: {}", m_dllVersion, cachedLatest);
        return false;
    }

    spdlog::info("Version Check: {} is up to date ({}).", sFixName, apiHostStr);
    spdlog::info("Version Check - Current Version: {}", m_dllVersion);
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

    std::getline(file, warnedVersion);  // may be empty

    auto cachedTime = parseISO8601(timeLine);
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - cachedTime);
    cacheIsFresh = (age.count() <= m_cacheTTLHours);

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
    std::string ua = sFixName + "/";
    ua += VERSION_STRING;
    return std::wstring(ua.begin(), ua.end());
}

bool LatestVersionChecker::queryGitHubLatestVersion(std::string& latestVersion)
{
    // Determine lowercase apiHost for HTTP calls
    const std::wstring apiHost =
#ifdef REPO_GITHUB
        L"api.github.com";
#elif defined(REPO_CODEBERG)
        L"codeberg.org";
#elif defined(REPO_GITLAB)
        L"gitlab.com";
#else
#error "No repository API defined (REPO_GITHUB / REPO_CODEBERG / REPO_GITLAB)"
#endif

        // Compose path according to repo
        std::wstring path;
#ifdef REPO_GITHUB
    path = L"/repos/" + std::wstring(m_repoOwner.begin(), m_repoOwner.end()) + L"/" +
        std::wstring(m_repoName.begin(), m_repoName.end()) + L"/releases/latest";
#elif defined(REPO_CODEBERG)
    path = L"/api/v1/repos/" + std::wstring(m_repoOwner.begin(), m_repoOwner.end()) + L"/" +
        std::wstring(m_repoName.begin(), m_repoName.end()) + L"/releases/latest";
#elif defined(REPO_GITLAB)
    path = L"/api/v4/projects/" + std::wstring(m_repoOwner.begin(), m_repoOwner.end()) + L"%2F" +
        std::wstring(m_repoName.begin(), m_repoName.end()) + L"/releases";
#endif

    spdlog::info("Version Check: Contacting {} API for latest version...", std::string(apiHost.begin(), apiHost.end()));

    HINTERNET hSession = WinHttpOpen(
        buildUserAgent().c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        nullptr,
        nullptr,
        0);

    if (!hSession)
    {
        spdlog::error("WinHttpOpen failed.");
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        apiHost.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0);

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring userAgentHeader = L"User-Agent: " + buildUserAgent();
    WinHttpAddRequestHeaders(
        hRequest,
        userAgentHeader.c_str(),
        -1,
        WINHTTP_ADDREQ_FLAG_REPLACE);

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
