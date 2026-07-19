// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once



namespace MGS3_LinkVarBuf
{
    inline uintptr_t* linkvarbuf = nullptr;

    template <typename T, uintptr_t Offset>
    struct LinkVarValue
    {
        operator T& () const
        {
            return *reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        T& get() const
        {
            return *reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        LinkVarValue& operator=(const T value)
        {
            get() = value;
            return *this;
        }
    };

    template <typename T, uintptr_t Offset>
    struct LinkVarPointer
    {
        operator T* () const
        {
            return reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        T* get() const
        {
            return reinterpret_cast<T*>(*linkvarbuf + Offset);
        }

        T& operator[](const size_t index) const
        {
            return get()[index];
        }
    };

    struct Injury
    {
        short x;
        short y;
        short z;
        short unknown_06;
        short injuryType;
        short treatmentsApplied;
        short injuryHealth;
    };

    static_assert(sizeof(Injury) == 14);

    enum class GameLevel : short
    {
        VeryEasy = 10,
        Easy = 20,
        Normal = 30,
        Hard = 40,
        Extreme = 50,
        EuropeanExtreme = 60
    };

    enum SpecialItemFlags : unsigned char
    {
        SpecialItem_None = 0,
        SpecialItem_StealthCamouflage = 1 << 0,
        SpecialItem_Infinity = 1 << 1,
        SpecialItem_EZGun = 1 << 2
    };

    enum GM_CONFIG : unsigned int
    {
        GM_CONFIG_VIBRATION_OFF = 0x1,

        GM_CONFIG_CUTSCENES_LETTERBOXED = 0x400,
        GM_CONFIG_PLAYTIME_STOP = 0x4000,
        GM_CONFIG_GAME_OVER_IF_DISCOVERED = 0x80000
    };


    inline LinkVarValue<short, 2>           GM_Result;
    inline LinkVarValue<short, 4>           GM_Language;
    inline LinkVarValue<short, 6>           GM_GameLevel;
    inline LinkVarValue<unsigned int, 8>    GM_Configuration;
    inline LinkVarValue<int, 16>            GM_SaveArea;
    inline LinkVarPointer<char, 20>         GM_SaveResidentDir;
    inline LinkVarPointer<char, 36>         GM_SaveAreaDir;



    inline LinkVarValue<short, 52>          GM_ContinueCount;
    inline LinkVarValue<short, 54>          GM_SaveCount;
    inline LinkVarValue<short, 56>          GM_AlertCount;
    inline LinkVarValue<short, 58>          GM_KillCount;

    inline LinkVarValue<unsigned char, 60>  GM_TsuchinokoFlag;
    inline LinkVarValue<unsigned char, 61>  GM_SpecialItemFlags;
    inline LinkVarValue<unsigned char, 62>  GM_KerotanFlag;
    inline LinkVarValue<unsigned char, 63>  GM_UniqueFoodCollectedCount;
    inline LinkVarValue<short, 64>          GM_InjuryCount;
    inline LinkVarValue<short, 66>          GM_Unknown_66;
    inline LinkVarValue<short, 68>          GM_LifebarDamageCount;
    inline LinkVarValue<short, 70>          GM_MealCount;
    inline LinkVarValue<int, 72>            GM_StagePlayTime;
    inline LinkVarValue<int, 76>            GM_PlayTime;
    inline LinkVarValue<short, 1448>        GM_LifeMedicineUseCount;

    inline LinkVarValue<int, 1452>          GM_WeaponCount;
    inline LinkVarValue<int, 1456>          GM_ItemCount;
    inline LinkVarPointer<short, 1460>      GM_Weapons;
    inline LinkVarPointer<short, 1476>      GM_Items;

    inline LinkVarValue<short, 1492>        GM_CurrentWeapon;
    inline LinkVarValue<short, 1494>        GM_CurrentItem;
    inline LinkVarValue<short, 1496>        GM_PreviousWeapon;
    inline LinkVarValue<short, 1498>        GM_PreviousItem;
    inline LinkVarValue<short, 1500>        GM_CurrentWeaponAmmo;

    //infinite suppressor -> 0F 84 ?? ?? ?? ?? 83 FB ?? 75 ?? 45 85 FF -> jz TRUE

    //  12 = Sneaking Suit
    //  20 = Fire
    //  21 = Spirit
    //  25 = desert tiger
    //  28 = Desert Auscam
    //  33 = Animals 
    //  34 = mummy
    inline LinkVarValue<unsigned char, 1662> GM_EquippedCamouflage; 
    inline LinkVarValue<unsigned char, 1663> GM_EquippedFacepaint;  // 13 = Infinity

    enum CurrentWeaponFlags : unsigned int
    {
        CurrentWeapon_CqcCompatible = 0x400
    };
    inline LinkVarValue<unsigned int, 1664> GM_CurrentWeaponFlags;
    inline LinkVarValue<short, 1668>        GM_CurrentHealth;
    inline LinkVarValue<short, 1670>        GM_MaxHealth;
    inline LinkVarPointer<Injury, 1672>     GM_Injuries;

    inline LinkVarValue<short, 2634>        GM_CurrentStamina;
    inline LinkVarValue<short, 2636>        GM_MaxStamina;
    inline LinkVarValue<short, 2638>        GM_CurrentBattery;
    inline LinkVarValue<short, 2640>        GM_MaxBattery;
    inline LinkVarValue<short, 2642>        GM_CurrentBugproof;
    inline LinkVarValue<short, 2644>        GM_MaxBugproof;
    inline LinkVarValue<int, 2646>          GM_DiazepamCount;

    enum SpecialAnimalCaptureFlags : unsigned int
    {
        SpecialAnimal_121 = 0x02,
        SpecialAnimal_107 = 0x04,
        SpecialAnimal_106 = 0x08,
        SpecialAnimal_108 = 0x10,
        SpecialAnimal_TsuchinokoCaught = 0x20,
        SpecialAnimal_TsuchinokoSpawned = 0x40
    };
    inline LinkVarValue<unsigned int, 6300> GM_SpecialAnimalCaptureFlags;

    inline LinkVarValue<unsigned int, 6304> GM_FoodCaptureFlagsLow;
    inline LinkVarValue<unsigned int, 6308> GM_FoodCaptureFlagsHigh;
    //// Food index:
    //  0-10 = Snakes             // 11 total
    //  12-14 = Frogs              // 3 total
    //  23-27 = Birds              // 5 total
    //  28-30 = Fish               // 3 total
    //  32-38 = Mushrooms          // 7 total
    //  39-41 = Fruits             // 3 total
    //  44-47 = Medicinal plants   // 4 total

}

