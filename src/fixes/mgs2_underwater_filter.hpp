#pragma once

class MGS2UnderwaterFilterFix final
{
public:
    bool bEnabled = true;

    void Initialize();
    void PatchWork(void* work) const;
    void BeforePresent();
    void InstallD3D11StateHooks();
};

inline MGS2UnderwaterFilterFix g_MGS2UnderwaterFilterFix;
