#include "stdafx.h"
#include "mg2_elevators.hpp"
#include "mg2_elevator_sprites.hpp"

#include "common.hpp"
#include "mg2_linkvarbuf.hpp"

using namespace MG2_LinkVarBuf;

namespace
{
    constexpr int nMoveTicks = 32;
    constexpr int nArrivalWaitTicks = 32;
    constexpr int nPortCallTicks = 64;
    constexpr int nLampWidth = 32;
    constexpr int nLampHeight = 8;
    constexpr int nLampY = 124;
    constexpr int anLampX[] = { 48, 112 };
    struct ElevatorRoute
    {
        int nStopCount;
        int anFloors[4];
        int nInitialTimer;
        int nInitialFloor;
    };

    constexpr ElevatorRoute astRoutes[] =
    {
        { 3, { MG2_ELEVATOR_FLOOR_B2, MG2_ELEVATOR_FLOOR_B1, MG2_ELEVATOR_FLOOR_2F }, 32, MG2_ELEVATOR_FLOOR_B2 },
        { 4, { MG2_ELEVATOR_FLOOR_1F, MG2_ELEVATOR_FLOOR_2F, MG2_ELEVATOR_FLOOR_3F, MG2_ELEVATOR_FLOOR_4F }, 16, MG2_ELEVATOR_FLOOR_1F },
        { 2, { MG2_ELEVATOR_FLOOR_B3, MG2_ELEVATOR_FLOOR_4F }, 8, MG2_ELEVATOR_FLOOR_B3 },
        { 2, { MG2_ELEVATOR_FLOOR_1F, MG2_ELEVATOR_FLOOR_10F }, 4, MG2_ELEVATOR_FLOOR_1F },
        { 3, { MG2_ELEVATOR_FLOOR_B1, MG2_ELEVATOR_FLOOR_1F, MG2_ELEVATOR_FLOOR_20F }, 19, MG2_ELEVATOR_FLOOR_B1 },
        { 2, { MG2_ELEVATOR_FLOOR_1F, MG2_ELEVATOR_FLOOR_30F }, 22, MG2_ELEVATOR_FLOOR_1F },
        { 2, { MG2_ELEVATOR_FLOOR_B3, MG2_ELEVATOR_FLOOR_1F }, 25, MG2_ELEVATOR_FLOOR_B3 },
        { 2, { MG2_ELEVATOR_FLOOR_B3, MG2_ELEVATOR_FLOOR_1F }, 15, MG2_ELEVATOR_FLOOR_B3 },
        { 2, { MG2_ELEVATOR_FLOOR_B3, MG2_ELEVATOR_FLOOR_1F }, 24, MG2_ELEVATOR_FLOOR_B3 },
        { 2, { MG2_ELEVATOR_FLOOR_B3, MG2_ELEVATOR_FLOOR_1F }, 255, MG2_ELEVATOR_FLOOR_1F },
    };

    struct ElevatorCar
    {
        int nStop = 0;
        int nFloor = MG2_ELEVATOR_FLOOR_B3;
        int nTimer = 0;
    };

    std::array<ElevatorCar, std::size(astRoutes)> s_astCars {};
    bool s_bInitialized = false;
    bool s_bLampRoom = false;
    const MG2_ElevatorSprites::UnlitLamp* s_pUnlitLamp = nullptr;
    uint32_t s_nRandom = 1;
    int s_nWaitingCar = -1;
    int s_nCallStartStop = 0;
    bool s_bArrivalChimed = false;
    int s_nArrivalTicks = 0;
    int s_nStateTicks = 0;
    int s_nPreviousState = MG2_ELEVATOR_IDLE;
    SafetyHookInline s_hScriptRun {};
    SafetyHookInline s_hDrawSprites {};

    constexpr uint8_t aanLampPixels[nLampHeight][nLampWidth] =
    {
        { 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240 },
        { 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126 },
        { 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126 },
        { 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 },
        { 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 125 },
        { 253, 124, 124, 124, 240, 240, 240, 124, 240, 240, 240, 124, 240, 240, 240, 124, 240, 240, 240, 124, 240, 240, 240, 124, 240, 240, 240, 124, 124, 124, 124, 125 },
        { 124, 128, 128, 128, 240, 240, 240, 128, 240, 240, 240, 128, 240, 240, 240, 128, 240, 240, 240, 128, 240, 240, 240, 128, 240, 240, 240, 128, 128, 128, 128, 125 },
        { 128, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 126, 125 },
    };

    int FindStop(int nCar, int nFloor)
    {
        const auto& stRoute = astRoutes[nCar];
        for (int nStop = 0; nStop < stRoute.nStopCount; nStop++)
        {
            if (stRoute.anFloors[nStop] == nFloor)
            {
                return nStop;
            }
        }
        return -1;
    }

    void ResetCars()
    {
        for (size_t nCar = 0; nCar < s_astCars.size(); nCar++)
        {
            s_astCars[nCar] = { 0, astRoutes[nCar].nInitialFloor, astRoutes[nCar].nInitialTimer };
        }
        s_nWaitingCar = -1;
        s_nStateTicks = 0;
        s_nArrivalTicks = 0;
        s_nRandom = static_cast<uint32_t>(GetTickCount64()) | 1;
        s_bInitialized = true;
		if(g_Logging.bVerboseLogging)
		{
		    spdlog::info("MG2: Elevators : Car simulation initialized.");
		}
    }

    bool RandomUp()
    {
        // Intentionally kept seperate from vanilla enemy & item rng.
        s_nRandom ^= s_nRandom << 13;
        s_nRandom ^= s_nRandom >> 17;
        s_nRandom ^= s_nRandom << 5;
        return (s_nRandom & 1) != 0;
    }

    void SetCarStop(int nCar, int nStop)
    {
        auto& stCar = s_astCars[nCar];
        stCar.nStop = nStop;
        stCar.nFloor = astRoutes[nCar].anFloors[nStop];
    }

    void TickCars()
    {
        const int nSelected = GM_ElevatorCar;
        const int nState = GM_ElevatorState;
        const bool bSelected = nSelected >= 0 && nSelected < static_cast<int>(s_astCars.size()) && nState != MG2_ELEVATOR_IDLE;
        const int nTarget = bSelected ? FindStop(nSelected, GM_ElevatorFloor) : -1;
        const bool bWaiting = bSelected && nState == MG2_ELEVATOR_CALLED && nTarget >= 0;
        if (bWaiting && s_nWaitingCar != nSelected)
        {
            s_nWaitingCar = nSelected;
            s_nCallStartStop = s_astCars[nSelected].nStop;
            s_bArrivalChimed = false;
            s_nArrivalTicks = 0;
        }
        else if (!bWaiting)
        {
            s_nWaitingCar = -1;
        }

        for (int nCar = 0; nCar < static_cast<int>(s_astCars.size()); nCar++)
        {
            auto& stCar = s_astCars[nCar];
            const bool bHeld = bSelected && nSelected == nCar;
            if (nCar == MG2_ELEVATOR_CAR_10 && !bHeld)
            {
                continue;
            }
            if (bHeld && !bWaiting)
            {
                if (nTarget >= 0)
                {
                    SetCarStop(nCar, nTarget);
                }
                if (--stCar.nTimer <= 0)
                {
                    stCar.nTimer = nMoveTicks;
                }
                continue;
            }
            if (bHeld && !MG2_Elevators::bRestoreTimings)
            {
                const int nElapsed = std::clamp(static_cast<int>(GM_ElevatorTimer) + 1, 0, nPortCallTicks);
                const int nDistance = nTarget - s_nCallStartStop;
                SetCarStop(nCar, s_nCallStartStop + nDistance * nElapsed / nPortCallTicks);
                continue;
            }
            if (--stCar.nTimer > 0)
            {
                continue;
            }
            stCar.nTimer = nMoveTicks;
            const int nStep = bHeld ? (nTarget > stCar.nStop) - (nTarget < stCar.nStop) : RandomUp() ? 1 : -1;
            SetCarStop(nCar, std::clamp(stCar.nStop + nStep, 0, astRoutes[nCar].nStopCount - 1));
        }

        if (bWaiting && MG2_Elevators::bRestoreTimings)
        {
            if (s_astCars[nSelected].nFloor != GM_ElevatorFloor)
            {
                GM_ElevatorTimer = -1;
            }
            else
            {
                if (!s_bArrivalChimed)
                {
                    MG2_Gamefuncs::pfnRequestSound(MG2_SOUND_ELEVATOR_ARRIVAL);
                    s_bArrivalChimed = true;
                }
                else
                {
                    s_nArrivalTicks++;
                }
                // Skip the early chime and retain the door opening transition.
                GM_ElevatorTimer = s_nArrivalTicks >= nArrivalWaitTicks ? nPortCallTicks - 1 : -1;
            }
        }
    }

    void UpdateSequenceTimer()
    {
        const int nState = GM_ElevatorState;
        if (nState != s_nPreviousState)
        {
            s_nStateTicks = 0;
        }
        const int nElapsed = ++s_nStateTicks;
        int nTimer = 0;
        switch (nState)
        {
        case MG2_ELEVATOR_OPENING:
        case MG2_ELEVATOR_CLOSING:
            nTimer = std::min(nElapsed * 2, 16);
            break;
        case MG2_ELEVATOR_OPEN:
            nTimer = std::min(nElapsed * 4, 256);
            break;
        case MG2_ELEVATOR_DEPARTING:
            // 16 ticks to close, then a 16-tick pause before moving.
            nTimer = std::min(nElapsed / 2, 16);
            break;
        case MG2_ELEVATOR_MOVING:
        {
            const int nCar = GM_ElevatorCar;
            const int nRideTicks = nCar >= MG2_ELEVATOR_CAR_4 && nCar <= MG2_ELEVATOR_CAR_6 ? 12 : 24;
            const int nChimeTick = nRideTicks - 8;
            // mc script chimes at 8 and advances a floor at 16.
            nTimer = nElapsed < nChimeTick ? nElapsed * 8 / nChimeTick : std::min(8 + nElapsed - nChimeTick, 16);
            break;
        }
        case MG2_ELEVATOR_ARRIVING:
            // 8 ticks settling, 32 waiting, then 16 opening the inside doors.
            nTimer = nElapsed <= 8 ? nElapsed - 1 : nElapsed <= 40 ? 7 : std::min(8 + (nElapsed - 40) / 2, 16);
            break;
        default:
            return;
        }
        GM_ElevatorTimer = nTimer - 1; // Script 135 increments before processing current state.
    }

    void __fastcall ScriptRun_hook(int* pScript)
    {
        const bool bElevators = IsScript(pScript, MG2_SCRIPT_ELEVATORS);
        const bool bStarting = bElevators && pScript[MG2_SCRIPT_PROG_COUNT] == 0;
        if (bStarting)
        {
            ResetCars();
        }
        else if (bElevators && s_bInitialized)
        {
            TickCars();
            if (MG2_Elevators::bRestoreTimings)
            {
                UpdateSequenceTimer();
            }
        }
        const int nPreviousState = GM_ElevatorState;
        const int nPreviousFloor = GM_ElevatorFloor;
        s_hScriptRun.call<void>(pScript);
        if (bElevators && (GM_ElevatorState != nPreviousState || GM_ElevatorFloor != nPreviousFloor))
        {
            s_nStateTicks = 0;
        }
        if (bElevators)
        {
            s_nPreviousState = GM_ElevatorState;
        }
        if (bElevators && !bStarting && GM_ElevatorState != MG2_ELEVATOR_IDLE && GM_ElevatorState != MG2_ELEVATOR_CALLED)
        {
            const int nCar = GM_ElevatorCar;
            if (nCar >= 0 && nCar < static_cast<int>(s_astCars.size()))
            {
                const int nStop = FindStop(nCar, GM_ElevatorFloor);
                if (nStop >= 0)
                {
                    SetCarStop(nCar, nStop);
                }
            }
        }
    }

    void DrawLamp(int nCar)
    {
        const int nFloor = s_astCars[nCar].nFloor;
        if (nFloor < MG2_ELEVATOR_FLOOR_B2 || nFloor > MG2_ELEVATOR_FLOOR_4F)
        {
            return;
        }
        const int nLitX = 5 + (nFloor - MG2_ELEVATOR_FLOOR_B2) * 4;
        for (int nY = 0; nY < nLampHeight; nY++)
        {
            for (int nX = 0; nX < nLampWidth;)
            {
                const int nColour = nFloor == MG2_ELEVATOR_FLOOR_4F && nY == 0 && nX >= 24 ? 125 : aanLampPixels[nY][nX];
                const int nStart = nX++;
                while (nX < nLampWidth && (nFloor == MG2_ELEVATOR_FLOOR_4F && nY == 0 && nX >= 24 ? 125 : aanLampPixels[nY][nX]) == nColour)
                {
                    nX++;
                }
                const int anRectangle[] = { anLampX[nCar] + nStart, nLampY + nY, nX - nStart, 1 };
                MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_ROOM, nColour);
                MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_ROOM, anRectangle);
            }
        }
        const int anLight[] = { anLampX[nCar] + nLitX, nLampY + 5, 1, 2 };
        MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_ROOM, 250);
        MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_ROOM, anLight);
    }

    void DrawPixelRow(std::span<const uint8_t> anPixels, int nLeft, int nTop)
    {
        const int nWidth = static_cast<int>(anPixels.size());
        for (int nX = 0; nX < nWidth;)
        {
            const int nColour = anPixels[nX];
            const int nStart = nX++;
            while (nX < nWidth && anPixels[nX] == nColour)
            {
                nX++;
            }
            const int anRectangle[] = { nLeft + nStart, nTop, nX - nStart, 1 };
            MG2_Gamefuncs::pfnSetDrawColor(MG2_LAYER_ROOM, nColour);
            MG2_Gamefuncs::pfnLayerFillRect(MG2_LAYER_ROOM, anRectangle);
        }
    }

    void __fastcall DrawSprites_hook()
    {
        if (GM_Scene == MG2_SCENE_GAMEPLAY && GM_InsideElevator == 0)
        {
            if (s_bInitialized && s_bLampRoom)
            {
                DrawLamp(MG2_ELEVATOR_CAR_1);
                DrawLamp(MG2_ELEVATOR_CAR_2);
            }
            if (s_pUnlitLamp)
            {
                DrawPixelRow(MG2_ElevatorSprites::anUnlitLampPixels, s_pUnlitLamp->nX, s_pUnlitLamp->nY);
            }
        }
        s_hDrawSprites.call<void>();
    }
}

void MG2_Elevators::HandleLevelTransition()
{
    s_pUnlitLamp = nullptr;
    for (const auto& stLamp : MG2_ElevatorSprites::astUnlitLamps)
    {
        if (IsRoom(stLamp.nArea, stLamp.nRoom))
        {
            s_pUnlitLamp = &stLamp;
            break;
        }
    }
    s_bLampRoom = MG2_LinkVarBuf::IsAnyRoom({
        { 1, 11 },
        { 2, 4 },
        { 3, 10 },
        { 4, 12 },
        { 5, 4 },
        { 6, 7 },
    });
}

void MG2_Elevators::Initialize()
{
    if (!bRestoreLamps && !bRestoreTimings)
    {
        return;
    }
    if (!MG2_LinkVarBuf::pScriptGlobals || !MG2_LinkVarBuf::pLoadedScripts || !MG2_LinkVarBuf::pRunningScriptCode || !MG2_Gamefuncs::pfnScriptRun || (bRestoreTimings && !MG2_Gamefuncs::pfnRequestSound) || (bRestoreLamps && (!MG2_Gamefuncs::pfnDrawSprites || !MG2_Gamefuncs::pfnSetDrawColor || !MG2_Gamefuncs::pfnLayerFillRect)))
    {
        spdlog::error("MG2: Elevators : Missing script or drawing functions. Skipping.");
    }
    else
    {
        s_hScriptRun = safetyhook::create_inline(MG2_Gamefuncs::pfnScriptRun, ScriptRun_hook);
        LOG_HOOK(s_hScriptRun, "MG2: Elevators : ScriptRun()")
        if (s_hScriptRun && bRestoreLamps)
        {
            s_hDrawSprites = safetyhook::create_inline(MG2_Gamefuncs::pfnDrawSprites, DrawSprites_hook);
            LOG_HOOK(s_hDrawSprites, "MG2: Elevators : DrawSprites()")
        }
        if (!s_hScriptRun || (bRestoreLamps && !s_hDrawSprites))
        {
            s_hDrawSprites.reset();
            s_hScriptRun.reset();
            spdlog::error("MG2: Elevators : Incomplete hook setup. Disabled elevator restoration.");
        }
        else
        {
            spdlog::info("MG2: Elevators : Initialized (floor lamps: {}, timings: {}).", bRestoreLamps, bRestoreTimings);
        }
    }
}
