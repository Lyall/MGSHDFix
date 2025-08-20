#pragma once
#include "config_keys.hpp"

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
inline std::string sLauncherConfigMSXGame = ConfigKeys::SkipLauncherMSX_Option_MG1;
inline int iLauncherConfigMSXWallType = 0;
inline std::string sLauncherConfigMSXWallAlign = ConfigKeys::MSXWallAlign_Option_Center;

inline std::pair DesktopDimensions = { 0,0 };