#include "stdafx.h"

#include "common.hpp"
#include "version_checking.hpp"

#include "logging.hpp"


// MIRROR SYNC WITH THE CONFIG TOOL'S UPDATER.CPP

namespace
{
    std::vector<int> parseVersionString(const std::string& versionStr)
    {
        std::vector<int> parts;
        std::istringstream ss(versionStr);
        std::string token;

        while (std::getline(ss, token, '.'))
        {
            if (token.empty())
            {
                parts.push_back(0);
                {
                    continue;
                }
            }

            size_t i = 0;
            while (i < token.size() && std::isdigit(static_cast<unsigned char>(token[i])))
            {
                ++i;
            }

            int value = (i > 0) ? std::stoi(token.substr(0, i)) : 0;
            parts.push_back(value);

            if (i < token.size())
            {
                // take first suffix letter -> 'a' = 1, 'b' = 2, etc.
                char c = static_cast<char>(std::tolower(token[i]));
                if (c >= 'a' && c <= 'z')
                {
                    parts.push_back((c - 'a') + 1);
                }
                else
                {
                    parts.push_back(1); // fallback for weird suffix
                }
            }
        }

        return parts;
    }

    std::string GetVersionFromFilePathA(const char* filePath, VersionCheck::VersionType type, bool fourDigit)
    {
        if (!filePath || filePath[0] == '\0')
        {
            return "0.0.0";
        }

        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeA(filePath, &handle);
        if (size == 0)
        {
            return "0.0.0";
        }

        std::vector<BYTE> versionInfo(size);
        if (!GetFileVersionInfoA(filePath, handle, size, versionInfo.data()))
        {
            return "0.0.0";
        }

        if (type == VersionCheck::VersionType::Product)
        {
            struct LANGANDCODEPAGE 
            { 
                WORD wLanguage; 
                WORD wCodePage; 
            };
            LANGANDCODEPAGE* lpTranslate = nullptr;
            UINT cbTranslate = 0;

            if (VerQueryValueA(versionInfo.data(), "\\VarFileInfo\\Translation", reinterpret_cast<LPVOID*>(&lpTranslate), &cbTranslate) && cbTranslate >= sizeof(LANGANDCODEPAGE))
            {
                char subBlock[64] = { 0 };
                sprintf_s(subBlock, "\\StringFileInfo\\%04x%04x\\ProductVersion", lpTranslate[0].wLanguage, lpTranslate[0].wCodePage);

                char* productVersion = nullptr;
                UINT len = 0;

                if (VerQueryValueA(versionInfo.data(), subBlock, reinterpret_cast<LPVOID*>(&productVersion), &len) && productVersion && len > 1)
                {
                    return std::string(productVersion, len - 1);
                }
            }
        }

        VS_FIXEDFILEINFO* fileInfo = nullptr;
        UINT fileInfoLen = 0;
        if (!VerQueryValueA(versionInfo.data(), "\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLen) || !fileInfo)
        {
            return "0.0.0";
        }

        DWORD verMS = (type == VersionCheck::VersionType::Product) ? fileInfo->dwProductVersionMS : fileInfo->dwFileVersionMS;
        DWORD verLS = (type == VersionCheck::VersionType::Product) ? fileInfo->dwProductVersionLS : fileInfo->dwFileVersionLS;

        int major = HIWORD(verMS);
        int minor = LOWORD(verMS);
        int patch = HIWORD(verLS);

        if (fourDigit)
        {
            int build = LOWORD(verLS);
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch) + "." + std::to_string(build);
        }

        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }
}

namespace VersionCheck
{

    std::string GetModuleVersion(HMODULE module, VersionType type, bool fourDigit)
    {
        if (!module)
        {
            return "0.0.0";
        }

        char modulePath[MAX_PATH] = { 0 };
        if (!GetModuleFileNameA(module, modulePath, MAX_PATH))
        {
            return "0.0.0";
        }

        return GetVersionFromFilePathA(modulePath, type, fourDigit);
    }

    std::string GetFileVersion(const std::filesystem::path& filePath, VersionType type, bool fourDigit)
    {
        if (filePath.empty())
        {
            return "0.0.0";
        }

        const std::string pathA = filePath.string();
        return GetVersionFromFilePathA(pathA.c_str(), type, fourDigit);
    }

    CompareResult CompareSemanticVersion(const std::string& currentVersion, const std::string& targetVersion)
    {
        std::vector<int> currentParts = parseVersionString(currentVersion);
        std::vector<int> targetParts = parseVersionString(targetVersion);

        size_t n = std::max(currentParts.size(), targetParts.size());
        currentParts.resize(n, 0);
        targetParts.resize(n, 0);

        for (size_t i = 0; i < n; ++i)
        {
            if (currentParts[i] < targetParts[i])
            {
                return CompareResult::Older;
            }
            if (currentParts[i] > targetParts[i])
            {
                return CompareResult::Newer;
            }
        }
        return CompareResult::Equal;
    }
}

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

namespace
{
    // Prevent multiple dialogs per process lifetime
    bool g_ShownUpdateContactError = false;

    void ShowUpdateContactFailure(const std::vector<std::string>& providers)
    {
        if (g_ShownUpdateContactError)
        {
            return;
        }
        if (providers.empty())
        {
            return;
        }

        std::string joined;
        for (size_t i = 0; i < providers.size(); ++i)
        {
            joined += providers[i];
            if (i + 1 < providers.size())
            {
                joined += ", ";
            }
        }

        std::string msg =
            "Failed to contact " + joined +
            " while update checks are enabled.\n\n"
            "Is your firewall or network blocking the game from reaching the update provider?";

        MessageBoxA(nullptr, msg.c_str(), (sFixName + " update checker").c_str(), MB_OK | MB_ICONWARNING);
        g_ShownUpdateContactError = true;
    }
}

bool LatestVersionChecker::checkForUpdates()
{
    std::string cachedLatest;
    std::string warnedVersion;
    bool cacheIsFresh = false;
    bool didCheck = false;

    // Track which providers we attempted. If they all fail, pop a MessageBoxA.
    std::vector<std::string> attemptedProviders;

    if (!loadCache(cachedLatest, warnedVersion, cacheIsFresh))
    {
#if defined(PRIMARY_REPO_URL)
        {
            RepoInfo repoInfo = parseRepoUrl(PRIMARY_REPO_URL);
            spdlog::info("Version Check: No cache found. Contacting {} API for latest version.", repoInfo.displayName);
            attemptedProviders.push_back(repoInfo.displayName);

            if (queryLatestVersion(repoInfo, cachedLatest))
            {
                didCheck = true;
            }
        }
#endif
#if defined(FALLBACK_REPO_URL)
    if (!didCheck)
    {
        RepoInfo repoInfo = parseRepoUrl(FALLBACK_REPO_URL);
        spdlog::info("Version Check: Primary API failed or missing. Trying fallback {}.", repoInfo.displayName);
        attemptedProviders.push_back(repoInfo.displayName);

        if (queryLatestVersion(repoInfo, cachedLatest))
        {
            didCheck = true;
        }
    }
#endif
    if (!didCheck)
    {
        spdlog::info("Version Check: Unable to contact Repo API. Skipping version check.");
        ShowUpdateContactFailure(attemptedProviders);
        return false;
    }

    saveCache(cachedLatest, "");
    }
    else if (!cacheIsFresh)
    {
#if defined(PRIMARY_REPO_URL)
        {
            RepoInfo primaryRepoInfo = parseRepoUrl(PRIMARY_REPO_URL);
            spdlog::info("Version Check: Cache stale. Refreshing from {} API.", primaryRepoInfo.displayName);
            attemptedProviders.push_back(primaryRepoInfo.displayName);

            std::string latestFromApi;
            if (queryLatestVersion(primaryRepoInfo, latestFromApi))
            {
                cachedLatest = latestFromApi;
                saveCache(cachedLatest, warnedVersion);
                didCheck = true;
            }
        }
#endif

#if defined(FALLBACK_REPO_URL)
        if (!didCheck)
        {
            RepoInfo fallbackRepoInfo = parseRepoUrl(FALLBACK_REPO_URL);
            spdlog::info("Version Check: Primary API failed or missing on stale cache. Trying fallback {}.", fallbackRepoInfo.displayName);
            attemptedProviders.push_back(fallbackRepoInfo.displayName);

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
            ShowUpdateContactFailure(attemptedProviders);
            return false;
        }
    }
    else
    {
        spdlog::info("Version Check: Under {} hours since last update check. Skipping update check.", iCacheTTLHours);
    }

    switch (VersionCheck::CompareSemanticVersion(VERSION_STRING, cachedLatest))
    {
    case VersionCheck::CompareResult::Equal:
        spdlog::info("Version Check: {} is up to date.", FIX_NAME);
        return false;
    case VersionCheck::CompareResult::Newer:
        spdlog::info("Version Check: Welcome back, Commander! You are running a development build of {}!", FIX_NAME);
        spdlog::info("Version Check - Current Version: {}, Latest Release: {}", VERSION_STRING, cachedLatest);
        bDebugBuild = true;
        return false;
    case VersionCheck::CompareResult::Older:
        spdlog::warn("Version Check: A new version of {} is available.", FIX_NAME);
        spdlog::warn("Version Check - Current Version: {}, Latest Version: {}", VERSION_STRING, cachedLatest);
        if (!(eGameType & LAUNCHER))
        {
            bNewUpdateAvailable = true;
        }

        if (warnedVersion != cachedLatest)
        {
            if (bConsoleUpdateNotifications)
            {
                Logging::ShowConsole();
                std::cout << FIX_NAME << " Update Notice: New version of " << FIX_NAME
                    << " is available.\nCurrent Version: " << VERSION_STRING
                    << ", Latest Version: " << cachedLatest << std::endl;
            }
            saveCache(cachedLatest, cachedLatest);
            return true;
        }
        return false;
    }
    return false;
}

bool LatestVersionChecker::loadCache(std::string& cachedLatest, std::string& warnedVersion, bool& cacheIsFresh)
{
    std::ifstream file(m_cacheFile);
    if (!file)
    {
        return false;
    }

    std::string versionLine;
    std::string timeLine;
    std::string installedVersionLine;

    if (!std::getline(file, versionLine) || !std::getline(file, timeLine) || !std::getline(file, warnedVersion) || !std::getline(file, installedVersionLine))
    {
        return false;
    }

    cachedLatest = versionLine;

    auto cachedTime = parseISO8601(timeLine);
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - cachedTime);
    cacheIsFresh = age.count() <= iCacheTTLHours && installedVersionLine == VERSION_STRING;

    return true;
}

void LatestVersionChecker::saveCache(const std::string& latestVersion, const std::string& warnedVersion)
{
    std::ofstream file(m_cacheFile);
    if (!file)
    {
        return;
    }

    file << latestVersion << "\n";
    file << currentTimeISO8601() << "\n";
    file << warnedVersion << "\n";
    file << VERSION_STRING << "\n";
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

    // Reasonable timeouts to avoid hangs on blocked networks
    WinHttpSetTimeouts(hSession,
        5000,  // resolve
        5000,  // connect
        5000,  // send
        5000); // receive

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

    // User-Agent header
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
        // HTTP status
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        if (WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode, &size, WINHTTP_NO_HEADER_INDEX))
        {
            if (statusCode < 200 || statusCode >= 300)
            {
                spdlog::error("Version Check: {} responded with HTTP {}", repoInfo.displayName, statusCode);
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return false;
            }
        }

        DWORD avail = 0;
        do
        {
            DWORD downloaded = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &avail))
            {
                spdlog::error("Version Check: WinHttpQueryDataAvailable failed with error: {}", GetLastError());
                break;
            }
            if (!avail)
            {
                break;
            }

            std::string buffer(avail, 0);
            if (!WinHttpReadData(hRequest, &buffer[0], avail, &downloaded))
            {
                spdlog::error("Version Check: WinHttpReadData failed with error: {}", GetLastError());
                break;
            }
            response.append(buffer, 0, downloaded);
        } while (avail > 0);
    }
    else
    {
        spdlog::error("Version Check: WinHttpSendRequest or WinHttpReceiveResponse failed with error: {}", GetLastError());
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (response.empty())
    {
        return false;
    }

    // Try common patterns
    {
        std::smatch m;
        std::regex re(R"delim("tag_name"\s*:\s*"\s*v?([^"]+)")delim");
        if (std::regex_search(response, m, re) && m.size() > 1)
        {
            latestVersion = m[1];
            return true;
        }
    }
    {
        std::smatch m;
        std::regex reTag(R"delim("tag_name"\s*:\s*"\s*v?([^"]+)")delim");
        std::regex reName(R"delim("name"\s*:\s*"\s*v?(\d+\.\d+\.\d+)")delim");
        if (std::regex_search(response, m, reTag) && m.size() > 1)
        {
            latestVersion = m[1];
            return true;
        }
        if (std::regex_search(response, m, reName) && m.size() > 1)
        {
            latestVersion = m[1];
            return true;
        }
    }

    return false;
}

std::string LatestVersionChecker::currentTimeISO8601()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::tm tmLocal {};
#if defined(_WIN32)
    localtime_s(&tmLocal, &now_c);
#else
    tmLocal = *std::localtime(&now_c);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tmLocal, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::chrono::system_clock::time_point LatestVersionChecker::parseISO8601(const std::string& timeStr)
{
    std::tm tm {};
    std::istringstream ss(timeStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail())
    {
        // If parse fails, force a very old time to force refresh next run
        return std::chrono::system_clock::time_point {};
    }
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

#endif // Repo URL defined check
