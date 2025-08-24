#pragma once
// ReSharper disable CppClangTidyModernizeMacroToEnum
#include <string>

// Core name & version
#define FIX_NAME "MGSHDFix"
#define PRIMARY_REPO_URL "https://github.com/Lyall/MGSHDFix"
#define FALLBACK_REPO_URL "https://github.com/ShizCalev/MGSHDFix"

#define VERSION_MAJOR 2
#define VERSION_MINOR 8
#define VERSION_PATCH 0


#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define VERSION_STRING STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)
inline const std::string sFixVersion = VERSION_STRING;
inline const std::string sFixName = FIX_NAME;

// Metadata
#define COMPANY_NAME      "Lyall & Contributors"
#define PRODUCT_NAME      FIX_NAME
#define PRODUCT_VERSION   VERSION_STRING
#define FILE_VERSION      VERSION_STRING
#define LEGAL_COPYRIGHT   "© 2025 Lyall & Contributors. Licensed under the MIT License."
#define LEGAL_TRADEMARKS  ""
#define COMMENTS          ""
#define FILE_DESCRIPTION_ASI     FIX_NAME " ASI Plugin"
#define INTERNAL_NAME_ASI        FIX_NAME ".asi"
#define ORIGINAL_FILENAME_ASI    FIX_NAME ".asi"

// Universal Config Tool Metadata
#define COMPANY_NAME_CONFIG      "Afevis"
#define LEGAL_COPYRIGHT_CONFIG   "© 2025 Afevis. Licensed under the MIT License."
#define FILE_DESCRIPTION_CONFIG  "Universal Config Tool"
#define PRODUCT_NAME_CONFIG      "Universal Config Tool for " FIX_NAME
#define INTERNAL_NAME_CONFIG     FIX_NAME " Config Tool.exe"
#define ORIGINAL_FILENAME_CONFIG FIX_NAME " Config Tool.exe"

#define IDI_ICON1           101
#define IDB_BANNER_MG1      102
#define IDB_BANNER_MGS2     103
#define IDB_BANNER_MGS3     104

