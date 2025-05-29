#pragma once
#include "helper.hpp"
#include <inipp/inipp.h>

extern std::string sExeName;

enum MgsGame : std::uint8_t
{
    NONE     = 0,
    MGS2     = 1 << 0,
    MGS3     = 1 << 1,
    MG       = 1 << 2,
    LAUNCHER = 1 << 3,
    UNKNOWN  = 1 << 4
};
extern MgsGame eGameType;

extern inipp::Ini<char> ini;
extern HMODULE baseModule;
extern std::string sGameVersion;

//Config Options
extern int iCurrentResY;
extern float fAspectRatio;
