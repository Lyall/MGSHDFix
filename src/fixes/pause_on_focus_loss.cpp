


/*
#pragma endregion misc_alt_tab_pause_fix
/*
if((eGameType == MgsGame::MGS2) || (eGameType == MgsGame::MGS3)){
    uint8_t* WeaponPutawayScanResult = Memory::PatternScan(baseModule, ((eGameType == MgsGame::MGS2) ? "89 1D ?? ?? ?? ?? 8B C3 48 83 C4" : nullptr));
    if (WeaponPutawayScanResult)
    {
       // uint8_t* Weapon_Aiming_Origin_Offset = Memory::PatternScan(baseModule, "83 3D ?? ?? ?? ?? 00 ?? ?? F2 0F 10 0D");
        //int* is_aiming = reinterpret_cast<int*>(Memory::GetAbsolute((uintptr_t)Weapon_Aiming_Origin_Offset + 13));

        static SafetyHookMid WeaponPutawayFix4MidHook{};
        WeaponPutawayFix4MidHook = safetyhook::create_mid(WeaponPutawayScanResult,
            [](SafetyHookContext& ctx)
            {
                spdlog::info("MGS2 | MGS3: Unequip pointer 4");
                MGS2_MGS3_Aiming_Fix();
            });
    }
 } */

 //Replaces the "DAMAGED DATA" save file error message with an actually useful explanation of the issue with the save file.
/* if (eGameType == MgsGame::MGS2) {
     static SafetyHookMid SaveMessage1_Hook{};
     SaveMessage1_Hook = safetyhook::create_mid(Memory::PatternScan(baseModule, "4C 0F 44 C8 48 8B 43"), //year
         [](SafetyHookContext& ctx)
         {
             spdlog::info("hook 1.");
             ctx.r9 = reinterpret_cast<uintptr_t>(&"ERROR: DUPLICATE FILE IN SAVE FOLDER");
             ctx.rax = reinterpret_cast<uintptr_t>(&"ERROR: DUPLICATE FILE IN SAVE FOLDER");
         });
     if (SaveMessage1_Hook)
         spdlog::info("hook 1 pos.");
     static SafetyHookMid SaveMessage2_Hook{};
     SaveMessage2_Hook = safetyhook::create_mid(Memory::PatternScan(baseModule, "48 0F 44 F8 E9 ?? ?? ?? ?? 83 BB ?? ?? ?? ?? ?? 8B CF"), //year
         [](SafetyHookContext& ctx)
         {
             spdlog::info("hook 2.");
             ctx.rdi = reinterpret_cast<uintptr_t>(&"ERROR: DUPLICATE FILE IN SAVE FOLDER");
             ctx.rax = reinterpret_cast<uintptr_t>(&"ERROR: DUPLICATE FILE IN SAVE FOLDER");
         });
     if (SaveMessage2_Hook)
         spdlog::info("hook 2 pos.");

     static SafetyHookMid SaveMessage4_Hook{};
     SaveMessage4_Hook = safetyhook::create_mid(Memory::PatternScan(baseModule, "48 89 5C 24 ?? FF 15 ?? ?? ?? ?? 8B D8"), //year
         [](SafetyHookContext& ctx)
         {
             spdlog::info("hook 1. {}", reinterpret_cast<char*>(ctx.rcx));
             ctx.rcx = reinterpret_cast<uintptr_t>(&"BADASS BITCH GUN");
         });
     if (SaveMessage4_Hook)
         spdlog::info("hook 1 pos.");
 }*/
/*
}

bool fixPauseOnFocusLoss()
{
    return bDisablePauseOnFocusLoss || InCutscene() || InDemoCutscene();
}

void disablePauseOnFocusLoss()
{
    if (!(eGameType == MgsGame::MGS2 || eGameType == MgsGame::MGS3 || eGameType == MgsGame::MG))
    {
        return;
    }

    uint8_t* BP_COsContext_ShouldPauseApplicationScanResult = Memory::PatternScan(baseModule, "85 C0 74 ?? 48 83 C6");
    if (BP_COsContext_ShouldPauseApplicationScanResult)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: Pause on Alt-Tab: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)BP_COsContext_ShouldPauseApplicationScanResult - (uintptr_t)baseModule);
        static SafetyHookMid BP_COsContext_ShouldPauseApplicationMidHook {};
        BP_COsContext_ShouldPauseApplicationMidHook = safetyhook::create_mid(BP_COsContext_ShouldPauseApplicationScanResult,
            [](SafetyHookContext& ctx)
            {
                if (fixPauseOnFocusLoss)
                {
                    ctx.rax = 0;
                }
            });
    }

    uint8_t* BP_COsContext_ShouldPauseApplication_2_Result = Memory::PatternScan(baseModule, "85 C0 74 ?? F7 05 ?? ?? ?? ?? ?? ?? ?? ?? 75");
    if (BP_COsContext_ShouldPauseApplication_2_Result)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: Pause on Alt-Tab 2: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)BP_COsContext_ShouldPauseApplication_2_Result - (uintptr_t)baseModule);
        static SafetyHookMid BP_COsContext_ShouldPauseApplication2MidHook {};
        BP_COsContext_ShouldPauseApplication2MidHook = safetyhook::create_mid(BP_COsContext_ShouldPauseApplication_2_Result,
            [](SafetyHookContext& ctx)
            {
                if (fixPauseOnFocusLoss)
                {
                    ctx.rax = 0;
                }
            });

    }

    uint8_t* BP_COsContext_ShouldPauseApplication_3_Result = Memory::PatternScan(baseModule, "44 8B 2D ?? ?? ?? ?? 46 89 B4 A6");
    if (BP_COsContext_ShouldPauseApplication_3_Result)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: Pause on Alt-Tab 3: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)BP_COsContext_ShouldPauseApplication_3_Result - (uintptr_t)baseModule);
        static SafetyHookMid BP_COsContext_ShouldPauseApplication3MidHook {};
        BP_COsContext_ShouldPauseApplication3MidHook = safetyhook::create_mid(BP_COsContext_ShouldPauseApplication_3_Result,
            [](SafetyHookContext& ctx)
            {
                if (fixPauseOnFocusLoss)
                {
                    ctx.rax = 0;
                }
            });

    }





    uint8_t* NHT_GetIsMinimizedResult = Memory::PatternScan(baseModule, "C3 83 B8 ?? ?? ?? ?? ?? 0F 94 C0");
    if (NHT_GetIsMinimizedResult)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: Pause on Alt-Tab 4: Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)NHT_GetIsMinimizedResult - (uintptr_t)baseModule);
        static SafetyHookMid NHT_GetIsMinimizedMidHook {};
        NHT_GetIsMinimizedMidHook = safetyhook::create_mid(NHT_GetIsMinimizedResult,
            [](SafetyHookContext& ctx)
            {
                if (fixPauseOnFocusLoss)
                {
                    ctx.rax = 0;
                }
            });
    }
    /*
    HMODULE engineModule = GetModuleHandleA("Engine.dll");
    if (!engineModule)
    {
        spdlog::error("MG/MG2 | MGS 2 | MGS3: Launcher Config: Failed to get Engine.dll module handle");
        return;
    }
    MGS3_COsContext_InitializeSKUandLang = decltype(MGS3_COsContext_InitializeSKUandLang)(GetProcAddress(engineModule, "NHT_COsContext_SetShouldPauseApplication"));
    if (MGS3_COsContext_InitializeSKUandLang)
    {
        if (Memory::HookIAT(baseModule, "Engine.dll", MGS3_COsContext_InitializeSKUandLang, MGS3_COsContext_InitializeSKUandLang_Hook))
        {
            spdlog::info("MG/MG2 | MGS 3: Launcher Config: Hooked COsContext::InitializeSKUandLang, overriding with Region/Language settings from INI");
        }
        else
        {
            spdlog::error("MG/MG2 | MGS 3: Launcher Config: Failed to apply COsContext::InitializeSKUandLang IAT hook");
        }
    }

    MGS3_COsContext_InitializeSKUandLang = decltype(MGS3_COsContext_InitializeSKUandLang)(GetProcAddress(engineModule, "BP_COsContext_ShouldPauseApplication"));
    if (MGS3_COsContext_InitializeSKUandLang)
    {
        if (Memory::HookIAT(baseModule, "Engine.dll", MGS3_COsContext_InitializeSKUandLang, MGS3_COsContext_InitializeSKUandLang_Hook))
        {
            spdlog::info("MG/MG2 | MGS 3: Launcher Config: Hooked COsContext::InitializeSKUandLang, overriding with Region/Language settings from INI");
        }
        else
        {
            spdlog::error("MG/MG2 | MGS 3: Launcher Config: Failed to apply COsContext::InitializeSKUandLang IAT hook");
        }
    }*/


/*
}

/*
using NHT_COsContext_SetControllerID_Fn = void (*)(int controllerType);
NHT_COsContext_SetControllerID_Fn NHT_COsContext_SetControllerID = nullptr;
void NHT_COsContext_SetControllerID_Hook(int controllerType)
{
    spdlog::info("NHT_COsContext_SetControllerID_Hook: controltype {} -> {}", controllerType, iLauncherConfigCtrlType);
    NHT_COsContext_SetControllerID(iLauncherConfigCtrlType);
}

using NHT_GetIsMinimized_Fn = void(*)(bool result);
NHT_GetIsMinimized_Fn NHT_GetIsMinimized = nullptr;
void __fastcall NHT_GetIsMinimized_Hook(bool)
{
    spdlog::info("MGS3_COsContext_InitializeSKUandLang: lang {} -> {}, sku {} -> {}", sku, iLauncherConfigRegion, lang, iLauncherConfigLanguage);
    return fixPauseOnFocusLoss() ? 0 : NHT_GetIsMinimized();
}
*/
/*
void ReplaceSaveErrorMessages()
{

}

void MGS2_MGS3_Aiming_Fix()
{
    spdlog::info("MGS2 | MGS3: triggered");
    return;
}



#pragma region misc_alt_tab_pause_fix
if ((eGameType == MgsGame::MGS2) || (eGameType == MgsGame::MGS3) || (eGameType == MgsGame::MG))
{
    /*
    uint8_t* GetAsyncKeyStateTestResult = Memory::PatternScan(baseModule, "66 85 C0 0F 85 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 66 85 C0 0F 85 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 66 85 C0 0F 85 ?? ?? ?? ?? B9");
    if (GetAsyncKeyStateTestResult)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: GetAsyncKeyStateTestResult : Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GetAsyncKeyStateTestResult - (uintptr_t)baseModule);
        static SafetyHookMid BP_COsContext_ShouldPauseApplicationMidHook{};
        BP_COsContext_ShouldPauseApplicationMidHook = safetyhook::create_mid(GetAsyncKeyStateTestResult,
            [](SafetyHookContext& ctx)
            {
                ctx.rax = 0;
            });
    }
    GetAsyncKeyStateTestResult = Memory::PatternScan(baseModule, "66 85 C0 0F 85 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 66 85 C0 0F 85 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 66 85 C0 0F 85 ?? ?? ?? ?? 48 89 5C 24");
    if (GetAsyncKeyStateTestResult)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: GetAsyncKeyStateTestResult : Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GetAsyncKeyStateTestResult - (uintptr_t)baseModule);
        static SafetyHookMid GetAsyncKeyStateTest1Hook{};
        GetAsyncKeyStateTest1Hook = safetyhook::create_mid(GetAsyncKeyStateTestResult,
            [](SafetyHookContext& ctx)
            {
                ctx.rax = 0;
            });
    }
    GetAsyncKeyStateTestResult = Memory::PatternScan(baseModule, "66 85 C0 0F 85 ?? ?? ?? ?? B9 ?? ?? ?? ?? FF 15 ?? ?? ?? ?? 66 85 C0 0F 85 ?? ?? ?? ?? 48 89 5C 24");
    if (GetAsyncKeyStateTestResult)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: GetAsyncKeyStateTestResult : Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GetAsyncKeyStateTestResult - (uintptr_t)baseModule);
        static SafetyHookMid GetAsyncKeyStateTest2Hook{};
        GetAsyncKeyStateTest2Hook = safetyhook::create_mid(GetAsyncKeyStateTestResult,
            [](SafetyHookContext& ctx)
            {
                ctx.rax = 0;
            });
    }
    GetAsyncKeyStateTestResult = Memory::PatternScan(baseModule, "66 85 C0 0F 85 ?? ?? ?? ?? 48 89 5C 24");
    if (GetAsyncKeyStateTestResult)
    {
        spdlog::info("MG/MG2 | MGS2 | MGS3: GetAsyncKeyStateTestResult : Address is {:s}+{:x}", sExeName.c_str(), (uintptr_t)GetAsyncKeyStateTestResult - (uintptr_t)baseModule);
        static SafetyHookMid GetAsyncKeyStateTest3Hook{};
        GetAsyncKeyStateTest3Hook = safetyhook::create_mid(GetAsyncKeyStateTestResult,
            [](SafetyHookContext& ctx)
            {
                ctx.rax = 0;
            });
    }
    */

    /*
    //disablePauseOnFocusLoss();

    bPauseOnFocusLoss
    */