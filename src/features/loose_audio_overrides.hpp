#pragma once

namespace LooseAudioOverrides
{
    inline bool bEnabled = false;
    inline std::string sDebugLogging = "Disabled";
    inline std::string sLoudnessMode = "Use File Volume";
    void Initialize();
    bool HasVariant(std::wstring_view svAsset);
    bool EndVariant(std::wstring_view svAsset);
}
