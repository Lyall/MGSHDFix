#pragma once

class IntroSkip final
{
public:
    bool isEnabled;
    void Initialize() const;
    void InitializeMG1() const;
    void InitializeMG2() const;
};

inline IntroSkip g_IntroSkip;
