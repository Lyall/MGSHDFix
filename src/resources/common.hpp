#pragma once
#include "helper.hpp"
#include "logging.hpp"


inline std::string sExeName;
inline std::filesystem::path sExePath;
inline std::filesystem::path sGameSavePath;
inline bool bIsLauncher = false;

inline HMODULE baseModule = GetModuleHandle(NULL);
inline HMODULE engineModule;
inline HMODULE unityPlayer;


struct GameInfo
{
    std::string GameTitle;
    std::string ExeName;
    int SteamAppId;
};
inline const GameInfo* game = nullptr;

enum MgsGame : std::uint8_t
{
    NONE     = 0,
    MGS2     = 1 << 0,
    MGS3     = 1 << 1,
    MG       = 1 << 2,
    LAUNCHER = 1 << 3,
    UNKNOWN  = 1 << 4
};
inline MgsGame eGameType = UNKNOWN;


enum class MgSubGame : std::uint8_t
{
    NONE = 0,
    MG1,
    MG2
};
inline MgSubGame eMgSubGame = MgSubGame::NONE;

inline const std::map<MgsGame, GameInfo> kGames = {
    {MGS2, {"Metal Gear Solid 2 MC", "METAL GEAR SOLID2.exe", 2131640}},
    {MGS3, {"Metal Gear Solid 3 MC", "METAL GEAR SOLID3.exe", 2131650}},
    {MG, {"Metal Gear / Metal Gear 2 (MSX)", "METAL GEAR.exe", 2131680}},
};

inline bool usDatExists;
inline bool jpDatExists;

constexpr float f_PS2_Width = 512.0f;
constexpr float f_PS2_Height = 448.0f;


//#define BEFORE_COMPARISON_PICS
#if defined(BEFORE_COMPARISON_PICS)
//#define DISABLE_MGS3_CUTSCENE_CAMERA_FIX
//#define BEFORE_COMPARISON_PICS_NO_GAMMA_CORRECTION
#define BEFORE_COMPARISON_PICTURES_NO_TYPO_FIXES
#define BEFORE_COMPARISON_PICS_NO_SMAA
#endif


