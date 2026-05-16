#pragma once

namespace MGS2_Hostage_Type_Easter_Egg
{
    void Force();

    enum HostageMode : std::uint8_t
    {
        RTC_NORMAL,
        RTC_JENNIFER,   /// Midnight - Jennifer Love Hewitt
        RTC_CATHY,      /// 10PM 
        RTC_KATOCHAN    /// 1PM - Kato-chan
    };
    inline HostageMode hostageMode = RTC_NORMAL;
}
