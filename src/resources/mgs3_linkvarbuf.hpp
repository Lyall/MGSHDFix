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

    enum SpecialItemFlags : short
    {
        SpecialItem_None = 0,
        SpecialItem_StealthCamouflage = 1 << 0,
        SpecialItem_Infinity = 1 << 1,
        SpecialItem_EZGun = 1 << 2
    };

    enum GM_CONFIG : int
    {
        GM_CONFIG_PLAYTIME_STOP = 0x4000,
        GM_CONFIG_GAME_OVER_IF_DISCOVERED = 0x80000
    };


    inline LinkVarValue<short, 6>           GM_GameLevel;
    inline LinkVarValue<int, 8>             GM_Configuration;

    inline LinkVarPointer<char, 36>         GM_AreaCode;

    inline LinkVarValue<short, 52>          GM_ContinueCount;
    inline LinkVarValue<short, 54>          GM_SaveCount;
    inline LinkVarValue<short, 56>          GM_AlertCount;
    inline LinkVarValue<short, 58>          GM_KillCount;
    inline LinkVarValue<short, 60>          GM_SpecialItemFlags;
    inline LinkVarValue<unsigned char, 62>  GM_KerotanFlag;
    inline LinkVarValue<short, 64>          GM_InjuryCount;
    inline LinkVarValue<short, 68>          GM_DamageCount;
    inline LinkVarValue<short, 70>          GM_MealCount;
    inline LinkVarValue<int, 72>            GM_AreaPlayTime;
    inline LinkVarValue<int, 76>            GM_PlayTime;

    inline LinkVarValue<short, 1448>        GM_LifeMedicineUseCount;

    inline LinkVarPointer<short, 1460>      GM_Weapons;
    inline LinkVarPointer<short, 1476>      GM_Items;
    inline LinkVarValue<short, 1492>        GM_CurrentWeapon;
    inline LinkVarValue<short, 1494>        GM_CurrentItem;

    inline LinkVarValue<short, 1662>        GM_EquippedCamouflage;
    inline LinkVarValue<short, 1668>        GM_CurrentHealth;
    inline LinkVarValue<short, 1670>        GM_MaxHealth;
    inline LinkVarPointer<Injury, 1672>     GM_Injuries;

    inline LinkVarValue<short, 2634>        GM_CurrentStamina;
    inline LinkVarValue<short, 2636>        GM_MaxStamina;

}

