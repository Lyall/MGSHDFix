#include "common.hpp"
#include "spdlog/spdlog.h"

void Init_ASILoaderSanityChecks()
{
    //Don't simplify by removing filesystem::exists() from this check. While GetFileDescription does handle non-existent files own its own, checking filesystem::exists() first saves 400+ ms of initialization time
    if (std::filesystem::exists(sExePath / "d3d11.dll") && (Util::GetFileDescription((sExePath / "d3d11.dll").string()) == Util::GetFileDescription((sExePath / "winhttp.dll").string())))
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "DUPLICATE MOD LOADER ERROR: Multiple ASI Loader .dll's detected! This can cause inconsistent bugs and crashes.\n";
        spdlog::error("DUPLICATE MOD LOADER ERROR: Multiple ASI Loader .dll installations detected! This can cause inconsistent bugs and crashes.");
        std::cout << "DUPLICATE MOD LOADER ERROR: Please delete d3d11.dll, it has been replaced by winhttp.dll & wininit.dll.\n";
        spdlog::error("DUPLICATE MOD LOADER ERROR: Please delete d3d11.dll, it has been replaced by winhttp.dll & wininit.dll.");
#ifndef _WIN32
        std::cout << "DUPLICATE MOD LOADER ERROR: Steam Deck / Linux users must also replace their Steam game launch paramaters with the following command:\n";
        spdlog::error("DUPLICATE MOD LOADER ERROR: Steam Deck / Linux users must also replace their Steam game launch paramaters with the following command:");
        std::cout << "`WINEDLLOVERRIDES=\"wininet,winhttp=n,b\" % command % `\n";
        spdlog::error("`WINEDLLOVERRIDES=\"wininet,winhttp=n,b\" % command % `");
#endif
        spdlog::info("----------");
    }
    Util::CheckForASIFiles(sFixName, true, true, nullptr); //Exit thread & warn the user if multiple copies of MGSHDFix are trying to initialize.
}
