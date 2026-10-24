#include "stdafx.h"
#include "mg2_room_patches.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    constexpr int nPitfallWidth = 32;
    constexpr int nPitfallHeight = 31;
    constexpr int nPitfallFrames = 8;

    struct PitfallFrame
    {
        std::array<uint8_t, nPitfallWidth * nPitfallHeight> anPixels;
        uintptr_t pPixels = 0;
        DrawLayer stImage {};
    };

    struct PitfallPatch
    {
        int nArea;
        int nRoom;
        int nX;
        int nY;
        int nVariant;
        int nRecord = -1;
        int nScript = -1;
        int nOffsetX = 0;
        int nOffsetY = 0;
        bool bAdd = false;
        std::vector<uint8_t> vExpected;
        std::vector<PitfallFrame> vFrames;
    };

    struct DoorArtwork
    {
        MG2_RoomPatches::DoorArtworkDefinition stDefinition;
        uintptr_t pPixels = 0;
        DrawLayer stImage {};
    };

    std::vector<DoorArtwork> s_vDoorArtwork;

    struct DrawOffset
    {
        int nX;
        int nY;
    };

    std::vector<MG2_RoomPatches::DoorDefinition> s_vDoors;
    std::vector<uint8_t> s_vDoorTable;
    std::unordered_map<int, DrawOffset> s_mDoorRecords;
    // MG2 objects and sprites are movable handles
    struct DoorSprite
    {
        uintptr_t hObject;
        DrawOffset stOffset;
        DoorArtwork* pArtwork;
    };

    std::vector<DoorSprite> s_vDoorSprites;
    const uint8_t* s_pOriginalDoors = nullptr;
    std::vector<PitfallPatch> s_vPitfalls;
    std::vector<uint8_t> s_vObjectTable;
    std::vector<PitfallPatch*> s_vAreaPitfalls;
    std::unordered_map<Sprite*, PitfallPatch*> s_mPitfalls; // Script-created pitfall sprites.

    struct ObjectPitfall
    {
        uintptr_t hObject;
        PitfallPatch* pPatch;
    };

    std::vector<ObjectPitfall> s_vObjectPitfalls; // Room-object pitfalls (see DoorSprite).
    SafetyHookMid s_hRegisterPitfall;
    SafetyHookMid s_hDrawArtwork;
    uintptr_t s_hAreaChunk = 0;
    const uint8_t* s_pOriginalObjects = nullptr;
    int s_nObjectArea = -1;
    SafetyHookMid s_hBuildObjects;
    SafetyHookInline s_hGetChunkEntryData;
    SafetyHookInline s_hGetChunkEntrySize;
    SafetyHookMid s_hBuildDoors;
    SafetyHookMid s_hRegisterDoor;
    SafetyHookMid s_hInitSprite;
    SafetyHookInline s_hDrawSprite;

    DoorSprite* FindDoorSprite(const Sprite* pSprite)
    {
        if (GM_Scene != MG2_SCENE_GAMEPLAY)
        {
            return nullptr; // Door objects are released while an area loads.
        }
        for (auto& stDoor : s_vDoorSprites)
        {
            auto* pObject = *reinterpret_cast<void**>(stDoor.hObject);
            if (pObject && MG2_Gamefuncs::pfnGetObjectSprite(pObject) == pSprite)
            {
                return &stDoor;
            }
        }
        return nullptr;
    }

    PitfallPatch* FindPitfall(Sprite* pSprite)
    {
        if (const auto itPitfall = s_mPitfalls.find(pSprite); itPitfall != s_mPitfalls.end())
        {
            return itPitfall->second;
        }
        if (GM_Scene != MG2_SCENE_GAMEPLAY)
        {
            return nullptr;
        }
        for (const auto& stPitfall : s_vObjectPitfalls)
        {
            auto* pObject = *reinterpret_cast<void**>(stPitfall.hObject);
            if (pObject && MG2_Gamefuncs::pfnGetObjectSprite(pObject) == pSprite)
            {
                return stPitfall.pPatch;
            }
        }
        return nullptr;
    }

    uint8_t* __fastcall GetChunkEntryData_hook(uintptr_t hChunk, int nEntry)
    {
        auto* pData = s_hGetChunkEntryData.call<uint8_t*>(hChunk, nEntry);
        if (nEntry == 2 && hChunk == s_hAreaChunk && GM_Area == s_nObjectArea && pData == s_pOriginalObjects && !s_vObjectTable.empty())
        {
            return s_vObjectTable.data();
        }
        return pData;
    }

    unsigned int __fastcall GetChunkEntrySize_hook(uintptr_t hChunk, int nEntry)
    {
        if (nEntry == 2 && hChunk == s_hAreaChunk && GM_Area == s_nObjectArea && !s_vObjectTable.empty())
        {
            return static_cast<unsigned int>(s_vObjectTable.size());
        }
        return s_hGetChunkEntrySize.call<unsigned int>(hChunk, nEntry);
    }

    void PatchObjectRecords(SafetyHookContext& ctx)
    {
        s_mPitfalls.clear();
        s_vObjectPitfalls.clear();
        s_vAreaPitfalls.clear();
        s_vObjectTable.clear();
        s_hAreaChunk = 0;
        s_pOriginalObjects = nullptr;
        s_nObjectArea = -1;
        if (ctx.rdi != 0)
        {
            return; // BuildRoomObjects saved IsBossSurvival() in rdi.
        }
        const int nArea = GM_Area;
        const auto itFirst = std::find_if(s_vPitfalls.begin(), s_vPitfalls.end(), [nArea](const PitfallPatch& stPatch) { return stPatch.nArea == nArea && stPatch.nScript < 0; });
        if (itFirst == s_vPitfalls.end())
        {
            return;
        }
        const uintptr_t hChunk = ctx.rcx;
        const auto* pOriginal = s_hGetChunkEntryData.call<uint8_t*>(hChunk, 2);
        const auto nSize = s_hGetChunkEntrySize.call<unsigned int>(hChunk, 2);
        if (nSize != itFirst->vExpected.size() || std::memcmp(pOriginal, itFirst->vExpected.data(), nSize) != 0)
        {
            spdlog::warn("MG2: Room Patches : A{} object table has changed. Skipping pitfall patches.", nArea);
            return;
        }
        const auto* pGrid = s_hGetChunkEntryData.call<uint8_t*>(hChunk, 0);
        const auto nGridSize = s_hGetChunkEntrySize.call<unsigned int>(hChunk, 0);
        if (nGridSize < 4 || nGridSize < 4u + pGrid[0] * pGrid[1])
        {
            spdlog::warn("MG2: Room Patches : A{} room grid is invalid. Skipping pitfall patches.", nArea);
            return;
        }
        const size_t nRecordsEnd = 1 + pOriginal[0] * nRoomObjectRecordSize;
        std::vector<uint8_t> vTable(pOriginal, pOriginal + nRecordsEnd);
        std::vector<PitfallPatch*> vAreaPitfalls(pOriginal[0], nullptr);
        for (auto& stPatch : s_vPitfalls)
        {
            if (stPatch.nArea != nArea || stPatch.nScript >= 0)
            {
                continue;
            }
            const int nGridX = stPatch.nX / nRoomWidth;
            const int nGridY = stPatch.nY / nRoomHeight;
            if (nGridX >= pGrid[0] || nGridY >= pGrid[1] || pGrid[4 + nGridY * pGrid[0] + nGridX] != stPatch.nRoom)
            {
                spdlog::warn("MG2: Room Patches : A{} R{} pitfall position is outside its room. Skipping pitfall patches.", nArea, stPatch.nRoom);
                return;
            }
            if (!stPatch.bAdd)
            {
                vAreaPitfalls[stPatch.nRecord] = &stPatch;
                continue;
            }
            std::array<uint8_t, nRoomObjectRecordSize> anRecord { MG2_ROOM_OBJECT_PITFALL, 0, 0, 0, 0, 20, 0, static_cast<uint8_t>(stPatch.nVariant) };
            Memory::WriteBE16(anRecord.data() + 1, static_cast<uint16_t>(stPatch.nX));
            Memory::WriteBE16(anRecord.data() + 3, static_cast<uint16_t>(stPatch.nY));
            vTable.insert(vTable.end(), anRecord.begin(), anRecord.end());
            vTable[0]++;
            vAreaPitfalls.push_back(&stPatch);
        }
        // Place routes after the expanded record table so MC resolves their offsets correctly.
        vTable.insert(vTable.end(), pOriginal + nRecordsEnd, pOriginal + nSize);
        s_vObjectTable = std::move(vTable);
        s_vAreaPitfalls = std::move(vAreaPitfalls);
        s_hAreaChunk = hChunk;
        s_pOriginalObjects = pOriginal;
        s_nObjectArea = nArea;
        spdlog::info("MG2: Room Patches : A{} initialized {} pitfall patches ({} additions).", nArea, std::count_if(s_vAreaPitfalls.begin(), s_vAreaPitfalls.end(), [](const PitfallPatch* pPatch) { return pPatch != nullptr; }), s_vObjectTable[0] - pOriginal[0]);
    }

    void RegisterPitfall(SafetyHookContext& ctx)
    {
        if (!s_pOriginalObjects || s_vAreaPitfalls.empty())
        {
            return;
        }
        const int nRecord = static_cast<int>(ctx.r13);
        if (nRecord < 0 || static_cast<size_t>(nRecord) >= s_vAreaPitfalls.size())
        {
            return;
        }
        auto* pPatch = s_vAreaPitfalls[nRecord];
        if (!pPatch)
        {
            return;
        }
        // object's handle slot, read the same way BuildRoomObjects reads it after CreateObject
        const auto* pSlot = reinterpret_cast<const uintptr_t*>(*reinterpret_cast<const uintptr_t*>(ctx.rsp + 0x60) + ctx.r14);
        const uintptr_t hObject = Memory::IsReadable(pSlot, sizeof(uintptr_t)) ? *pSlot : 0;
        if (!Memory::IsReadable(reinterpret_cast<const void*>(hObject), sizeof(uintptr_t)) || *reinterpret_cast<const uintptr_t*>(hObject) != ctx.rsi)
        {
            spdlog::warn("MG2: Room Patches : A{} pitfall {} object handle not found. Skipping artwork.", pPatch->nArea, nRecord);
            return;
        }
        s_vObjectPitfalls.push_back({ hObject, pPatch });
    }

    void RegisterScriptPitfall(SafetyHookContext& ctx)
    {
        if (static_cast<int>(ctx.rdx) != MG2_SPRITE_SHEET_ROOM_OBJECTS)
        {
            return;
        }
        for (auto& stPatch : s_vPitfalls)
        {
            if (stPatch.nScript < 0 || !IsRoom(stPatch.nArea, stPatch.nRoom) || static_cast<int>(ctx.r8) != MG2_PITFALL_ANIMATION_FIRST + stPatch.nVariant)
            {
                continue;
            }
            if (*reinterpret_cast<const int*>(ctx.rsp + 0x38) != stPatch.nX % nRoomWidth || *reinterpret_cast<const int*>(ctx.rsp + 0x40) != stPatch.nY % nRoomHeight)
            {
                continue;
            }
            const auto* pHandle = reinterpret_cast<const uintptr_t*>(pLoadedScripts[stPatch.nScript]);
            if (!Memory::IsReadable(pHandle, sizeof(uintptr_t)))
            {
                return;
            }
            const auto* pCode = reinterpret_cast<const uint8_t*>(*pHandle);
            if (!Memory::IsReadable(pCode, stPatch.vExpected.size()) || std::memcmp(pCode, stPatch.vExpected.data(), stPatch.vExpected.size()) != 0)
            {
                spdlog::warn("MG2: Room Patches : Script {} has changed. Skipping pitfall artwork.", stPatch.nScript);
                return;
            }
            s_mPitfalls[reinterpret_cast<Sprite*>(ctx.rcx)] = &stPatch;
            return;
        }
    }

    void DrawPitfallArtwork(SafetyHookContext& ctx)
    {
        auto* pSprite = reinterpret_cast<Sprite*>(ctx.rsi);
        auto* pPatch = FindPitfall(pSprite);
        if (!pPatch)
        {
            return;
        }
        const auto& stPatch = *pPatch;
        if (stPatch.vFrames.empty() || !IsRoom(stPatch.nArea, stPatch.nRoom) || pSprite->nSheet != MG2_SPRITE_SHEET_ROOM_OBJECTS || pSprite->nAnim != MG2_PITFALL_ANIMATION_FIRST + stPatch.nVariant)
        {
            return;
        }
        auto* pSource = reinterpret_cast<int*>(ctx.rsp + 0x68);
        if (ctx.r13 != reinterpret_cast<uintptr_t>(&pSprite->nFirstTrackFrame) + sizeof(int))
        {
            ctx.r8 = 0;
            pSource[2] = 0;
            return;
        }
        auto& stFrame = pPatch->vFrames[std::clamp(pSprite->nTime, 0, nPitfallFrames - 1)];
        *reinterpret_cast<DrawLayer**>(ctx.rsp + 0x48) = &stFrame.stImage;
        ctx.rbx = static_cast<uint32_t>(ctx.rbx - 2 + stPatch.nOffsetX);
        ctx.r9 = static_cast<uint32_t>(ctx.r9 + stPatch.nOffsetY);
        ctx.rcx = 0;
        ctx.r10 = 0;
        ctx.r8 = nPitfallWidth;
        ctx.rdx = nPitfallHeight;
        pSource[0] = 0;
        pSource[1] = 0;
        pSource[2] = nPitfallWidth;
        pSource[3] = nPitfallHeight;
    }

    void DrawArtwork(SafetyHookContext& ctx)
    {
        auto* pSprite = reinterpret_cast<Sprite*>(ctx.rsi);
        const auto* pDoor = FindDoorSprite(pSprite);
        if (!pDoor || !pDoor->pArtwork)
        {
            DrawPitfallArtwork(ctx);
            return;
        }
        auto& stPatch = *pDoor->pArtwork;
        const auto& stDefinition = stPatch.stDefinition;
        const auto& stArtwork = *stDefinition.pArtwork;
        if (!IsRoom(stDefinition.nArea, stDefinition.nRoom) || pSprite->nSheet != stArtwork.nSheet || pSprite->nAnim < stArtwork.nFirstAnimation || pSprite->nAnim > stArtwork.nLastAnimation)
        {
            return;
        }
        const int nTrack = static_cast<int>((ctx.r13 - reinterpret_cast<uintptr_t>(&pSprite->nFirstTrackFrame)) / sizeof(int));
        if (stArtwork.nTrack >= 0 && stArtwork.nTrack != nTrack)
        {
            return;
        }
        auto* pSource = reinterpret_cast<int*>(ctx.rsp + 0x68);
        const int nX = pSource[0] - stArtwork.nSourceX + nTrack * stArtwork.nTrackStride;
        const int nY = pSource[1] - stArtwork.nSourceY;
        int nHeight = pSource[3];
        if (stArtwork.nSourceHeight > 0)
        {
            if (pSource[3] != stArtwork.nSourceHeight)
            {
                return;
            }
            nHeight = std::min(nHeight, stArtwork.nHeight - nY);
        }
        if (nX < 0 || nY < 0 || pSource[2] <= 0 || nHeight <= 0 || nX + pSource[2] > stArtwork.nWidth || nY + nHeight > stArtwork.nHeight)
        {
            return;
        }
        *reinterpret_cast<DrawLayer**>(ctx.rsp + 0x48) = &stPatch.stImage;
        ctx.rcx = pSource[0] = nX;
        ctx.r10 = pSource[1] = nY;
        ctx.rdx = pSource[3] = nHeight;
    }

    void PatchDoorRecords(SafetyHookContext& ctx)
    {
        s_vDoorSprites.clear();
        s_mDoorRecords.clear();
        s_pOriginalDoors = reinterpret_cast<const uint8_t*>(ctx.rbx);
        s_vDoorTable.clear();
        const size_t nSize = static_cast<uint32_t>(ctx.rax);
        for (const auto& stDoor : s_vDoors)
        {
            if (stDoor.nArea != GM_Area)
            {
                continue;
            }
            const size_t nOffset = static_cast<size_t>(stDoor.nRecord) * nDoorRecordSize;
            if (nOffset + nDoorRecordSize > nSize || std::memcmp(s_pOriginalDoors + nOffset, stDoor.anExpected.data(), nDoorRecordSize) != 0)
            {
                spdlog::warn("MG2: Door Fixes : A{} door {} has changed. Skipping.", stDoor.nArea, stDoor.nRecord);
                continue;
            }
            if (s_vDoorTable.empty())
            {
                s_vDoorTable.assign(s_pOriginalDoors, s_pOriginalDoors + nSize);
            }
            auto* pRecord = s_vDoorTable.data() + nOffset;
            const int nX = (pRecord[1] << 8 | pRecord[2]) + stDoor.nWorldX;
            const int nY = (pRecord[3] << 8 | pRecord[4]) + stDoor.nWorldY;
            Memory::WriteBE16(pRecord + 1, static_cast<uint16_t>(nX));
            Memory::WriteBE16(pRecord + 3, static_cast<uint16_t>(nY));
            if (stDoor.nSpriteType != -2)
            {
                pRecord[12] = static_cast<uint8_t>(stDoor.nSpriteType);
            }
            s_mDoorRecords[stDoor.nRecord] = { stDoor.nVisualX, stDoor.nVisualY };
        }
        if (!s_vDoorTable.empty())
        {
            ctx.rbx = reinterpret_cast<uintptr_t>(s_vDoorTable.data());
            *reinterpret_cast<uintptr_t*>(ctx.rbp - 0x11) = ctx.rbx;
        }
    }

    void RegisterDoorArtwork(SafetyHookContext& ctx)
    {
        const int nRecord = static_cast<int>(ctx.r14);
        DoorSprite stDoor { *reinterpret_cast<const uintptr_t*>(ctx.r12), {}, nullptr }; // r12 = record's slot in the door handle table.
        if (const auto itOffset = s_mDoorRecords.find(nRecord); itOffset != s_mDoorRecords.end())
        {
            stDoor.stOffset = itOffset->second;
        }
        for (auto& stPatch : s_vDoorArtwork)
        {
            const auto& stDefinition = stPatch.stDefinition;
            if (stDefinition.nArea != GM_Area || stDefinition.nRecord != nRecord)
            {
                continue;
            }
            const auto* pRecord = s_pOriginalDoors + static_cast<size_t>(nRecord) * nDoorRecordSize;
            if (std::memcmp(pRecord, stDefinition.anExpected.data(), nDoorRecordSize) != 0)
            {
                spdlog::warn("MG2: Door Sprites : A{} door {} has changed. Skipping artwork.", stDefinition.nArea, stDefinition.nRecord);
                break;
            }
            stDoor.pArtwork = &stPatch;
            break;
        }
        if (stDoor.pArtwork || stDoor.stOffset.nX != 0 || stDoor.stOffset.nY != 0)
        {
            s_vDoorSprites.push_back(stDoor);
        }
    }

    void __fastcall DrawSprite_hook(Sprite* pSprite, int nMode)
    {
        DrawOffset stOffset {};
        if (const auto* pDoor = FindDoorSprite(pSprite))
        {
            stOffset = pDoor->stOffset;
        }
        if (const auto* pPatch = FindPitfall(pSprite))
        {
            const auto& stPatch = *pPatch;
            if (stPatch.vFrames.empty() && IsRoom(stPatch.nArea, stPatch.nRoom) && pSprite->nSheet == MG2_SPRITE_SHEET_ROOM_OBJECTS && pSprite->nAnim == MG2_PITFALL_ANIMATION_FIRST + stPatch.nVariant)
            {
                stOffset = { stPatch.nOffsetX, stPatch.nOffsetY };
            }
        }
        if (stOffset.nX == 0 && stOffset.nY == 0)
        {
            s_hDrawSprite.call<void>(pSprite, nMode);
            return;
        }
        const int nX = pSprite->nX;
        const int nY = pSprite->nY;
        pSprite->nX += stOffset.nX;
        pSprite->nY += stOffset.nY;
        s_hDrawSprite.call<void>(pSprite, nMode);
        pSprite->nX = nX;
        pSprite->nY = nY;
    }
}

void MG2_RoomPatches::AddDoor(const DoorDefinition& stDefinition)
{
    s_vDoors.push_back(stDefinition);
}

bool MG2_RoomPatches::AddDoorArtwork(const DoorArtworkDefinition& stDefinition)
{
    const auto* pArtwork = stDefinition.pArtwork;
    if (!pArtwork || stDefinition.anExpected.size() != nDoorRecordSize || pArtwork->nWidth <= 0 || pArtwork->nHeight <= 0 || pArtwork->anPixels.size() != static_cast<size_t>(pArtwork->nWidth) * pArtwork->nHeight)
    {
        spdlog::error("MG2: Door Sprites : Invalid bundled artwork for A{} R{}.", stDefinition.nArea, stDefinition.nRoom);
        return false;
    }
    s_vDoorArtwork.push_back({ stDefinition });
    return true;
}

bool MG2_RoomPatches::AddPitfall(const PitfallDefinition& stDefinition)
{
    if (stDefinition.anExpected.empty() || (!stDefinition.anFrames.empty() && stDefinition.anFrames.size() != nPitfallFrames * nPitfallWidth * nPitfallHeight) || (stDefinition.anFrames.empty() && (stDefinition.bAdd || (stDefinition.nOffsetX == 0 && stDefinition.nOffsetY == 0))))
    {
        spdlog::error("MG2: Pitfalls : Invalid bundled artwork for A{} R{}.", stDefinition.nArea, stDefinition.nRoom);
        return false;
    }
    PitfallPatch stPatch {};
    stPatch.nArea = stDefinition.nArea;
    stPatch.nRoom = stDefinition.nRoom;
    stPatch.nX = stDefinition.nX;
    stPatch.nY = stDefinition.nY;
    stPatch.nVariant = stDefinition.nVariant;
    stPatch.nRecord = stDefinition.nRecord;
    stPatch.nScript = stDefinition.nScript;
    stPatch.nOffsetX = stDefinition.nOffsetX;
    stPatch.nOffsetY = stDefinition.nOffsetY;
    stPatch.bAdd = stDefinition.bAdd;
    stPatch.vExpected.assign(stDefinition.anExpected.begin(), stDefinition.anExpected.end());
    stPatch.vFrames.resize(stDefinition.anFrames.empty() ? 0 : nPitfallFrames);
    for (size_t nFrame = 0; nFrame < stPatch.vFrames.size(); nFrame++)
    {
        auto& anPixels = stPatch.vFrames[nFrame].anPixels;
        std::copy_n(stDefinition.anFrames.data() + nFrame * anPixels.size(), anPixels.size(), anPixels.data());
    }
    s_vPitfalls.push_back(std::move(stPatch));
    return true;
}

void MG2_RoomPatches::Initialize()
{
    if (s_vPitfalls.empty() && s_vDoorArtwork.empty() && s_vDoors.empty())
    {
        return;
    }
    for (auto& stPatch : s_vPitfalls)
    {
        for (auto& stFrame : stPatch.vFrames)
        {
            stFrame.pPixels = reinterpret_cast<uintptr_t>(stFrame.anPixels.data());
            stFrame.stImage.hPixels = reinterpret_cast<uintptr_t>(&stFrame.pPixels);
            stFrame.stImage.nWidth = nPitfallWidth;
            stFrame.stImage.nHeight = nPitfallHeight;
        }
    }

    for (auto& stPatch : s_vDoorArtwork)
    {
        const auto& stArtwork = *stPatch.stDefinition.pArtwork;
        stPatch.pPixels = reinterpret_cast<uintptr_t>(stArtwork.anPixels.data());
        stPatch.stImage.hPixels = reinterpret_cast<uintptr_t>(&stPatch.pPixels);
        stPatch.stImage.nWidth = stArtwork.nWidth;
        stPatch.stImage.nHeight = stArtwork.nHeight;
    }

    //BuildDoors+5E
    uint8_t* pBuildDoors = Memory::PatternScan(mg2Module, "8B C8 45 33 C0 B8 ?? ?? ?? ?? F7 E9 03 D1 C1 FA", "MG2: Room Patches : BuildDoors()+0x5E : apply door fixes");
    //BuildDoors+39B
    uint8_t* pRegisterDoor = Memory::PatternScan(mg2Module, "8B 45 ?? 3B 45 ?? 75 ?? 8B 4D ?? E8 ?? ?? ?? ?? 8B 4D ?? F7 D8", "MG2: Room Patches : BuildDoors()+0x39B : register door artwork");
    const bool bObjectPatches = std::any_of(s_vPitfalls.begin(), s_vPitfalls.end(), [](const PitfallPatch& stPatch) { return stPatch.nScript < 0; });
    const bool bScriptArtwork = std::any_of(s_vPitfalls.begin(), s_vPitfalls.end(), [](const PitfallPatch& stPatch) { return stPatch.nScript >= 0; });
    //BuildRoomObjects+3A
    uint8_t* pObjectRecords = bObjectPatches ? Memory::PatternScan(mg2Module, "E8 ?? ?? ?? ?? BB ?? ?? ?? ?? 48 8D 0D", "MG2: Room Patches : BuildRoomObjects()+0x3A : append room objects") : nullptr;
    const bool bPitfallArtwork = !s_vPitfalls.empty();
    const bool bArtwork = bPitfallArtwork || !s_vDoorArtwork.empty();
    //BuildRoomObjects+4F8
    uint8_t* pRegisterPitfall = bPitfallArtwork ? Memory::PatternScan(mg2Module, "?? ?? ?? 8B 6F ?? E8 ?? ?? ?? ?? ?? ?? ?? 48 63 D8 E8", "MG2: Room Patches : BuildRoomObjects()+0x4F8 : register pitfall artwork") : nullptr;
    //DrawSprite+35F
    uint8_t* pDrawArtwork = bArtwork ? Memory::PatternScan(mg2Module, "41 3B DF 7D ?? 8B C3 41 2B C7 41 03 C0 44 8B C0 89 44 24", "MG2: Room Patches : DrawSprite()+0x35F : replace artwork before clipping") : nullptr;
    if ((bScriptArtwork && !pLoadedScripts) || (bPitfallArtwork && !pRegisterPitfall) || (bArtwork && !pDrawArtwork) || (bObjectPatches && (!pObjectRecords || !MG2_Gamefuncs::pfnGetChunkEntryData || !MG2_Gamefuncs::pfnGetChunkEntrySize)) || !pBuildDoors || !pRegisterDoor || !MG2_Gamefuncs::pfnBuildDoors || !MG2_Gamefuncs::pfnDrawSprite || !MG2_Gamefuncs::pfnInitSprite || !MG2_Gamefuncs::pfnGetObjectSprite || !pScriptGlobals)
    {
        spdlog::error("MG2: Room Patches : Missing room or sprite functions. Skipping.");
    }
    else
    {
        if (bObjectPatches)
        {
            s_hGetChunkEntryData = safetyhook::create_inline(MG2_Gamefuncs::pfnGetChunkEntryData, GetChunkEntryData_hook);
            LOG_HOOK(s_hGetChunkEntryData, "MG2: Room Patches : GetChunkEntryData()")
            s_hGetChunkEntrySize = safetyhook::create_inline(MG2_Gamefuncs::pfnGetChunkEntrySize, GetChunkEntrySize_hook);
            LOG_HOOK(s_hGetChunkEntrySize, "MG2: Room Patches : GetChunkEntrySize()")
            s_hBuildObjects = safetyhook::create_mid(pObjectRecords, PatchObjectRecords);
            LOG_HOOK(s_hBuildObjects, "MG2: Room Patches : BuildRoomObjects()+0x3A")
        }
        if (bPitfallArtwork)
        {
            s_hRegisterPitfall = safetyhook::create_mid(pRegisterPitfall, RegisterPitfall);
            LOG_HOOK(s_hRegisterPitfall, "MG2: Room Patches : BuildRoomObjects()+0x4F8")
        }
        if (bArtwork)
        {
            s_hDrawArtwork = safetyhook::create_mid(pDrawArtwork, DrawArtwork);
            LOG_HOOK(s_hDrawArtwork, "MG2: Room Patches : DrawSprite()+0x35F")
        }
        s_hBuildDoors = safetyhook::create_mid(pBuildDoors, PatchDoorRecords);
        LOG_HOOK(s_hBuildDoors, "MG2: Room Patches : BuildDoors()+0x5E")
        s_hRegisterDoor = safetyhook::create_mid(pRegisterDoor, RegisterDoorArtwork);
        LOG_HOOK(s_hRegisterDoor, "MG2: Room Patches : BuildDoors()+0x39B")
        s_hInitSprite = safetyhook::create_mid(MG2_Gamefuncs::pfnInitSprite, [](SafetyHookContext& ctx)
        {
            s_mPitfalls.erase(reinterpret_cast<Sprite*>(ctx.rcx));
            RegisterScriptPitfall(ctx);
        });
        LOG_HOOK(s_hInitSprite, "MG2: Room Patches : InitSprite()")
        s_hDrawSprite = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprite, DrawSprite_hook);
        LOG_HOOK(s_hDrawSprite, "MG2: Room Patches : DrawSprite()")
        if ((bPitfallArtwork && !s_hRegisterPitfall) || (bArtwork && !s_hDrawArtwork) || (bObjectPatches && (!s_hBuildObjects || !s_hGetChunkEntryData || !s_hGetChunkEntrySize)) || !s_hBuildDoors || !s_hRegisterDoor || !s_hInitSprite || !s_hDrawSprite)
        {
            s_hDrawArtwork.reset();
            s_hRegisterPitfall.reset();
            s_mPitfalls.clear();
            s_vObjectPitfalls.clear();
            s_hBuildObjects.reset();
            s_hGetChunkEntrySize.reset();
            s_hGetChunkEntryData.reset();
            s_hDrawSprite.reset();
            s_hInitSprite.reset();
            s_hRegisterDoor.reset();
            s_hBuildDoors.reset();
            s_vDoorSprites.clear();
            spdlog::error("MG2: Room Patches : Incomplete hook setup. Disabled room patches.");
        }
        else
        {
            if (!s_vDoors.empty())
            {
                spdlog::info("MG2: Door Fixes : Initialized with {} bundled corrections.", s_vDoors.size());
            }
            if (!s_vDoorArtwork.empty())
            {
                spdlog::info("MG2: Door Sprites : Initialized with {} bundled restorations.", s_vDoorArtwork.size());
            }
            if (!s_vPitfalls.empty())
            {
                spdlog::info("MG2: Pitfalls : Initialized with {} bundled restorations.", s_vPitfalls.size());
            }
        }
    }
}
