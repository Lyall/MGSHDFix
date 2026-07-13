#include "stdafx.h"
#include "mgs2_tanker_snake_snap.hpp"

#include "common.hpp"
#include "gamevars.hpp"
#include "game_stages.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

#include <unordered_map>
#include <cmath>

namespace
{
    // Only the crane shot puts the camera this high; it peaks at 138,024.
    constexpr float kCraneY = 130000.0f;
    // Copies wait at the world origin until their take starts.
    constexpr float kParkedRadius = 2000.0f;
    // A copy this far from its own start has been walking a while, so it belongs to the last shot.
    constexpr float kLeftover = 10000.0f;

    SafetyHookMid gFrameHook {};
    SafetyHookMid gCommandHook {};
    SafetyHookMid gMotionPackHook {};

    struct DemoObj
    {
        float pos[3] {};       // where the demo last put it
        float start[3] {};     // where its take began
        float rewind[3] {};
        bool  visible = false;
        bool  placed = false;
        bool  rewound = false;
    };

    std::unordered_map<int, DemoObj> gObjs;
    bool gInCraneShot = false;

    float Length(const float* v)
    {
        return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    }

    // DM_Packet_Frame( DM_WORK *work, DEMO_FRAME *packet )
    void PacketFrame_Detour(SafetyHookContext& ctx)
    {
        if (!g_GameVars.IsStage(MGS2Stages::D00T))
        {
            return;
        }

        const bool crane = *reinterpret_cast<const float*>(ctx.rdx + 0x24) > kCraneY;   // camera_pos.vy
        if (crane && !gInCraneShot)
        {
            // The cut. Anyone still on screen is left over from the shot before it.
            for (auto& [id, obj] : gObjs)
            {
                if (!obj.visible || !obj.placed)
                {
                    continue;
                }
                for (int i = 0; i < 3; i++)
                {
                    obj.rewind[i] = obj.start[i] - obj.pos[i];
                }
                obj.rewound = Length(obj.rewind) > kLeftover;
            }
        }
        else if (!crane && gInCraneShot)
        {
            for (auto& [id, obj] : gObjs)
            {
                obj.rewound = false;
            }
        }
        gInCraneShot = crane;
    }

    // DM_Packet_Command( DM_WORK *work, DEMO_COMMAND *packet )
    void PacketCommand_Detour(SafetyHookContext& ctx)
    {
        if (!g_GameVars.IsStage(MGS2Stages::D00T))
        {
            return;
        }

        const uint8_t* packet = reinterpret_cast<const uint8_t*>(ctx.rdx);
        if (*reinterpret_cast<const int*>(packet + 0x10) != 0)   // DEMO_COMMAND_OBJECT_VISIBLE
        {
            return;
        }

        DemoObj& obj = gObjs[*reinterpret_cast<const int*>(packet + 0x14)];
        obj.visible = (*reinterpret_cast<const int*>(packet + 0x18) == 0);   // param1: 0 shows, channel bits hide
        if (!obj.visible)
        {
            obj.rewound = false;   // the swap finally happened; stop covering for this copy
        }
    }

    // DM_Packet_MotionPack( DM_WORK *work, DEMO_MOTION_PACK *packet ) - runs before the game reads it
    void PacketMotionPack_Detour(SafetyHookContext& ctx)
    {
        if (!g_GameVars.IsStage(MGS2Stages::D00T))
        {
            return;
        }

        uint8_t* packet = reinterpret_cast<uint8_t*>(ctx.rdx);
        const int lists = *reinterpret_cast<const int*>(packet + 0x10);
        if (lists <= 0 || lists > 4096)
        {
            return;
        }

        // The game flags packets it has already byte-swapped, so one can come round twice. Don't
        // let the rewind below stack.
        static uint8_t* lastPacket = nullptr;
        static uint64_t lastFrame = ~0ull;
        if (packet == lastPacket && g_D3D11Hooks.FrameCount == lastFrame)
        {
            return;
        }
        lastPacket = packet;
        lastFrame = g_D3D11Hooks.FrameCount;

        for (int i = 0; i < lists; i++)
        {
            const uint8_t* info = packet + 0x20 + i * 12;   // DEMO_MOTION_INFO
            if (!(*reinterpret_cast<const uint16_t*>(info + 4) & 0x0001))   // DEMO_MOTIONPACK_MOV
            {
                continue;
            }

            DemoObj& obj = gObjs[*reinterpret_cast<const int*>(info)];
            float* pos = reinterpret_cast<float*>(packet + *reinterpret_cast<const uint16_t*>(info + 8));

            // A copy leaves the origin for the bridge on the frame its take starts.
            const bool placed = Length(pos) > kParkedRadius;
            if (placed && !obj.placed)
            {
                memcpy(obj.start, pos, sizeof(obj.start));
            }
            obj.placed = placed;
            memcpy(obj.pos, pos, sizeof(obj.pos));

            if (obj.rewound)
            {
                for (int axis = 0; axis < 3; axis++)
                {
                    pos[axis] += obj.rewind[axis];
                }
            }
        }
    }
}

void MGS2TankerSnakeSnap::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    // Every packet handler opens with the same endian-swap test, so the register moves either side
    // of it are what pin these down. Stack size and the branch over the swap are compiler noise.
    uint8_t* frame = Memory::PatternScan(baseModule,
        "8B 02 4C 8B C2 4C 8B C9 0F BA E0 1E 72 ?? 0F BA E8 1E 89 02 48 63 51 10",
        "MGS2: Tanker Snake Snap: mode/demo/demo_frm.c -> DM_Packet_Frame()");
    uint8_t* command = Memory::PatternScan(baseModule,
        "40 53 48 83 EC ?? 8B 02 4C 8B CA 48 8B D9 0F BA E0 1E 72 ?? 0F BA E8 1E 89 02 8B 52 10 85 D2",
        "MGS2: Tanker Snake Snap: mode/demo/demo_cmd.c -> DM_Packet_Command()");
    uint8_t* motionPack = Memory::PatternScan(baseModule,
        "48 89 54 24 10 48 89 4C 24 08 55 41 55 41 56 41 57 48 83 EC ?? 8B 02 4C 8B FA 4C 8B E9",
        "MGS2: Tanker Snake Snap: mode/demo/demo_mtn.c -> DM_Packet_MotionPack()");

    if (!frame || !command || !motionPack)
    {
        spdlog::info("MGS2: Tanker Snake Snap - demo packet patternscan failure; fix disabled.");
        return;
    }

    gFrameHook = safetyhook::create_mid(frame, PacketFrame_Detour);
    LOG_HOOK(gFrameHook, "MGS2: Tanker Snake Snap: mode/demo/demo_frm.c -> DM_Packet_Frame()");

    gCommandHook = safetyhook::create_mid(command, PacketCommand_Detour);
    LOG_HOOK(gCommandHook, "MGS2: Tanker Snake Snap: mode/demo/demo_cmd.c -> DM_Packet_Command()");

    gMotionPackHook = safetyhook::create_mid(motionPack, PacketMotionPack_Detour);
    LOG_HOOK(gMotionPackHook, "MGS2: Tanker Snake Snap: mode/demo/demo_mtn.c -> DM_Packet_MotionPack()");
}
