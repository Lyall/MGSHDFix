#pragma once

class MGS2UnderwaterFilterFix final
{
public:
    bool bEnabled = true;

    void Initialize();
    void PatchWork(void* work) const;
    void BeforePresent();
};

inline MGS2UnderwaterFilterFix g_MGS2UnderwaterFilterFix;
