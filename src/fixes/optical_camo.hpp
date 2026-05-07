#pragma once

class OpticalCamoFix final
{
public:
    void Initialize() const;

    bool bEnabled = true;
};

inline OpticalCamoFix g_OpticalCamoFix;
