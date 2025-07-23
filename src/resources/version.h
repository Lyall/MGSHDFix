#pragma once

// Core name & version
#define FIX_NAME "MGSHDFix"
#define VERSION_STRING "2.5.1"
#define PRIMARY_REPO_URL "https://github.com/Lyall/MGSHDFix"
//#define FALLBACK_REPO_URL "https://codeberg.org/Lyall/MGSHDFix"


// Version
inline constexpr std::string sFixVersion = VERSION_STRING;
inline constexpr std::string sFixName = FIX_NAME;
inline constexpr int iConfigVersion = 4; //increment this when making config changes, along with the number at the bottom of the config file
//that way we can sanity check to ensure people don't have broken/disabled features due to old config files.



#define VERSION_MAJOR     2
#define VERSION_MINOR     5
#define VERSION_PATCH     1

// Metadata
#define COMPANY_NAME      "Lyall & Contributors"
#define PRODUCT_NAME      FIX_NAME
#define FILE_DESCRIPTION  FIX_NAME " ASI Plugin"
#define INTERNAL_NAME     FIX_NAME ".asi"
#define ORIGINAL_FILENAME FIX_NAME ".asi"
#define PRODUCT_VERSION   VERSION_STRING
#define FILE_VERSION      VERSION_STRING
#define LEGAL_COPYRIGHT   "© 2025 Lyall & Contributors. Licensed under the MIT License."
#define LEGAL_TRADEMARKS  ""
#define COMMENTS          ""
