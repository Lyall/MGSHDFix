#pragma once

struct MGTextOverride
{
    uint32_t nBlock;
    int32_t nIndex;
    const char* pszLanguage;
    const char* pszReplacementUtf8;
};

namespace MG_TextOverrides
{
    inline bool bMG1RestoreBossNames = false;
    //inline bool bMG1RestoreOriginalScript = false;

    void InitializeMG1();
    inline bool bMG2RestoreBossNames = false;
    inline bool bMG2RestoreOriginalScript = true;

    void InitializeMG2();
}
