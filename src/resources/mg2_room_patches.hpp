#pragma once

namespace MG2_RoomPatches
{
    struct PitfallDefinition
    {
        int nArea;
        int nRoom;
        int nX;
        int nY;
        int nVariant;
        int nRecord;
        int nScript;
        int nOffsetX;
        int nOffsetY;
        bool bAdd;
        std::span<const uint8_t> anExpected;
        std::span<const uint8_t> anFrames;
    };

    struct SpriteArtworkDefinition
    {
        int nSheet;
        int nFirstAnimation;
        int nLastAnimation;
        int nSourceX;
        int nSourceY;
        int nWidth;
        int nHeight;
        std::span<const uint8_t> anPixels;
        int nTrack = -1;
        int nTrackStride = 0;
        int nSourceHeight = 0;
    };

    struct DoorDefinition
    {
        int nArea;
        int nRecord;
        int nWorldX;
        int nWorldY;
        int nVisualX;
        int nVisualY;
        int nSpriteType;
        std::array<uint8_t, 15> anExpected;
    };

    struct DoorArtworkDefinition
    {
        int nArea;
        int nRoom;
        int nRecord;
        std::span<const uint8_t> anExpected;
        const SpriteArtworkDefinition* pArtwork;
    };

    void AddDoor(const DoorDefinition& stDefinition);
    bool AddDoorArtwork(const DoorArtworkDefinition& stDefinition);
    bool AddPitfall(const PitfallDefinition& stDefinition);
    void Initialize();
}
