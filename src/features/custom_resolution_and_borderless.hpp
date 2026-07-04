#pragma once

namespace CustomResolutionAndBorderless
{
    void Init_FixDPIScaling();
    void Init_CalculateScreenSize();
    void Init_CustomResolution();
    void Init_ScaleEffects();
    void Init_AspectFOVFix();
    void Init_HUDFix();

    inline bool bAspectFix;
    inline bool bHUDFix;
    inline bool bFOVFix;
    inline bool bFramebufferFix;

    inline std::pair DesktopDimensions = { 0,0 };

    inline std::string bWindowOrFullscreenMode;
    inline bool bOutputResolution;
    inline int iOutputResX;
    inline int iOutputResY;
    inline float fHeightDeltaFrom720p;
    inline int iOriginalOutputResX; //before clamping in windowed/borderless mode
    inline int iOriginalOutputResY; //before clamping in windowed/borderless mode
    inline bool bUsingAutomaticOutputX = false;
    inline bool bUsingAutomaticOutputY = false;

    inline int iInternalResX;
    inline int iInternalResY;
    
    inline int iFinalWindowResolutionX;
    inline int iFinalWindowResolutionY;

    inline bool bWindowedMode = false;
    inline bool bBorderlessMode = false;
    inline bool bMaximizeBorderless = false;
    inline bool bLimitToPrimaryMonitor = false;

    inline bool bEnableFSRWarning = true;


}

