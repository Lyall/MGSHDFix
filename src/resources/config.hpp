#pragma once

class Config final
{
public:
    static void Read();
};



inline float fAspectRatio;
inline bool bAspectFix;
inline bool bHUDFix;
inline bool bFOVFix;
inline bool bOutputResolution;
inline int iOutputResX;
inline int iOutputResY;
inline int iInternalResX;
inline int iInternalResY;
inline bool bWindowedMode;
inline bool bBorderlessMode;
inline bool bFramebufferFix;
inline bool bLauncherJumpStart;
inline int iAnisotropicFiltering;
inline bool bDisableTextureFiltering;
inline bool bMouseSensitivity;
inline float fMouseSensitivityXMulti;
inline float fMouseSensitivityYMulti;
inline bool bDisableCursor;

inline bool bIsPS2controltype = false;

// Launcher ini variables
inline bool bLauncherConfigSkipLauncher = false;
inline int iLauncherConfigCtrlType = 5;
inline int iLauncherConfigRegion = 0;
inline int iLauncherConfigLanguage = 0;
inline std::string sLauncherConfigMSXGame = "mg1";
inline int iLauncherConfigMSXWallType = 0;
inline std::string sLauncherConfigMSXWallAlign = "C";


inline const std::initializer_list<std::string> kLauncherConfigCtrlTypes = {
    "ps5",
    "ps4",
    "xbox",
    "nx",
    "stmd",
    "kbd",
    "ps2"
};

inline const std::initializer_list<std::string> kLauncherConfigLanguages = {
    "en",
    "jp",
    "fr",
    "gr",
    "it",
    "pr",
    "sp",
    "du",
    "ru"
};

inline const std::initializer_list<std::string> kLauncherConfigRegions = {
    "us",
    "jp",
    "eu"
};

inline std::pair DesktopDimensions = { 0,0 };