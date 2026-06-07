#pragma once

namespace CustomPlayerName
{
    void Apply();

    inline bool bUseStoryName = false;
    
    inline bool bUseCustomName = false;
    inline std::string sCustomName = "LIFE";

    constexpr int MAX_NAME_LENGTH = 15; // max length for name strings

}

