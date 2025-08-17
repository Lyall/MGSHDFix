#include "common.hpp"
#include "gamevars.hpp"
#include "effect_speeds.hpp"
#include "logging.hpp"
#include "mgs2_sunglasses.hpp"
#include "stat_persistence.hpp"


void GameVars::Initialize()
{
    if (eGameType & MGS2)
    {
        aimingState = reinterpret_cast<uint64_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 2D ?? ?? ?? ?? F3 0F 10 1D", "MGS 2: GameVars: aimingState") + 2)); // +17DF86C 2.0.1
        cutsceneFlag = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 05 ?? ?? ?? ?? 45 33 F6 8B 0D", "MGS 2: GameVars: cutsceneFlag") + 2));
        scriptedSequenceFlag = reinterpret_cast<int*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "44 39 35 ?? ?? ?? ?? 89 15", "MGS 2: GameVars: scriptedSequenceFlag") + 3));
        actorWaitValue = reinterpret_cast<double*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "66 0F 2F 05 ?? ?? ?? ?? 73 ?? 33 C0", "MGS 2: GameVars: actorWaitValue") + 4));
        currentStage = reinterpret_cast<char const*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "4C 8D 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 4C 8D 05", "MGS 2: GameVars: currentStage") + 3));

        heldTriggers = reinterpret_cast<uint32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F2 0F 11 05 ?? ?? ?? ?? 0F 11 0D", "MGS 2: GameVars: heldTriggers") + 4));

        spdlog::info("GameVars: aimingState address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)aimingState - (uintptr_t)baseModule);
        spdlog::info("GameVars: cutsceneFlag address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)cutsceneFlag - (uintptr_t)baseModule);
        spdlog::info("GameVars: scriptedSequenceFlag address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)scriptedSequenceFlag - (uintptr_t)baseModule);
        spdlog::info("GameVars: actorWaitValue address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)actorWaitValue - (uintptr_t)baseModule);
        spdlog::info("GameVars: currentStage address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)currentStage - (uintptr_t)baseModule);
        spdlog::info("GameVars: heldTriggers address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)heldTriggers - (uintptr_t)baseModule);
        
        if (uint8_t* LevelTransitionResult = Memory::PatternScan(baseModule, "89 73 ?? 81 25", "GameVars: Level Transition"))
        {
            static SafetyHookMid levelTransitionMidHook {};
            levelTransitionMidHook = safetyhook::create_mid(LevelTransitionResult,
                [](SafetyHookContext& ctx)
                {
                    OnLevelTransition();
                });
            LOG_HOOK(levelTransitionMidHook, "GameVars: Level Transition")
        }
    }
    else if (eGameType & MGS3)
    {
        aimingState = reinterpret_cast<uint64_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "8B 35 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 49 89 8D", "MGS 3: GameVars: aimingState") + 2));
        heldTriggers = reinterpret_cast<uint32_t*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "48 8D 3D ?? ?? ?? ?? BA ?? ?? ?? ?? 41 B8", "MGS 3: GameVars: heldTriggers") + 3));

        spdlog::info("GameVars: aimingState address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)aimingState - (uintptr_t)baseModule);
        spdlog::info("GameVars: heldTriggers address is {:s}+{:X}", sExeName.c_str(), (uintptr_t)heldTriggers - (uintptr_t)baseModule);
        
        if (uint8_t* LevelTransitionResult = Memory::PatternScan(baseModule, "89 5F ?? E9 ?? ?? ?? ?? 39 1D", "GameVars: Level Transition"))
        {
            static SafetyHookMid levelTransitionMidHook {};
            levelTransitionMidHook = safetyhook::create_mid(LevelTransitionResult,
                [](SafetyHookContext& ctx)
                {
                    OnLevelTransition();
                });
            LOG_HOOK(levelTransitionMidHook, "GameVars: Level Transition")
        }
    }
}

bool GameVars::InCutscene() const //does not count pad demos or FMVs, only full cutscenes.
{
    return cutsceneFlag == nullptr ? false : (*cutsceneFlag == 1);
}

bool GameVars::InScriptedSequence() const //Scripted sequences, i.e., forced codec calls, cutscenes, pad demos (ingame tutorials & forced movements ala first meeting Stillman), and FMVs.
{
    return scriptedSequenceFlag == nullptr ? false : (*scriptedSequenceFlag == 1);
}

double GameVars::ActorWaitMultiplier() const
{
    return actorWaitValue == nullptr ? 1.0 : ((1.0/60) / *actorWaitValue);
}

const char* GameVars::GetCurrentStage() const
{
    return currentStage == nullptr ? "nullptr" : currentStage;
}


void GameVars::SetAimingState(const uint64_t state) const
{
    if (aimingState != nullptr)
    {
        *aimingState = state;
        return;
    }
    spdlog::error("GameVars: SetAimingState: aimingState is null!");
}

uint64_t GameVars::GetAimingState() const
{
    if (aimingState != nullptr)
    {
        return *aimingState;
    }
    spdlog::error("GameVars: GetAimingState: aimingState is null!");
    return 0;
}


bool GameVars::MGS2IsHoldingWeaponMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_WeaponMenu) != 0;
}
bool GameVars::MGS2IsHoldingEquipmentMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_EquipmentMenu) != 0;
}
bool GameVars::MGS2IsHoldingFirstPerson() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_FirstPerson) != 0;
}
bool GameVars::MGS2IsHoldingLockOn() const
{
    return (*heldTriggers & HoldingTriggers::MGS2_LockOn) != 0;
}

bool GameVars::MGS3IsHoldingEquipmentMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_EquipmentMenu) != 0;
}

bool GameVars::MGS3IsHoldingWeaponMenu() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_WeaponMenu) != 0;
}

bool GameVars::MGS3IsHoldingLockOn() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_LockOn) != 0;
}

bool GameVars::MGS3IsHoldingFirstPerson() const
{
    return (*heldTriggers & HoldingTriggers::MGS3_FirstPerson) != 0;
}


void GameVars::OnLevelTransition()
{
    g_EffectSpeedFix.Reset();
    g_StatPersistence.SaveStats();
    MGS2Sunglasses::CheckOnTransition();
}


namespace
{
#define X(name, id, mode, disp) { id, mode, disp },

    inline const Stage g_StagesMGS2[] = { MGS2_STAGE_LIST };
    inline const Stage g_StagesMGS3[] = { MGS3_STAGE_LIST };

#undef X

    inline const Stage* FindStageByName(const char* currentStage)
    {
        if (!currentStage)
            return nullptr;

        // eGameType is always valid (exactly one of {MGS2, MGS3})
        const Stage* begin = nullptr;
        const Stage* end = nullptr;

        if (eGameType & MGS2)
        {
            begin = std::begin(g_StagesMGS2);
            end = std::end(g_StagesMGS2);
        }
        else if (eGameType & MGS3)
        {
            begin = std::begin(g_StagesMGS3);
            end = std::end(g_StagesMGS3);
        }

        for (auto it = begin; it != end; ++it)
        {
            if (_stricmp(it->sStageId, currentStage) == 0)
                return &(*it);
        }

        return nullptr;
    }
}

std::string GameVars::GetRichPresenceString() const
{
    const char* currentStage = GetCurrentStage();
    const Stage* s = FindStageByName(currentStage);

    if (s == nullptr)
    {
        if (currentStage == nullptr)
        {
            return "Unknown Stage";
        }
        return std::string("Unknown Stage (") + currentStage + ")";
    }

    // VR formatting uses " | ", everything else uses ": "
    const bool isVR = (std::strncmp(s->sGameMode, "VR:", 3) == 0);
    std::string result;

    if (isVR)
    {
        result = std::string(s->sGameMode) + " | " + s->sRichPresenceName;
    }
    else
    {
        result = std::string(s->sGameMode) + ": " + s->sRichPresenceName;
    }

    if (InCutscene())
    {
        result += " - In Cutscene";
    }

    return result;
}

std::string GameVars::GetGameMode() const
{
    const char* currentStage = GetCurrentStage();
    const Stage* s = FindStageByName(currentStage);

    if (s != nullptr)
    {
        return s->sGameMode;
    }

    if (currentStage == nullptr)
    {
        return "Unknown";
    }

    return std::string("Unknown (") + currentStage + ")";
}

bool GameVars::IsStage(const char* stageConst) const
{
    const char* current = GetCurrentStage();
    if (current == nullptr)
    {
        spdlog::error("IsStage() called with null current stage pointer");
        return false;
    }

    return _stricmp(current, stageConst) == 0;
}

bool GameVars::IsAnyStage(std::initializer_list<const char*> stages) const
{
    const char* current = GetCurrentStage();
    if (current == nullptr)
    {
        spdlog::error("IsAnyStage() called with null current stage pointer");
        return false;
    }

    for (const char* s : stages)
    {
        if (_stricmp(current, s) == 0)
        {
            return true;
        }
    }
    return false;
}
