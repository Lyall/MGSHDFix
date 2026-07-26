#pragma once
// ReSharper disable CppClangTidyModernizeMacroToEnum
#include <string>

// Core name & version
#define FIX_NAME "MGSHDFix"
#define PRIMARY_REPO_URL "https://github.com/ShizCalev/MGSHDFix"
#define FALLBACK_REPO_URL "https://gitlab.com/ShizCalev/MGSHDFix"
#define DISCORD_URL "https://discord.gg/bFv9bZmWDV"

#define VERSION_MAJOR 4
#define VERSION_MINOR 0
#define VERSION_PATCH 1
#define VERSION_CI_BUILD 0

/// Current release version of MGSHDFix at time of compile.
/// This is automatically set by CI and should be left at 0.0.0.
inline const std::string ASI_LOADER_VERSION_STRING = "0.0.0"; 

inline constexpr const char* kAsiLoaderDescription = "Ultimate-ASI-Loader-x64";


#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define VERSION_STRING STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH) "." STRINGIFY(VERSION_CI_BUILD)
inline const std::string sFixVersion = VERSION_STRING;
inline const std::string sFixName = FIX_NAME;
inline const std::string sFixNameAndVersion = FIX_NAME " " VERSION_STRING;

// Metadata
#define COMPANY_NAME      "Lyall, Afevis, & Contributors"
#define PRODUCT_NAME      FIX_NAME
#define PRODUCT_VERSION   VERSION_STRING
#define FILE_VERSION      VERSION_STRING
#define LEGAL_COPYRIGHT   "© 2026 Lyall, Afevis, & Contributors. Licensed under the MIT License."
#define LEGAL_TRADEMARKS  ""
#define COMMENTS          ""
#define FILE_DESCRIPTION_ASI     FIX_NAME " ASI Plugin"
#define INTERNAL_NAME_ASI        FIX_NAME ".asi"
#define ORIGINAL_FILENAME_ASI    FIX_NAME ".asi"

// Universal Config Tool Metadata
#define COMPANY_NAME_CONFIG      "Afevis"
#define LEGAL_COPYRIGHT_CONFIG   "© 2026 Afevis. Licensed under the MIT License."
#define FILE_DESCRIPTION_CONFIG  "Universal Config Tool"
#define PRODUCT_NAME_CONFIG      "Universal Config Tool for " FIX_NAME
#define INTERNAL_NAME_CONFIG     FIX_NAME " Config Tool.exe"
#define ORIGINAL_FILENAME_CONFIG FIX_NAME " Config Tool.exe"

#define IDI_ICON1           101
#define IDB_BANNER_MG1      102
#define IDB_BANNER_MGS2     103
#define IDB_BANNER_MGS3     104

