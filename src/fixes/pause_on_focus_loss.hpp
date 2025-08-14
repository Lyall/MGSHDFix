#pragma once

class PauseOnFocusLoss final
{
public:
    static void Initialize();
    static bool ShouldFixPauseState();

    bool bPauseOnFocusLoss;
    bool bFixAltTabBugs;
};

inline PauseOnFocusLoss g_PauseOnFocusLoss;


