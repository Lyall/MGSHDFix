#pragma once

class CPUCoreLimitFix final
{
public:
    static void ApplyFix();

    bool bEnabled = false;
};

inline CPUCoreLimitFix g_CPUCoreLimitFix;
