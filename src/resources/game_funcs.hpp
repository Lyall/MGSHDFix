#pragma once

namespace MG1_LinkVarBuf
{
    struct DrawLayer;
}

// One entry of a stage chara table (chara.h CHARA_TABLE)
struct CHARA
{
    unsigned int class_id;
    void* (__fastcall* new_)(int, int);
};
static_assert(sizeof(CHARA) == 0x10);

namespace Shared_Gamefuncs
{
    using GetLanguage_t = int(__fastcall*)();
    inline GetLanguage_t pfnGetLanguage = nullptr;

    void HookFuncs();
    inline bool StartInDebugMode = false;
    inline int  DevMenuHotkey = 0;


    using GM_SetArea_t = void(__fastcall*)(int id, const char* dirname);
    inline GM_SetArea_t GM_SetArea = nullptr;

    using GM_GetArea_t = const char* ();
    inline GM_GetArea_t* GM_GetArea = nullptr;

    using BP_AdjustTick_t = int(__fastcall*)(int tick);
    inline BP_AdjustTick_t BP_AdjustTick = nullptr;

    using GCL_ChangeSenerioCode_t = void(__fastcall*)(int code);
    inline GCL_ChangeSenerioCode_t GCL_ChangeSenerioCode = nullptr;

    using BP_GetAssetLoadFullPath_t = void(__fastcall*)(char* fullPath, char* unifiedFullPath, const char* manifestLoadPath, const char* manifestUnifiedPath);
    inline BP_GetAssetLoadFullPath_t BP_GetAssetLoadFullPath = nullptr;
};



namespace MGS2_GameFuncs
{
    void HookGameFuncs();

    // More than one fix hooks these, so they're scanned once here, before any of those hooks go in.
    inline uint8_t* DM_ExecDemoStream = nullptr;
    inline uint8_t* BP_UpdatePreshadeBuffer = nullptr;
    
    using GM_SeSet_t = int64_t(__fastcall*)(int a1, uint8_t a2, int16_t a3);
    inline GM_SeSet_t GM_SeSet = nullptr;

    using GM_ItemNum_t = int(__fastcall*)(int a1);
    inline GM_ItemNum_t GM_ItemNum = nullptr;

    using GM_WeaponNum_t = int(__fastcall*)(int a1);
    inline GM_WeaponNum_t GM_WeaponNum = nullptr;

    using L2D_GetObject_t = void* (__fastcall*)(int handle, int strcode);
    inline L2D_GetObject_t L2D_GetObject = nullptr;


    using L2D_GetParts_t = void* (__fastcall*)(int, int);
    inline L2D_GetParts_t L2D_GetParts = nullptr;

    using WriteString_t = void(__fastcall*)(void*, const char*, int);
    inline WriteString_t WriteString = nullptr;

    using UpdateVectors_t = void(__fastcall*)(void* work);
    inline UpdateVectors_t   UpdateVectors_4 = nullptr;
    
    using GM_GetDGGroupID_t = int(__fastcall*)(int stageMap);
    inline GM_GetDGGroupID_t GM_GetDGGroupID = nullptr;
    
    using GV_DestroyActor_t = void(__fastcall*)(void* work);
    inline GV_DestroyActor_t GV_DestroyActor = nullptr;

    using HZX_OnlineHazardCheck_t = int(__fastcall*)(int id, const float* from, const float* to, int chk, int seg, int flr);
    inline HZX_OnlineHazardCheck_t HZX_OnlineHazardCheck = nullptr;

}



namespace MG_Gamefuncs
{
    using SetFont_t = void(__fastcall*)(int nFont, uintptr_t hGlyphs, uintptr_t hKanji, int nHeight);
    using DrawText_t = void(__fastcall*)(uint32_t* pFrame, int* pText);
    using ComposeTextLayer_t = void(__fastcall*)(int nDrawText);
    using MeasureTextFont_t = unsigned int(__fastcall*)(char* pszText, int nStart, int nLength, int nFont);
    using UploadTexture_t = int64_t(__fastcall*)(void* pTexture, uint32_t* pPixels);
}

namespace MG1_Gamefuncs
{
    using GetChunkEntryData_t = const uint8_t* (__fastcall*)(uintptr_t hChunk, int nEntry);
    inline GetChunkEntryData_t pfnGetChunkEntryData = nullptr;
    using GetChunkEntrySize_t = uint32_t(__fastcall*)(uintptr_t hChunk, int nEntry);
    inline GetChunkEntrySize_t pfnGetChunkEntrySize = nullptr;

    using SceneTick_t = void(__fastcall*)();
    inline SceneTick_t pfnSceneTick = nullptr;

    using ShowMessage_t = void(__fastcall*)(int nChunk, int nRecord, int nAutoAdvance, int nReturnScene, int nScriptSlot);
    inline ShowMessage_t pfnShowMessage = nullptr;
    using BlitImage_t = void(__fastcall*)(int nLayer, const MG1_LinkVarBuf::DrawLayer* pImage, const int* pSourceRect, int nX, int nY);
    inline BlitImage_t pfnBlitImage = nullptr;

    using GotoStage_t = void(__fastcall*)(int nStage, int nRoom);
    inline GotoStage_t pfnGotoStage = nullptr;
    using SwitchScene_t = void(__fastcall*)();
    inline SwitchScene_t pfnSwitchScene = nullptr;

    using SetFont_t = MG_Gamefuncs::SetFont_t;
    inline SetFont_t pfnSetFont = nullptr;

    using DrawTextSlot_t = MG_Gamefuncs::DrawText_t;
    inline DrawTextSlot_t pfnDrawTextSlot = nullptr;
    using ComposeTextLayer_t = MG_Gamefuncs::ComposeTextLayer_t;
    inline ComposeTextLayer_t pfnComposeTextLayer = nullptr;
    using MeasureTextFont_t = MG_Gamefuncs::MeasureTextFont_t;
    inline MeasureTextFont_t pfnMeasureTextFont = nullptr;
    using UploadTexture_t = MG_Gamefuncs::UploadTexture_t;
    inline UploadTexture_t pfnUploadTexture = nullptr;

    using TestFlag_t = int(__fastcall*)(int flag);
    inline TestFlag_t TestFlag = nullptr;   // equiv of gcl varbuf

    void HookGameFuncs();

    void HookDllGameFuncs();
}



namespace MG2_LinkVarBuf
{
    struct EngineState;
    struct ObjectPosition;
    struct Sprite;
}

namespace MG2_Gamefuncs
{
    using SceneTick_t = void(__fastcall*)();
    inline SceneTick_t pfnSceneTick = nullptr;

    using GetEngineState_t = MG2_LinkVarBuf::EngineState* (__fastcall*)();
    inline GetEngineState_t pfnGetEngineState = nullptr;
    using GetPlayer_t = MG2_LinkVarBuf::ObjectPosition* (__fastcall*)();
    inline GetPlayer_t pfnGetPlayer = nullptr;
    using GetObjectSprite_t = MG2_LinkVarBuf::Sprite* (__fastcall*)(void* pObject);
    inline GetObjectSprite_t pfnGetObjectSprite = nullptr;
    using PrepareRoomEntry_t = void(__fastcall*)(void* pPlayer, MG2_LinkVarBuf::EngineState* pState, MG2_LinkVarBuf::Sprite* pSprite, int nEntrance, int nKeepSnakeRoom, unsigned int nEntryMode);
    inline PrepareRoomEntry_t pfnPrepareRoomEntry = nullptr;
    using RequestScene_t = void(__fastcall*)(int nScene);
    inline RequestScene_t pfnRequestScene = nullptr;
    using RunSceneTransition_t = void(__fastcall*)();
    inline RunSceneTransition_t pfnRunSceneTransition = nullptr;
    using IsGameplayActive_t = int(__fastcall*)();
    inline IsGameplayActive_t pfnIsGameplayActive = nullptr;

    using TestFlag_t = int(__fastcall*)(int nFlag);
    inline TestFlag_t pfnTestFlag = nullptr;
    using LoadImageBlob_t = void(__fastcall*)(void* pImageSlot, const uint8_t* pBlob);
    inline LoadImageBlob_t pfnLoadImageBlob = nullptr;
    using GetChunkEntryData_t = uint8_t* (__fastcall*)(uintptr_t hChunk, int nEntry);
    inline GetChunkEntryData_t pfnGetChunkEntryData = nullptr;
    using GetChunkEntrySize_t = unsigned int(__fastcall*)(uintptr_t hChunk, int nEntry);
    inline GetChunkEntrySize_t pfnGetChunkEntrySize = nullptr;

    using IsBossSurvival_t = int(__fastcall*)();
    inline IsBossSurvival_t pfnIsBossSurvival = nullptr;

    using SetFont_t = MG_Gamefuncs::SetFont_t;
    inline SetFont_t pfnSetFont = nullptr;

    using DrawTextItem_t = MG_Gamefuncs::DrawText_t;
    inline DrawTextItem_t pfnDrawTextItem = nullptr;

    using ComposeTextLayer_t = MG_Gamefuncs::ComposeTextLayer_t;
    inline ComposeTextLayer_t pfnComposeTextLayer = nullptr;

    using MeasureTextFont_t = MG_Gamefuncs::MeasureTextFont_t;
    inline MeasureTextFont_t pfnMeasureTextFont = nullptr;

    using UploadTexture_t = MG_Gamefuncs::UploadTexture_t;
    inline UploadTexture_t pfnUploadTexture = nullptr;

    using AVS_GetLocalizedString_t = const char* (__fastcall*)(unsigned int nBlock, int nIndex, int* pLength);
    inline AVS_GetLocalizedString_t pfnAVS_GetLocalizedString = nullptr;

    // Message typewriter: pArgs[0] = block, pArgs[1] = index.
    using DrawMessageText_t = void(__fastcall*)(unsigned int* pArgs, int nMode);
    inline DrawMessageText_t pfnDrawMessageText = nullptr;

    using ScriptRun_t = void(__fastcall*)(int* pScript);
    inline ScriptRun_t pfnScriptRun = nullptr;
    using DrawSprite_t = void(__fastcall*)(MG2_LinkVarBuf::Sprite* pSprite, int nMode);
    inline DrawSprite_t pfnDrawSprite = nullptr;
    using InitSprite_t = void(__fastcall*)(MG2_LinkVarBuf::Sprite* pSprite, int nSheet, int nAnimation, int nTrack, int nAnimating, int nDrawOrder, int nX, int nY);
    inline InitSprite_t pfnInitSprite = nullptr;
    using BuildDoors_t = void(__fastcall*)();
    inline BuildDoors_t pfnBuildDoors = nullptr;
    using DrawSprites_t = void(__fastcall*)();
    inline DrawSprites_t pfnDrawSprites = nullptr;
    using LayerFillRect_t = void(__fastcall*)(int nLayer, const int* pRectangle);
    inline LayerFillRect_t pfnLayerFillRect = nullptr;
    using SetDrawColor_t = void(__fastcall*)(int nLayer, int nColour);
    inline SetDrawColor_t pfnSetDrawColor = nullptr;
    using RequestSound_t = void(__fastcall*)(int nSound);
    inline RequestSound_t pfnRequestSound = nullptr;

    using ScriptCommand_t = int(__fastcall*)(int* pScript, uint8_t nCommand, int* pArgs, char* pStrings, int nCount);
    inline ScriptCommand_t pfnScriptCommand = nullptr;

    using HasItem_t = int(__fastcall*)(int itemId);
    inline HasItem_t HasItem = nullptr;

    void HookDllGameFuncs();
}



namespace MGS3_Gamefuncs
{

    void HookGameFuncs();
}


