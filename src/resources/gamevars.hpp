#pragma once

class GameVars final
{
private:
    static void OnLevelTransition();

    int* cutsceneFlag = nullptr;
    int* scriptedSequenceFlag = nullptr;
    double* actorWaitValue = nullptr;
    const char* currentStage = nullptr;

public:
    void Initialize();
    bool InCutscene() const; // If we're in a full demo cutscene.
    bool InScriptedSequence() const; // If the game is in a scripted sequence (cutscene or pad demo).
    double ActorWaitMultiplier() const;
    const char* GetCurrentStage() const;
};

inline GameVars g_GameVars;
