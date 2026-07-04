#pragma once

namespace MGS2_RestoreActionLevelSelection
{
    void Apply();
    bool IsActionLevelCaptionActive();

    inline bool bEnabled = true;

    constexpr int32_t kActionLevelCaptionName = 2328243;
    constexpr int32_t kActionLevelCaptionY = 337;
    constexpr int32_t kJapaneseActionLevelCaptionYOffset = 4;
}
