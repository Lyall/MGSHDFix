#pragma once

class KeepAimingAfterFiring final
{
public:
    static void Initialize();
    static void HandleLevelTransition();

    bool bAlwaysKeepAiming;
    bool bKeepAimingInFirstPerson;
    bool bKeepAimingOnLockOn;
    bool bKeepAimingInFPSMode;
    bool bOverrodeState = false;
};

inline KeepAimingAfterFiring g_KeepAimingAfterFiring;
