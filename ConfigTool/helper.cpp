#include "pch.h"
#include "helper.hpp"
#include <wx/app.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/thread.h>
#include "version.h"

namespace
{
    bool HasAnyGameExe(const std::filesystem::path& dir)
    {
        return std::filesystem::exists(dir / "METAL GEAR.exe") ||
            std::filesystem::exists(dir / "METAL GEAR SOLID2.exe") ||
            std::filesystem::exists(dir / "METAL GEAR SOLID3.exe");
    }

    std::filesystem::path TryFindDetectedGameRootByWalkingUp(const std::filesystem::path& start)
    {
        if (start.empty())
        {
            return {};
        }

        std::filesystem::path cur;
        try
        {
            cur = std::filesystem::weakly_canonical(start);
        }
        catch (...)
        {
            cur = start;
        }

        for (;;)
        {
            if (HasAnyGameExe(cur))
            {
                return cur;
            }

            const std::filesystem::path parent = cur.parent_path();
            if (parent.empty() || parent == cur)
            {
                break;
            }

            cur = parent;
        }

        return {};
    }

    bool IsWineHomePath(const std::filesystem::path& p)
    {
        const std::wstring ws = p.wstring();
        if (ws.size() < 7)
        {
            return false;
        }

        auto iequals_prefix = [](const std::wstring& s, const std::wstring& prefix)
            {
                if (s.size() < prefix.size())
                {
                    return false;
                }

                for (size_t i = 0; i < prefix.size(); ++i)
                {
                    if (towlower(s[i]) != towlower(prefix[i]))
                    {
                        return false;
                    }
                }
                return true;
            };

        return iequals_prefix(ws, L"Z:\\home");
    }

    bool IsSupportedAsiDirName(const std::filesystem::path& dir)
    {
        const std::wstring leaf = dir.filename().wstring();
        return (_wcsicmp(leaf.c_str(), L"plugins") == 0) ||
            (_wcsicmp(leaf.c_str(), L"scripts") == 0) ||
            (_wcsicmp(leaf.c_str(), L"update") == 0);
    }

    std::filesystem::path GetExeDirPath()
    {
        wxStandardPaths& sp = wxStandardPaths::Get();
        return std::filesystem::path(sp.GetExecutablePath().ToStdWstring()).parent_path();
    }


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
}

namespace Helper
{

    void RunOnMainThread(std::function<void()> fn)
    {
        if (wxIsMainThread())
        {
            fn();
        }
        else
        {
            wxTheApp->CallAfter(std::move(fn));
        }
    }

    // ------------------------------------------------------------
    // ASI location + install validation
    //
    // Invariant:
    //  - Return value is ALWAYS one of: <gameRoot>\plugins, \scripts, \update
    //  - Callers can always do: exePath = asiDir.parent_path()
    //
    // Also:
    //  - CWD is preferred to support symlinking
    //  - If CWD fails, exe dir is tried (Proton/Vortex/etc.)
    //  - Results are cached
    //  - If ASI is found but is NOT inside a valid game root (no game exe in parent),
    //    treat it as "wrong folder" and show the install guidance message here.
    // ------------------------------------------------------------

    std::filesystem::path FindASILocation(const std::string fileName)
    {
        static std::unordered_map<std::string, std::filesystem::path> s_cachedPaths;

        // Cache hit
        if (auto it = s_cachedPaths.find(fileName); it != s_cachedPaths.end())
        {
            const std::filesystem::path cachedDir = it->second;
            const std::filesystem::path cachedAsi = cachedDir / (fileName + ".asi");
            if (std::filesystem::exists(cachedAsi) && HasAnyGameExe(cachedDir.parent_path()))
            {
                return cachedDir;
            }

            s_cachedPaths.erase(it);
        }

        static const std::array<std::filesystem::path, 3> subdirs = {
            std::filesystem::path("plugins"),
            std::filesystem::path("scripts"),
            std::filesystem::path("update")
        };

        const std::filesystem::path cwdBase = std::filesystem::path(wxGetCwd().ToStdWstring());
        const std::filesystem::path exeDir = GetExeDirPath();

        std::filesystem::path lastFoundAsiDir; // set if we find the ASI but it is not a valid install (e.g. Vortex staging)

        auto isValidInstallForAsiDir = [&](const std::filesystem::path& asiDir) -> bool
            {
                if (asiDir.empty())
                {
                    return false;
                }

                const std::filesystem::path gameRoot = asiDir.parent_path();
                return HasAnyGameExe(gameRoot);
            };

        auto tryFindAsiDir = [&](const std::filesystem::path& base) -> std::filesystem::path
            {
                if (base.empty())
                {
                    return {};
                }

                // Case 1: user launched tool from inside plugins/scripts/update
                if (IsSupportedAsiDirName(base))
                {
                    const std::filesystem::path asiPath = base / (fileName + ".asi");
                    if (std::filesystem::exists(asiPath))
                    {
                        return base; // asiDir is already plugins/scripts/update
                    }
                }

                // Case 2: normal layout where base is game root
                for (const auto& sub : subdirs)
                {
                    const std::filesystem::path asiPath = base / sub / (fileName + ".asi");
                    if (std::filesystem::exists(asiPath))
                    {
                        return asiPath.parent_path(); // plugins/scripts/update
                    }
                }

                return {};
            };

        auto acceptOrRemember = [&](const std::filesystem::path& foundAsiDir) -> std::filesystem::path
            {
                if (foundAsiDir.empty())
                {
                    return {};
                }

                if (isValidInstallForAsiDir(foundAsiDir))
                {
                    s_cachedPaths[fileName] = foundAsiDir;
                    return foundAsiDir;
                }

                // Found the ASI, but it is not next to a real game exe (likely staging folder)
                lastFoundAsiDir = foundAsiDir;
                return {};
            };

            // 1) Prefer CWD (symlink friendly)
        if (std::filesystem::path found = tryFindAsiDir(cwdBase); !found.empty())
        {
            if (std::filesystem::path accepted = acceptOrRemember(found); !accepted.empty())
            {
                return accepted;
            }
        }

        // 2) Fallback to executable directory (Proton/Vortex friendly)
        if (std::filesystem::path found = tryFindAsiDir(exeDir); !found.empty())
        {
            if (std::filesystem::path accepted = acceptOrRemember(found); !accepted.empty())
            {
                return accepted;
            }
        }

        // ------------------------------------------------------------
        // Not found OR found only in an invalid location: show install guidance
        // ------------------------------------------------------------

        // If we found an ASI dir but it is not a valid install, show that location.
        // Otherwise show CWD (unless it's Proton home, in which case show EXE dir).
        const std::filesystem::path currentLocation =
            !lastFoundAsiDir.empty()
            ? lastFoundAsiDir.parent_path()
            : (IsWineHomePath(cwdBase) ? exeDir : cwdBase);

        std::filesystem::path detectedGameRoot = TryFindDetectedGameRootByWalkingUp(currentLocation);
        if (detectedGameRoot.empty())
        {
            const std::filesystem::path alt = (currentLocation == cwdBase) ? exeDir : cwdBase;
            detectedGameRoot = TryFindDetectedGameRootByWalkingUp(alt);
        }

        std::string message;

        if (!detectedGameRoot.empty())
        {
            message =
                "MGSHDFix has been extracted to the wrong folder!\n"
                "Please move all files from:\n\n" + currentLocation.string() +
                "\n\nto the detected game folder:\n\n" + detectedGameRoot.string();
        }
        else
        {
            message =
                "MGSHDFix could not determine a valid game installation folder.\n"
                "\n"
                "This usually happens for one of the following reasons:\n"
                "\n"
                " * The MGSHDFix zip file was extracted into a NEW FOLDER inside the game's directory.\n"
                "\n"
                " * " INTERNAL_NAME_CONFIG " was launched via a shortcut whose working directory (\"Start in\") is not set to the game's \"plugins\" folder.\n"
                "\n"
                "\n"
                "Current Location:\n\n" + currentLocation.string() + "\n"
                "\n"
                INTERNAL_NAME_CONFIG " must be launched from the game's \"plugins\" folder, or from a shortcut whose working directory is set to the game's \"plugins\" folder.\n"
                "\n"
                "If you are using a shortcut, edit its properties and set \"Start in\" to the game's \"plugins\" folder.\n"
                "If you are using symlinks, ensure the working directory resolves to the real \"plugins\" directory.\n"
                "\n"
                "All files must be extracted exactly as packaged inside the game's main folder.";

        }

        wxLogError(message);
        ExitProcess(1);
        return {};
    }

    VersionCompareResult CompareSemanticVersion(const std::string& currentVersion,
        const std::string& targetVersion)
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
                return VersionCompareResult::Older;
            }
            if (currentParts[i] > targetParts[i])
            {
                return VersionCompareResult::Newer;
            }
        }
        return VersionCompareResult::Equal;
    }

    std::string GetFileDescription(const std::string& filePath)
    {
        DWORD handle = 0;
        DWORD size = GetFileVersionInfoSizeA(filePath.c_str(), &handle);
        if (size > 0)
        {
            std::vector<BYTE> versionInfo(size);
            if (GetFileVersionInfoA(filePath.c_str(), handle, size, versionInfo.data()))
            {
                void* buffer = nullptr;
                UINT sizeBuffer = 0;
                if (VerQueryValueA(versionInfo.data(), R"(\VarFileInfo\Translation)", &buffer, &sizeBuffer))
                {
                    auto translations = static_cast<WORD*>(buffer);
                    size_t translationCount = sizeBuffer / sizeof(WORD) / 2; // Each translation is two WORDs (language and code page)
                    for (size_t i = 0; i < translationCount; ++i)
                    {
                        WORD language = translations[i * 2];
                        WORD codePage = translations[i * 2 + 1];
                        // Construct the query string for the file description
                        std::ostringstream subBlock;
                        subBlock << R"(\StringFileInfo\)" << std::hex << std::setw(4) << std::setfill('0') << language
                            << std::setw(4) << std::setfill('0') << codePage << R"(\ProductName)";
                        if (VerQueryValueA(versionInfo.data(), subBlock.str().c_str(), &buffer, &sizeBuffer))
                        {
                            return std::string(static_cast<char*>(buffer), sizeBuffer - 1);
                        }
                    }
                }
            }
        }

        return "File description not found.";
    }

    bool IsSteamOS()
    {
        static bool bCheckedSteamDeck = false;
        static bool bIsSteamDeck = false;

        if (bCheckedSteamDeck)
        {
            return bIsSteamDeck;
        }

        bCheckedSteamDeck = true;
        // Check for Proton/Steam Deck environment variables
        if (std::getenv("STEAM_COMPAT_CLIENT_INSTALL_PATH") || std::getenv("STEAM_COMPAT_DATA_PATH") || std::getenv("XDG_SESSION_TYPE"))
        {
            bIsSteamDeck = true;
        }

        return bIsSteamDeck;
    }
}
