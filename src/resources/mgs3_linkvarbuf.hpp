// ReSharper disable CppClangTidyClangDiagnosticUniqueObjectDuplication
#pragma once



namespace MGS3_LinkVarBuf
{
    enum MGS3WeaponIndex : uint8_t
    {
        MGS3_WEAPON_INDEX_None = 0x00,
        MGS3_WEAPON_INDEX_SurvivalKnife = 0x01,
        MGS3_WEAPON_INDEX_Fork = 0x02,
        MGS3_WEAPON_INDEX_CigSpray = 0x03,
        MGS3_WEAPON_INDEX_Handkerchief = 0x04,
        MGS3_WEAPON_INDEX_MK22 = 0x05,
        MGS3_WEAPON_INDEX_M1911A1 = 0x06,
        MGS3_WEAPON_INDEX_EzGun = 0x07,
        MGS3_WEAPON_INDEX_SAA = 0x08,
        MGS3_WEAPON_INDEX_Patriot = 0x09,
        MGS3_WEAPON_INDEX_Scorpion = 0x0A,
        MGS3_WEAPON_INDEX_XM16E1 = 0x0B,
        MGS3_WEAPON_INDEX_AK47 = 0x0C,
        MGS3_WEAPON_INDEX_M63 = 0x0D,
        MGS3_WEAPON_INDEX_M37 = 0x0E,
        MGS3_WEAPON_INDEX_SVD = 0x0F,
        MGS3_WEAPON_INDEX_MosinNagant = 0x10,
        MGS3_WEAPON_INDEX_RPG7 = 0x11,
        MGS3_WEAPON_INDEX_Torch = 0x12,
        MGS3_WEAPON_INDEX_Grenade = 0x13,
        MGS3_WEAPON_INDEX_WPGrenade = 0x14,
        MGS3_WEAPON_INDEX_StunGrenade = 0x15,
        MGS3_WEAPON_INDEX_ChaffGrenade = 0x16,
        MGS3_WEAPON_INDEX_SmokeGrenade = 0x17,
        MGS3_WEAPON_INDEX_EmptyMagazine = 0x18,
        MGS3_WEAPON_INDEX_TNT = 0x19,
        MGS3_WEAPON_INDEX_C3 = 0x1A,
        MGS3_WEAPON_INDEX_Claymore = 0x1B,
        MGS3_WEAPON_INDEX_Book = 0x1C,
        MGS3_WEAPON_INDEX_Mousetrap = 0x1D,
        MGS3_WEAPON_INDEX_DirectionalMic = 0x1E,

        MGS3_WEAPON_INDEX_Empty_1F = 0x1F,
        MGS3_WEAPON_INDEX_Empty_20 = 0x20,
        MGS3_WEAPON_INDEX_Empty_21 = 0x21,
        MGS3_WEAPON_INDEX_Empty_22 = 0x22,
        MGS3_WEAPON_INDEX_Empty_23 = 0x23,
        MGS3_WEAPON_INDEX_Empty_24 = 0x24,
        MGS3_WEAPON_INDEX_Empty_25 = 0x25,
        MGS3_WEAPON_INDEX_Empty_26 = 0x26,
        MGS3_WEAPON_INDEX_Empty_27 = 0x27,
        MGS3_WEAPON_INDEX_Empty_28 = 0x28,
        MGS3_WEAPON_INDEX_Empty_29 = 0x29,
        MGS3_WEAPON_INDEX_Empty_2A = 0x2A,
        MGS3_WEAPON_INDEX_Empty_2B = 0x2B,
        MGS3_WEAPON_INDEX_Empty_2C = 0x2C,
        MGS3_WEAPON_INDEX_Empty_2D = 0x2D,
        MGS3_WEAPON_INDEX_Empty_2E = 0x2E,
        MGS3_WEAPON_INDEX_Empty_2F = 0x2F,
        MGS3_WEAPON_INDEX_Empty_30 = 0x30,
        MGS3_WEAPON_INDEX_Empty_31 = 0x31,

        MGS3_WEAPON_INDEX_R_KingCobra = 0x32,
        MGS3_WEAPON_INDEX_R_TaiwaneseCobra = 0x33,
        MGS3_WEAPON_INDEX_R_ThaiCobra = 0x34,
        MGS3_WEAPON_INDEX_R_CoralSnake = 0x35,
        MGS3_WEAPON_INDEX_R_MilkSnake = 0x36,
        MGS3_WEAPON_INDEX_R_GreenTreePython = 0x37,
        MGS3_WEAPON_INDEX_R_GiantAnaconda = 0x38,
        MGS3_WEAPON_INDEX_R_ReticulatedPython = 0x39,
        MGS3_WEAPON_INDEX_R_SnakeLiquid = 0x3A,
        MGS3_WEAPON_INDEX_R_SnakeSolid = 0x3B,
        MGS3_WEAPON_INDEX_R_SnakeSolidus = 0x3C,
        MGS3_WEAPON_INDEX_R_IndianGavial = 0x3D,
        MGS3_WEAPON_INDEX_R_OttonFrog = 0x3E,
        MGS3_WEAPON_INDEX_R_TreeFrog = 0x3F,
        MGS3_WEAPON_INDEX_R_PoisonDartFrog = 0x40,
        MGS3_WEAPON_INDEX_R_Rat = 0x41,
        MGS3_WEAPON_INDEX_R_EuropeanRabbit = 0x42,
        MGS3_WEAPON_INDEX_R_FlyingSquirrel = 0x43,
        MGS3_WEAPON_INDEX_R_Markhor = 0x44,
        MGS3_WEAPON_INDEX_R_VampireBat = 0x45,
        MGS3_WEAPON_INDEX_R_HornetsNest = 0x46,
        MGS3_WEAPON_INDEX_R_EmperorScorpion = 0x47,
        MGS3_WEAPON_INDEX_R_CobaltBlueTarantula = 0x48,
        MGS3_WEAPON_INDEX_R_Parrot = 0x49,
        MGS3_WEAPON_INDEX_R_WhiteRumpedVulture = 0x4A,
        MGS3_WEAPON_INDEX_R_RedAvadavat = 0x4B,
        MGS3_WEAPON_INDEX_R_Magpie = 0x4C,
        MGS3_WEAPON_INDEX_R_SundaWhistlingThrush = 0x4D,
        MGS3_WEAPON_INDEX_R_BigeyeTrevally = 0x4E,
        MGS3_WEAPON_INDEX_R_MaroonShark = 0x4F,
        MGS3_WEAPON_INDEX_R_Arowana = 0x50,
        MGS3_WEAPON_INDEX_R_KenyanMangroveCrab = 0x51,
        MGS3_WEAPON_INDEX_R_RussianOysterMushroom = 0x52,
        MGS3_WEAPON_INDEX_R_UralLuminescentMushroom = 0x53,
        MGS3_WEAPON_INDEX_R_SiberianInkCap = 0x54,
        MGS3_WEAPON_INDEX_R_FlyAgaric = 0x55,
        MGS3_WEAPON_INDEX_R_RussianGlowcap = 0x56,
        MGS3_WEAPON_INDEX_R_Spatsa = 0x57,
        MGS3_WEAPON_INDEX_R_BaikalScalyTooth = 0x58,
        MGS3_WEAPON_INDEX_R_YablokoMoloko = 0x59,
        MGS3_WEAPON_INDEX_R_RussianFalseMango = 0x5A,
        MGS3_WEAPON_INDEX_R_Golova = 0x5B,
        MGS3_WEAPON_INDEX_R_VineMelon = 0x5C,
        MGS3_WEAPON_INDEX_R_InstantNoodles = 0x5D,
        MGS3_WEAPON_INDEX_R_RussianRation = 0x5E,
        MGS3_WEAPON_INDEX_R_CalorieMate = 0x5F,
        MGS3_WEAPON_INDEX_R_HiveOfPainHornets = 0x60,
        MGS3_WEAPON_INDEX_R_Tsuchinoko = 0x61,

        MGS3_WEAPON_INDEX_KingCobra = 0x62,
        MGS3_WEAPON_INDEX_TaiwaneseCobra = 0x63,
        MGS3_WEAPON_INDEX_ThaiCobra = 0x64,
        MGS3_WEAPON_INDEX_CoralSnake = 0x65,
        MGS3_WEAPON_INDEX_MilkSnake = 0x66,
        MGS3_WEAPON_INDEX_GreenTreePython = 0x67,
        MGS3_WEAPON_INDEX_GiantAnaconda = 0x68,
        MGS3_WEAPON_INDEX_ReticulatedPython = 0x69,
        MGS3_WEAPON_INDEX_SnakeLiquid = 0x6A,
        MGS3_WEAPON_INDEX_SnakeSolid = 0x6B,
        MGS3_WEAPON_INDEX_SnakeSolidus = 0x6C,
        MGS3_WEAPON_INDEX_IndianGavial = 0x6D,
        MGS3_WEAPON_INDEX_OttonFrog = 0x6E,
        MGS3_WEAPON_INDEX_TreeFrog = 0x6F,
        MGS3_WEAPON_INDEX_PoisonDartFrog = 0x70,
        MGS3_WEAPON_INDEX_Rat = 0x71,
        MGS3_WEAPON_INDEX_EuropeanRabbit = 0x72,
        MGS3_WEAPON_INDEX_FlyingSquirrel = 0x73,
        MGS3_WEAPON_INDEX_Markhor = 0x74,
        MGS3_WEAPON_INDEX_VampireBat = 0x75,
        MGS3_WEAPON_INDEX_HornetsNest = 0x76,
        MGS3_WEAPON_INDEX_EmperorScorpion = 0x77,
        MGS3_WEAPON_INDEX_CobaltBlueTarantula = 0x78,
        MGS3_WEAPON_INDEX_Parrot = 0x79,
        MGS3_WEAPON_INDEX_WhiteRumpedVulture = 0x7A,
        MGS3_WEAPON_INDEX_RedAvadavat = 0x7B,
        MGS3_WEAPON_INDEX_Magpie = 0x7C,
        MGS3_WEAPON_INDEX_SundaWhistlingThrush = 0x7D,
        MGS3_WEAPON_INDEX_BigeyeTrevally = 0x7E,
        MGS3_WEAPON_INDEX_MaroonShark = 0x7F,
        MGS3_WEAPON_INDEX_Arowana = 0x80,
        MGS3_WEAPON_INDEX_KenyanMangroveCrab = 0x81,
        MGS3_WEAPON_INDEX_Tsuchinoko = 0x82,

        MGS3_WEAPON_INDEX_Mauser = 0x83,
        MGS3_WEAPON_INDEX_Makarov = 0x84,
        MGS3_WEAPON_INDEX_AMD65 = 0x85,
        MGS3_WEAPON_INDEX_Flamethrower_86 = 0x86,
        MGS3_WEAPON_INDEX_Flamethrower_87 = 0x87,
        MGS3_WEAPON_INDEX_TommyGun = 0x88,
        MGS3_WEAPON_INDEX_DShK = 0x89,
        MGS3_WEAPON_INDEX_Body = 0x8A,
        MGS3_WEAPON_INDEX_AKKnife = 0x8B,
        MGS3_WEAPON_INDEX_FlyingPlatform = 0x8C,
        MGS3_WEAPON_INDEX_Core = 0x8D,
        MGS3_WEAPON_INDEX_LittleJoe = 0x8E,
        MGS3_WEAPON_INDEX_WilliamTell = 0x8F,
        MGS3_WEAPON_INDEX_ZU23 = 0x90,
        MGS3_WEAPON_INDEX_DAttack = 0x91,
        MGS3_WEAPON_INDEX_Plasma = 0x92,
        MGS3_WEAPON_INDEX_Drumcan = 0x93,
        MGS3_WEAPON_INDEX_GroundFire = 0x94,
        MGS3_WEAPON_INDEX_Kick = 0x95,
        MGS3_WEAPON_INDEX_LeftPunch = 0x96,
        MGS3_WEAPON_INDEX_RightPunch = 0x97,
        MGS3_WEAPON_INDEX_Pendulum = 0x98,
        MGS3_WEAPON_INDEX_BowTrap = 0x99,
        MGS3_WEAPON_INDEX_BowTrapPoison = 0x9A,
        MGS3_WEAPON_INDEX_KnifeAttack = 0x9B,
        MGS3_WEAPON_INDEX_Rolling = 0x9C,
        MGS3_WEAPON_INDEX_DoorAttack = 0x9D,
        MGS3_WEAPON_INDEX_AnimalWeak = 0x9E,
        MGS3_WEAPON_INDEX_AnimalStrong = 0x9F,
        MGS3_WEAPON_INDEX_TNTTrigger = 0xA0,
        MGS3_WEAPON_INDEX_CriticalKnife = 0xA1,
        MGS3_WEAPON_INDEX_ExplosionSmall = 0xA2,
        MGS3_WEAPON_INDEX_ExplosionMiddle = 0xA3,
        MGS3_WEAPON_INDEX_ExplosionLarge = 0xA4,
        MGS3_WEAPON_INDEX_Shield = 0xA5,
        MGS3_WEAPON_INDEX_AT2 = 0xA6,
        MGS3_WEAPON_INDEX_UB32 = 0xA7,
        MGS3_WEAPON_INDEX_NoseGun = 0xA8,
        MGS3_WEAPON_INDEX_BridgeShake = 0xA9,
        MGS3_WEAPON_INDEX_Scope = 0xAA,
        MGS3_WEAPON_INDEX_Wireless1 = 0xAB,
        MGS3_WEAPON_INDEX_Wireless2 = 0xAC,
        MGS3_WEAPON_INDEX_LSight = 0xAD,
        MGS3_WEAPON_INDEX_BPunch = 0xAE,
        MGS3_WEAPON_INDEX_Plasma_AF = 0xAF,
        MGS3_WEAPON_INDEX_RollingDrum = 0xB0,
        MGS3_WEAPON_INDEX_SnakeBite = 0xB1,
        MGS3_WEAPON_INDEX_VehicleDamage = 0xB2,
        MGS3_WEAPON_INDEX_VehicleShake = 0xB3,
        MGS3_WEAPON_INDEX_FPLightBreak = 0xB4,
        MGS3_WEAPON_INDEX_Bee = 0xB5,
        MGS3_WEAPON_INDEX_VehicleNoDamage = 0xB6,
        MGS3_WEAPON_INDEX_Croco = 0xB7,
        MGS3_WEAPON_INDEX_Grass = 0xB8,
        MGS3_WEAPON_INDEX_DamDefense = 0xB9,
        MGS3_WEAPON_INDEX_Stomp = 0xBA,
        MGS3_WEAPON_INDEX_ShagoBody = 0xBB,
        MGS3_WEAPON_INDEX_SideCar = 0xBC,
        MGS3_WEAPON_INDEX_PainBeeWeak = 0xBD,
        MGS3_WEAPON_INDEX_PainBee = 0xBE,
        MGS3_WEAPON_INDEX_BulletBee = 0xBF,
        MGS3_WEAPON_INDEX_BodyPlasma = 0xC0,
        MGS3_WEAPON_INDEX_DarkBee = 0xC1,
        MGS3_WEAPON_INDEX_Honey = 0xC2,
        MGS3_WEAPON_INDEX_ShagoVul = 0xC3,
        MGS3_WEAPON_INDEX_Shago100Pod = 0xC4,
        MGS3_WEAPON_INDEX_KickNoDamage = 0xC5,
        MGS3_WEAPON_INDEX_HagetakaEat = 0xC6,
        MGS3_WEAPON_INDEX_PunchBody = 0xC7,
        MGS3_WEAPON_INDEX_PunchUpper = 0xC8,
        MGS3_WEAPON_INDEX_Mortar = 0xC9,
        MGS3_WEAPON_INDEX_PipeGas = 0xCA,
        MGS3_WEAPON_INDEX_PunchBodyL = 0xCB,
        MGS3_WEAPON_INDEX_BulletBodyL = 0xCC,
        MGS3_WEAPON_INDEX_PunchJabL = 0xCD,
        MGS3_WEAPON_INDEX_WGrenade = 0xCE,
    };

    enum MGS3ItemIndex : uint8_t
    {
        MGS3_ITEM_INDEX_None = 0x0,
        MGS3_ITEM_INDEX_LifeMedicine = 0x1,
        MGS3_ITEM_INDEX_Pentazemin = 0x2,
        MGS3_ITEM_INDEX_FakeDeathPill = 0x3,
        MGS3_ITEM_INDEX_RevivalPill = 0x4,
        MGS3_ITEM_INDEX_Cigar = 0x5,
        MGS3_ITEM_INDEX_Binoculars = 0x6,
        MGS3_ITEM_INDEX_ThermalGoggles = 0x7,
        MGS3_ITEM_INDEX_NightVisionGoggles = 0x8,
        MGS3_ITEM_INDEX_Camera = 0x9,
        MGS3_ITEM_INDEX_MotionDetector = 0xA,
        MGS3_ITEM_INDEX_ActiveSonar = 0xB,
        MGS3_ITEM_INDEX_MineDetector = 0xC,
        MGS3_ITEM_INDEX_AntiPersonnelSensor = 0xD,
        MGS3_ITEM_INDEX_CBoxA = 0xE,
        MGS3_ITEM_INDEX_CBoxB = 0xF,
        MGS3_ITEM_INDEX_CBoxC = 0x10,
        MGS3_ITEM_INDEX_CBoxD = 0x11,
        MGS3_ITEM_INDEX_CrocCap = 0x12,
        MGS3_ITEM_INDEX_KeyA = 0x13,
        MGS3_ITEM_INDEX_KeyB = 0x14,
        MGS3_ITEM_INDEX_KeyC = 0x15,
        MGS3_ITEM_INDEX_Bandana = 0x16,
        MGS3_ITEM_INDEX_StealthCamouflage = 0x17,
        MGS3_ITEM_INDEX_BugJuice = 0x18,
        MGS3_ITEM_INDEX_MonkeyMask = 0x19,
        MGS3_ITEM_INDEX_Serum = 0x1A,
        MGS3_ITEM_INDEX_Antidote = 0x1B,
        MGS3_ITEM_INDEX_ColdMedicine = 0x1C,
        MGS3_ITEM_INDEX_DigestiveMedicine = 0x1D,
        MGS3_ITEM_INDEX_Ointment = 0x1E,
        MGS3_ITEM_INDEX_Splint = 0x1F,
        MGS3_ITEM_INDEX_Disinfectant = 0x20,
        MGS3_ITEM_INDEX_Styptic = 0x21,
        MGS3_ITEM_INDEX_Bandage = 0x22,
        MGS3_ITEM_INDEX_SutureKit = 0x23,
        MGS3_ITEM_INDEX_Knife = 0x24,
        MGS3_ITEM_INDEX_Battery = 0x25,
        MGS3_ITEM_INDEX_M1911A1Suppressor = 0x26,
        MGS3_ITEM_INDEX_MK22Suppressor = 0x27,
        MGS3_ITEM_INDEX_XM16E1Suppressor = 0x28,
        MGS3_ITEM_INDEX_OliveDrab = 0x29,
        MGS3_ITEM_INDEX_TigerStripe = 0x2A,
        MGS3_ITEM_INDEX_Leaf = 0x2B,
        MGS3_ITEM_INDEX_TreeBark = 0x2C,
        MGS3_ITEM_INDEX_ChocoChip = 0x2D,
        MGS3_ITEM_INDEX_Splitter = 0x2E,
        MGS3_ITEM_INDEX_Raindrop = 0x2F,
        MGS3_ITEM_INDEX_Squares = 0x30,
        MGS3_ITEM_INDEX_Water = 0x31,
        MGS3_ITEM_INDEX_Black = 0x32,
        MGS3_ITEM_INDEX_Snow = 0x33,
        MGS3_ITEM_INDEX_Naked = 0x34,
        MGS3_ITEM_INDEX_SneakingSuit = 0x35,
        MGS3_ITEM_INDEX_Scientist = 0x36,
        MGS3_ITEM_INDEX_Officer = 0x37,
        MGS3_ITEM_INDEX_Maintenance = 0x38,
        MGS3_ITEM_INDEX_Tuxedo = 0x39,
        MGS3_ITEM_INDEX_HornetStripe = 0x3A,
        MGS3_ITEM_INDEX_Spider = 0x3B,
        MGS3_ITEM_INDEX_Moss = 0x3C,
        MGS3_ITEM_INDEX_Fire = 0x3D,
        MGS3_ITEM_INDEX_Spirit = 0x3E,
        MGS3_ITEM_INDEX_ColdWar = 0x3F,
        MGS3_ITEM_INDEX_Snake = 0x40,
        MGS3_ITEM_INDEX_GaKo = 0x41,
        MGS3_ITEM_INDEX_DesertTiger = 0x42,
        MGS3_ITEM_INDEX_DPM = 0x43,
        MGS3_ITEM_INDEX_Flecktarn = 0x44,
        MGS3_ITEM_INDEX_Auscam = 0x45,
        MGS3_ITEM_INDEX_Animals = 0x46,
        MGS3_ITEM_INDEX_Fly = 0x47,
        MGS3_ITEM_INDEX_Banana = 0x48,
        MGS3_ITEM_INDEX_Downloaded = 0x49,
        MGS3_ITEM_INDEX_NoPaint = 0x4A,
        MGS3_ITEM_INDEX_Woodland = 0x4B,
        MGS3_ITEM_INDEX_BlackFace = 0x4C,
        MGS3_ITEM_INDEX_WaterFace = 0x4D,
        MGS3_ITEM_INDEX_DesertFace = 0x4E,
        MGS3_ITEM_INDEX_SplitterFace = 0x4F,
        MGS3_ITEM_INDEX_SnowFace = 0x50,
        MGS3_ITEM_INDEX_Kabuki = 0x51,
        MGS3_ITEM_INDEX_Zombie = 0x52,
        MGS3_ITEM_INDEX_Oyama = 0x53,
        MGS3_ITEM_INDEX_Mask = 0x54,
        MGS3_ITEM_INDEX_Green = 0x55,
        MGS3_ITEM_INDEX_Brown = 0x56,
        MGS3_ITEM_INDEX_Infinity = 0x57,
        MGS3_ITEM_INDEX_SovietUnion = 0x58,
        MGS3_ITEM_INDEX_UK = 0x59,
        MGS3_ITEM_INDEX_France = 0x5A,
        MGS3_ITEM_INDEX_Germany = 0x5B,
        MGS3_ITEM_INDEX_Italy = 0x5C,
        MGS3_ITEM_INDEX_Spain = 0x5D,
        MGS3_ITEM_INDEX_Sweden = 0x5E,
        MGS3_ITEM_INDEX_Japan = 0x5F,
        MGS3_ITEM_INDEX_USA = 0x60,
    };


    enum MGS3FoodIndex : uint8_t
    {
        //  0-10 = Snakes             // 11 total
        MGS3_FOOD_INDEX_KingCobra = 0x0,
        MGS3_FOOD_INDEX_TaiwaneseCobra = 0x1,
        MGS3_FOOD_INDEX_ThaiCobra = 0x2,
        MGS3_FOOD_INDEX_CoralSnake = 0x3,
        MGS3_FOOD_INDEX_MilkSnake = 0x4,
        MGS3_FOOD_INDEX_GreenTreePython = 0x5,
        MGS3_FOOD_INDEX_GiantAnaconda = 0x6,
        MGS3_FOOD_INDEX_ReticulatedPython = 0x7,
        MGS3_FOOD_INDEX_SnakeLiquid = 0x8,
        MGS3_FOOD_INDEX_SnakeSolid = 0x9,
        MGS3_FOOD_INDEX_SnakeSolidus = 0xA,

        MGS3_FOOD_INDEX_IndianGavial = 0xB,

        //  12-14 = Frogs              // 3 total
        MGS3_FOOD_INDEX_OttonFrog = 0xC,
        MGS3_FOOD_INDEX_TreeFrog = 0xD,
        MGS3_FOOD_INDEX_PoisonDartFrog = 0xE,

        MGS3_FOOD_INDEX_Rat = 0xF,
        MGS3_FOOD_INDEX_EuropeanRabbit = 0x10,
        MGS3_FOOD_INDEX_FlyingSquirrel = 0x11,
        MGS3_FOOD_INDEX_Markhor = 0x12,
        MGS3_FOOD_INDEX_VampireBat = 0x13,
        MGS3_FOOD_INDEX_HornetsNest = 0x14,
        MGS3_FOOD_INDEX_EmperorScorpion = 0x15,
        MGS3_FOOD_INDEX_CobaltBlueTarantula = 0x16,

        //  23-27 = Birds              // 5 total
        MGS3_FOOD_INDEX_Parrot = 0x17,
        MGS3_FOOD_INDEX_WhiteRumpedVulture = 0x18,
        MGS3_FOOD_INDEX_RedAvadavat = 0x19,
        MGS3_FOOD_INDEX_Magpie = 0x1A,
        MGS3_FOOD_INDEX_SndWhistlingThrush = 0x1B,

        //  28-30 = Fish               // 3 total
        MGS3_FOOD_INDEX_BigeyeTrevally = 0x1C,
        MGS3_FOOD_INDEX_MaroonShark = 0x1D,
        MGS3_FOOD_INDEX_Arowana = 0x1E,
        MGS3_FOOD_INDEX_KenyanMangroveCrab = 0x1F,

        //  32-38 = Mushrooms          // 7 total
        MGS3_FOOD_INDEX_RussianOysterMushroom = 0x20,
        MGS3_FOOD_INDEX_UralLuminescentMushroom = 0x21,
        MGS3_FOOD_INDEX_SiberianInkCap = 0x22,
        MGS3_FOOD_INDEX_FlyAgaric = 0x23,
        MGS3_FOOD_INDEX_RussianGlowcap = 0x24,
        MGS3_FOOD_INDEX_Spatsa = 0x25,
        MGS3_FOOD_INDEX_BaikalScalyTooth = 0x26,

        //  39-41 = Fruits             // 3 total
        MGS3_FOOD_INDEX_YablokoMoloko = 0x27,
        MGS3_FOOD_INDEX_RussianFalseMango = 0x28,
        MGS3_FOOD_INDEX_Golova = 0x29,
        MGS3_FOOD_INDEX_VineMelon = 0x2A,
        MGS3_FOOD_INDEX_InstantNoodles = 0x2B,

        MGS3_FOOD_INDEX_RussianRation = 0x2C,
        MGS3_FOOD_INDEX_CalorieMate = 0x2D,
        MGS3_FOOD_INDEX_HiveOfPainHornets = 0x2E,
        MGS3_FOOD_INDEX_Tsuchinoko = 0x2F,
    };

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
        GM_CONFIG_CAPTION_OFF = 0x2,
        GM_CONFIG_CUTSCENES_LETTERBOXED = 0x400,
        GM_CONFIG_END_IF_FOUND = 0x800,
        GM_CONFIG_PLAYTIME_STOP = 0x4000,
        GM_CONFIG_GAME_OVER_IF_DISCOVERED = 0x80000
    };


    inline LinkVarValue<short, 2>           GM_Result;
    inline LinkVarValue<short, 4>           GM_Language;
    inline LinkVarValue<short, 6>           GM_GameLevel;
    inline LinkVarValue<unsigned int, 8>    GM_Configuration;
    inline LinkVarValue<short, 10>          GM_ScrAdjX;
    inline LinkVarValue<short, 12>          GM_ScrAdjY;
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

    inline LinkVarValue<short, 746>         GM_Weapon;


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

    enum MGS3CamouflageIndex : uint8_t
    {
        MGS3_CAMOUFLAGE_INDEX_OliveDrab = 0x0,
        MGS3_CAMOUFLAGE_INDEX_TigerStripe = 0x1,
        MGS3_CAMOUFLAGE_INDEX_Leaf = 0x2,
        MGS3_CAMOUFLAGE_INDEX_TreeBark = 0x3,
        MGS3_CAMOUFLAGE_INDEX_ChocoChip = 0x4,
        MGS3_CAMOUFLAGE_INDEX_Splitter = 0x5,
        MGS3_CAMOUFLAGE_INDEX_Raindrop = 0x6,
        MGS3_CAMOUFLAGE_INDEX_Squares = 0x7,
        MGS3_CAMOUFLAGE_INDEX_Water = 0x8,
        MGS3_CAMOUFLAGE_INDEX_Black = 0x9,
        MGS3_CAMOUFLAGE_INDEX_Snow = 0xA,
        MGS3_CAMOUFLAGE_INDEX_Naked = 0xB,
        MGS3_CAMOUFLAGE_INDEX_SneakingSuit = 0xC,
        MGS3_CAMOUFLAGE_INDEX_Scientist = 0xD,
        MGS3_CAMOUFLAGE_INDEX_Officer = 0xE,
        MGS3_CAMOUFLAGE_INDEX_Maintenance = 0xF,
        MGS3_CAMOUFLAGE_INDEX_Tuxedo = 0x10,
        MGS3_CAMOUFLAGE_INDEX_HornetStripe = 0x11,
        MGS3_CAMOUFLAGE_INDEX_Spider = 0x12,
        MGS3_CAMOUFLAGE_INDEX_Moss = 0x13,
        MGS3_CAMOUFLAGE_INDEX_Fire = 0x14,
        MGS3_CAMOUFLAGE_INDEX_Spirit = 0x15,
        MGS3_CAMOUFLAGE_INDEX_ColdWar = 0x16,
        MGS3_CAMOUFLAGE_INDEX_Snake = 0x17,
        MGS3_CAMOUFLAGE_INDEX_GaKo = 0x18,
        MGS3_CAMOUFLAGE_INDEX_DesertTiger = 0x19,
        MGS3_CAMOUFLAGE_INDEX_DPM = 0x1A,
        MGS3_CAMOUFLAGE_INDEX_Flecktarn = 0x1B,
        MGS3_CAMOUFLAGE_INDEX_Auscam = 0x1C,
        MGS3_CAMOUFLAGE_INDEX_Animals = 0x1D,
        MGS3_CAMOUFLAGE_INDEX_Fly = 0x1E,
        MGS3_CAMOUFLAGE_INDEX_Banana = 0x1F,
        MGS3_CAMOUFLAGE_INDEX_Downloaded = 0x20,
    };

    //  33 = Animals ?
    //  34 = mummy?


    inline LinkVarValue<unsigned char, 1662> GM_EquippedCamouflage; 

    enum MGS3FacepaintIndex : uint8_t
    {
        MGS3_FACEPAINT_INDEX_NoPaint = 0x0,
        MGS3_FACEPAINT_INDEX_Woodland = 0x1,
        MGS3_FACEPAINT_INDEX_Black = 0x2,
        MGS3_FACEPAINT_INDEX_Water = 0x3,
        MGS3_FACEPAINT_INDEX_Desert = 0x4,
        MGS3_FACEPAINT_INDEX_Splitter = 0x5,
        MGS3_FACEPAINT_INDEX_Snow = 0x6,
        MGS3_FACEPAINT_INDEX_Kabuki = 0x7,
        MGS3_FACEPAINT_INDEX_Zombie = 0x8,
        MGS3_FACEPAINT_INDEX_Oyama = 0x9,
        MGS3_FACEPAINT_INDEX_Mask = 0xA,
        MGS3_FACEPAINT_INDEX_Green = 0xB,
        MGS3_FACEPAINT_INDEX_Brown = 0xC,
        MGS3_FACEPAINT_INDEX_Infinity = 0xD,
        MGS3_FACEPAINT_INDEX_SovietUnion = 0xE,
        MGS3_FACEPAINT_INDEX_UK = 0xF,
        MGS3_FACEPAINT_INDEX_France = 0x10,
        MGS3_FACEPAINT_INDEX_Germany = 0x11,
        MGS3_FACEPAINT_INDEX_Italy = 0x12,
        MGS3_FACEPAINT_INDEX_Spain = 0x13,
        MGS3_FACEPAINT_INDEX_Sweden = 0x14,
        MGS3_FACEPAINT_INDEX_Japan = 0x15,
        MGS3_FACEPAINT_INDEX_USA = 0x16,
    };

    inline LinkVarValue<unsigned char, 1663> GM_EquippedFacepaint;

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



    inline LinkVarValue<short, 11024>    GM_ResetLoadCount;
}

