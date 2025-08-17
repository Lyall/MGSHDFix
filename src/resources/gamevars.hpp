#pragma once

class GameVars final
{
private:
    static void OnLevelTransition();

    uint64_t* aimingState = nullptr;
    int* cutsceneFlag = nullptr;
    int* scriptedSequenceFlag = nullptr;
    double* actorWaitValue = nullptr;
    const char* currentStage = nullptr;
    uint32_t* heldTriggers = nullptr;

public:
    void Initialize();
    bool InCutscene() const; // If we're in a full demo cutscene.
    bool InScriptedSequence() const; // If the game is in a scripted sequence (cutscene or pad demo).
    double ActorWaitMultiplier() const;

    std::string GetRichPresenceString() const;
    std::string GetGameMode() const;
    const char* GetCurrentStage() const;
    bool IsStage(const char* stageConst) const;
    bool IsAnyStage(std::initializer_list<const char*> stages) const;

    void SetAimingState(uint64_t state) const;
    uint64_t GetAimingState() const;

    enum HoldingTriggers : uint32_t
    {
        // MGS2 analog byte triggers
        MGS2_WeaponMenu = 0xFF000000u,
        MGS2_EquipmentMenu = 0x00FF0000u,
        MGS2_FirstPerson = 0x0000FF00u,
        MGS2_LockOn = 0x000000FFu,

        // MGS3 boolean bit triggers
        MGS3_EquipmentMenu = 1u << 8,   // 0x00000100
        MGS3_WeaponMenu = 1u << 9,   // 0x00000200
        MGS3_LockOn = 1u << 10,  // 0x00000400
        MGS3_FirstPerson = 1u << 11   // 0x00000800
    };

    bool MGS2IsHoldingWeaponMenu() const;
    bool MGS2IsHoldingEquipmentMenu() const;
    bool MGS2IsHoldingFirstPerson() const;
    bool MGS2IsHoldingLockOn() const;
    bool MGS3IsHoldingWeaponMenu() const;
    bool MGS3IsHoldingEquipmentMenu() const;
    bool MGS3IsHoldingFirstPerson() const;
    bool MGS3IsHoldingLockOn() const;
};

inline GameVars g_GameVars;



enum MGS2WeaponIndex : uint8_t
{
    MGS2_WEAPON_INDEX_M9 = 0x1,
    MGS2_WEAPON_INDEX_USP = 0x2,
    MGS2_WEAPON_INDEX_SOCOM = 0x3,
    MGS2_WEAPON_INDEX_PSG1 = 0x4,
    MGS2_WEAPON_INDEX_RGB6 = 0x5,
    MGS2_WEAPON_INDEX_NIKITA = 0x6,
    MGS2_WEAPON_INDEX_STINGER = 0x7,
    MGS2_WEAPON_INDEX_CLAYMORE = 0x8,
    MGS2_WEAPON_INDEX_C4 = 0x9,
    MGS2_WEAPON_INDEX_CHAFF_GRENADE = 0xA,
    MGS2_WEAPON_INDEX_STUN_GRENADE = 0xB,
    MGS2_WEAPON_INDEX_D_MIC = 0xC,
    MGS2_WEAPON_INDEX_HIGH_FREQUENCY_BLADE = 0xD,
    MGS2_WEAPON_INDEX_COOLANT = 0xE,
    MGS2_WEAPON_INDEX_AKS74U = 0xF,
    MGS2_WEAPON_INDEX_MAGAZINE = 0x10,
    MGS2_WEAPON_INDEX_GRENADE = 0x11,
    MGS2_WEAPON_INDEX_M4 = 0x12,
    MGS2_WEAPON_INDEX_PSG1T = 0x13,
    MGS2_WEAPON_INDEX_D_MIC_ZOOMED = 0x14,
    MGS2_WEAPON_INDEX_BOOK = 0x15,
};

enum MGS3WeaponIndex : uint8_t
{
    MGS3_WEAPON_INDEX_MK22 = 0x5,
    MGS3_WEAPON_INDEX_M1911A1 = 0x6,
    MGS3_WEAPON_INDEX_EzGun = 0x7,
    MGS3_WEAPON_INDEX_SAA = 0x8,
    MGS3_WEAPON_INDEX_Patriot = 0x9,
    MGS3_WEAPON_INDEX_Scorpion = 0xA,
    MGS3_WEAPON_INDEX_XM16E1 = 0xB,
    MGS3_WEAPON_INDEX_AK47 = 0xC,
    MGS3_WEAPON_INDEX_M63 = 0xD,
    MGS3_WEAPON_INDEX_M37 = 0xE,
    MGS3_WEAPON_INDEX_SVD = 0xF,
    MGS3_WEAPON_INDEX_Mosin = 0x10,
    MGS3_WEAPON_INDEX_RPG7 = 0x11,

    /*
    MGS3_WEAPON_INDEX_SurvivalKnife     = 0x,
    MGS3_WEAPON_INDEX_Fork              = 0x,
    MGS3_WEAPON_INDEX_CigSpray          = 0x,
    MGS3_WEAPON_INDEX_Handkerchief      = 0x,
    MGS3_WEAPON_INDEX_Torch             = 0x,
    MGS3_WEAPON_INDEX_Grenade           = 0x,
    MGS3_WEAPON_INDEX_WpGrenade         = 0x,
    MGS3_WEAPON_INDEX_StunGrenade       = 0x,
    MGS3_WEAPON_INDEX_ChaffGrenade      = 0x,
    MGS3_WEAPON_INDEX_SmokeGrenade      = 0x,
    MGS3_WEAPON_INDEX_EmptyMag          = 0x,
    MGS3_WEAPON_INDEX_TNT               = 0x,
    MGS3_WEAPON_INDEX_C3                = 0x,
    MGS3_WEAPON_INDEX_Claymore          = 0x,
    MGS3_WEAPON_INDEX_Book              = 0x,
    MGS3_WEAPON_INDEX_Mousetrap         = 0x,
    MGS3_WEAPON_INDEX_DirectionalMic    = 0x,
    */
};

struct Stage
{
    const char* sStageId;        
    const char* sGameMode;        
    const char* sRichPresenceName; 
};

#define MGS2_STAGE_LIST \
    /* Menus                 */ \
    X(N_TITLE,   "n_title",   "Menu", "Title Screen") \
    X(SELECT,    "select",    "Menu", "Main Menu") \
    X(MSELECT,   "mselect",   "Menu", "VR Missions Menu") \
    X(TALES,     "tales",     "Menu", "Snake Tales Menu") \
    \
    /* Tanker (Playable)     */ \
    X(W00A, "w00a", "Tanker", "Deck A - Port") \
    X(W00B, "w00b", "Tanker", "Deck A - Starboard (Olga)") \
    X(W00C, "w00c", "Tanker", "Navigational Deck") \
    X(W01A, "w01a", "Tanker", "Deck A Crew Quarters") \
    X(W01B, "w01b", "Tanker", "Deck A Crew Quarters Starboard") \
    X(W01C, "w01c", "Tanker", "Deck C Crew Quarters") \
    X(W01D, "w01d", "Tanker", "Deck D Crew Quarters") \
    X(W01E, "w01e", "Tanker", "Deck E Bridge") \
    X(W01F, "w01f", "Tanker", "Deck A Crew Lounge") \
    X(W02A, "w02a", "Tanker", "Engine Room") \
    X(W03A, "w03a", "Tanker", "Deck 2 Port") \
    X(W03B, "w03b", "Tanker", "Deck 2 Starboard") \
    X(W04A, "w04a", "Tanker", "Hold No.1") \
    X(W04B, "w04b", "Tanker", "Hold No.2") \
    X(W04C, "w04c", "Tanker", "Hold No.3") \
    \
    /* Tanker (Cutscenes)    */ \
    X(D00T,   "d00t",   "Tanker", "George Washington Bridge") /*Tanker Opening*/ \
    X(D01T,   "d01t",   "Tanker", "Navigational Deck") /*Russian Invasion*/\
    X(D04T,   "d04t",   "Tanker", "Identifying Choppers") \
    X(D05T,   "d05t",   "Tanker", "Olga Cutscene") \
    X(D10T,   "d10t",   "Tanker", "Cutscene d10t") \
    X(D11T,   "d11t",   "Tanker", "Cutscene d11t") \
    X(D12T,   "d12t",   "Tanker", "Cutscene d12t") \
    X(D12T3,  "d12t3",  "Tanker", "Cutscene d12t3") \
    X(D12T4,  "d12t4",  "Tanker", "Cutscene d12t4") \
    X(D13T,   "d13t",   "Tanker", "Cutscene d13t (NG+ Rewards)") \
    X(D14T,   "d14t",   "Tanker", "Cutscene d14t") \
    \
    /* Plant (Playable)      */ \
    X(W11A, "w11a", "Plant", "Strut A - Dock") \
    X(W11B, "w11b", "Plant", "Strut A - Dock (Bomb Disposal)") \
    X(W11C, "w11c", "Plant", "Strut A - Dock (Fortune)") \
    X(W12A, "w12a", "Plant", "Strut A Roof") \
    X(W12B, "w12b", "Plant", "Strut A Pump Room") \
    X(W12C, "w12c", "Plant", "Strut A Roof (Bomb)") \
    X(W13A, "w13a", "Plant", "AB Connecting Bridge") \
    X(W13B, "w13b", "Plant", "AB Connecting Bridge (Sensor B)") \
    X(W14A, "w14a", "Plant", "Transformer Room") \
    X(W15A, "w15a", "Plant", "BC Connecting Bridge") \
    X(W15B, "w15b", "Plant", "BC Connecting Bridge (After Stillman)") \
    X(W16A, "w16a", "Plant", "Dining Hall") \
    X(W16B, "w16b", "Plant", "Dining Hall (Post Stillman)") \
    X(W17A, "w17a", "Plant", "CD Connecting Bridge") \
    X(W18A, "w18a", "Plant", "Sediment Pool") \
    X(W19A, "w19a", "Plant", "DE Connecting Bridge") \
    X(W20A, "w20a", "Plant", "Parcel Room") \
    X(W20B, "w20b", "Plant", "Heliport") \
    X(W20C, "w20c", "Plant", "Heliport (Bomb)") \
    X(W20D, "w20d", "Plant", "Heliport (Post Ninja)") \
    X(W21A, "w21a", "Plant", "EF Connecting Bridge") \
    X(W21B, "w21b", "Plant", "EF Connecting Bridge 2") \
    X(W22A, "w22a", "Plant", "Warehouse") \
    X(W23A, "w23a", "Plant", "FA Connecting Bridge") \
    X(W23B, "w23b", "Plant", "FA Connecting Bridge (Post Shell 2)") \
    X(W24A, "w24a", "Plant", "Shell 1 Core - 1F") \
    X(W24B, "w24b", "Plant", "Shell 1 Core - B1") \
    X(W24C, "w24c", "Plant", "Shell 1 Core - Hostage Room") \
    X(W24D, "w24d", "Plant", "Shell 1 Core - B2") \
    X(W25A, "w25a", "Plant", "Shell Connecting Bridge") \
    X(W25B, "w25b", "Plant", "Shell Connecting Bridge (Destroyed)") \
    X(W25C, "w25c", "Plant", "Strut L Perimeter") \
    X(W25D, "w25d", "Plant", "KL Connecting Bridge") \
    X(W28A, "w28a", "Plant", "Sewage Treatment") \
    X(W31A, "w31a", "Plant", "Shell 2 Core") \
    X(W31B, "w31b", "Plant", "Shell 2 Filtration Chamber No.1") \
    X(W31C, "w31c", "Plant", "Shell 2 Filtration Chamber No.2") \
    X(W31D, "w31d", "Plant", "Shell 2 Core (With Emma)") \
    X(W32A, "w32a", "Plant", "Oil Fence") \
    X(W32B, "w32b", "Plant", "Oil Fence (Vamp Fight)") \
    X(W41A, "w41a", "Plant", "GW - Stomach") \
    X(W42A, "w42a", "Plant", "GW - Jejunum") \
    X(W43A, "w43a", "Plant", "GW - Ascending Colon") \
    X(W44A, "w44a", "Plant", "GW - Ileum") \
    X(W45A, "w45a", "Plant", "GW - Sigmoid Colon") \
    X(W46A, "w46a", "Plant", "GW - Rectum") \
    X(W51A, "w51a", "Plant", "Arsenal Gear") \
    X(W61A, "w61a", "Plant", "Federal Hall") \
    \
    /* Plant (Cutscenes)     */ \
    X(MUSEUM,   "museum",   "Plant", "Briefing") \
    X(WEBDEMO,  "webdemo",  "Plant", "Web Demo") \
    X(ENDING,   "ending",   "Plant", "Results Screen") \
    X(D001P01,  "d001p01",  "Plant", "Plant Opening") \
    X(D001P02,  "d001p02",  "Plant", "Sea Dock Cutscene") \
    X(D005P01,  "d005p01",  "Plant", "Raiden On Elevator") \
    X(D005P03,  "d005p03",  "Plant", "Strut A Roof Cutscene") \
    X(D010P01,  "d010p01",  "Plant", "Meeting Vamp") \
    X(D012P01,  "d012p01",  "Plant", "ADUD") \
    X(D014P01,  "d014p01",  "Plant", "Stillman Cutscene") \
    X(D021P01,  "d021p01",  "Plant", "Fatman and Ninja") \
    X(D036P03,  "d036p03",  "Plant", "Hostage Cutscene") \
    X(D036P05,  "d036p05",  "Plant", "Shell 1 Cutscene") \
    X(D045P01,  "d045p01",  "Plant", "Cutscene d045p01") \
    X(D046P01,  "d046p01",  "Plant", "Cutscene d046p01") \
    X(D053P01,  "d053p01",  "Plant", "Cutscene d053p01") \
    X(D055P01,  "d055p01",  "Plant", "Cutscene d055p01") \
    X(D063P01,  "d063p01",  "Plant", "Cutscene d063p01") \
    X(D065P02,  "d065p02",  "Plant", "Cutscene d065p02") \
    X(D070P01,  "d070p01",  "Plant", "Cutscene d070p01") \
    X(D070P09,  "d070p09",  "Plant", "Cutscene d070p09") \
    X(D070PX9,  "d070px9",  "Plant", "Cutscene d070px9") \
    X(D078P01,  "d078p01",  "Plant", "Cutscene d078p01") \
    X(D080P01,  "d080p01",  "Plant", "Cutscene d080p01") \
    X(D080P06,  "d080p06",  "Plant", "Cutscene d080p06") \
    X(D080P07,  "d080p07",  "Plant", "Cutscene d080p07") \
    X(D080P08,  "d080p08",  "Plant", "Cutscene d080p08") \
    X(D082P01,  "d082p01",  "Plant", "Cutscene d082p01") \
    \
    /* Alternate Tanker maps */ \
    X(A00A, "a00a", "Alternate", "Deck A - Port (Alternate)") \
    X(A00B, "a00b", "Alternate", "Navigational Deck (Alternate)") \
    X(A00C, "a00c", "Alternate", "Navigational Deck (Unused Alt)") \
    X(A01A, "a01a", "Alternate", "Deck A Crew Quarters (Alternate)") \
    X(A01B, "a01b", "Alternate", "Deck A Crew Quarters Starboard (Alternate)") \
    X(A01C, "a01c", "Alternate", "Deck C Crew Quarters (Alternate)") \
    X(A01D, "a01d", "Alternate", "Deck D Crew Quarters (Alternate)") \
    X(A01E, "a01e", "Alternate", "Deck E Bridge (Alternate)") \
    X(A01F, "a01f", "Alternate", "Deck A Crew Lounge (Alternate)") \
    X(A02A, "a02a", "Alternate", "Engine Room (Alternate)") \
    X(A03A, "a03a", "Alternate", "Deck 2 Port (Alternate)") \
    X(A03B, "a03b", "Alternate", "Deck 2 Starboard (Alternate)") \
    X(A04A, "a04a", "Alternate", "Hold No.1 (Alternate)") \
    X(A04B, "a04b", "Alternate", "Hold No.2 (Alternate)") \
    X(A04C, "a04c", "Alternate", "Hold No.3 (Alternate)") \
    /* Alternate Plant and Snake Tales maps */ \
    X(A11A, "a11a", "Alternate", "Strut A - Dock (Snake Tales)") \
    X(A11B, "a11b", "Alternate", "Strut A - Dock (Bomb) (Snake Tales)") \
    X(A11C, "a11c", "Alternate", "Strut A - Dock (Fortune) (Snake Tales)") \
    X(A12A, "a12a", "Alternate", "Strut A Roof (Snake Tales)") \
    X(A12B, "a12b", "Alternate", "Strut A Pump Room (Snake Tales)") \
    X(A12C, "a12c", "Alternate", "Strut A Roof (Bomb) (Snake Tales)") \
    X(A13A, "a13a", "Alternate", "AB Connecting Bridge (Snake Tales)") \
    X(A13B, "a13b", "Alternate", "AB Connecting Bridge (Sensor B) (Snake Tales)") \
    X(A14A, "a14a", "Alternate", "Transformer Room (Snake Tales)") \
    X(A15A, "a15a", "Alternate", "BC Connecting Bridge (Snake Tales)") \
    X(A15B, "a15b", "Alternate", "BC Connecting Bridge (After Stillman) (Snake Tales)") \
    X(A16A, "a16a", "Alternate", "Dining Hall (Snake Tales)") \
    X(A16B, "a16b", "Alternate", "Dining Hall (Post Stillman) (Snake Tales)") \
    X(A17A, "a17a", "Alternate", "CD Connecting Bridge (Snake Tales)") \
    X(A18A, "a18a", "Alternate", "Sediment Pool (Snake Tales)") \
    X(A19A, "a19a", "Alternate", "DE Connecting Bridge (Snake Tales)") \
    X(A20A, "a20a", "Alternate", "Parcel Room (Snake Tales)") \
    X(A20B, "a20b", "Alternate", "Heliport (Snake Tales)") \
    X(A20C, "a20c", "Alternate", "Heliport (Bomb) (Snake Tales)") \
    X(A20D, "a20d", "Alternate", "Heliport (Post Ninja) (Snake Tales)") \
    X(A21A, "a21a", "Alternate", "EF Connecting Bridge (Snake Tales)") \
    X(A22A, "a22a", "Alternate", "Warehouse (Snake Tales)") \
    X(A23B, "a23b", "Alternate", "FA Connecting Bridge (Snake Tales)") \
    X(A24A, "a24a", "Alternate", "Shell 1 Core - 1F (Snake Tales)") \
    X(A24B, "a24b", "Alternate", "Shell 1 Core - B1 (Snake Tales)") \
    X(A24C, "a24c", "Alternate", "Shell 1 Core - Hostage Room (Snake Tales)") \
    X(A24D, "a24d", "Alternate", "Shell 1 Core - B2 (Snake Tales)") \
    X(A25A, "a25a", "Alternate", "Shell Connecting Bridge (Snake Tales)") \
    X(A25B, "a25b", "Alternate", "Shell Connecting Bridge (Destroyed) (Snake Tales)") \
    X(A25C, "a25c", "Alternate", "Strut L Perimeter (Snake Tales)") \
    X(A25D, "a25d", "Alternate", "KL Connecting Bridge (Snake Tales)") \
    X(A28A, "a28a", "Alternate", "Sewage Treatment (Snake Tales)") \
    X(A31A, "a31a", "Alternate", "Shell 2 Core (Snake Tales)") \
    X(A31B, "a31b", "Alternate", "Shell 2 Filtration Chamber No.1 (Snake Tales)") \
    X(A31C, "a31c", "Alternate", "Shell 2 Filtration Chamber No.2 (Snake Tales)") \
    X(A31D, "a31d", "Alternate", "Shell 2 Core (With Emma) (Snake Tales)") \
    X(A32A, "a32a", "Alternate", "Oil Fence (Snake Tales)") \
    X(A32B, "a32b", "Alternate", "Oil Fence (Vamp Fight) (Snake Tales)") \
    X(A41A, "a41a", "Alternate", "GW - Stomach (Snake Tales)") \
    X(A42A, "a42a", "Alternate", "GW - Jejunum (Snake Tales)") \
    X(A43A, "a43a", "Alternate", "GW - Ascending Colon (Snake Tales)") \
    X(A44A, "a44a", "Alternate", "GW - Ileum (Snake Tales)") \
    X(A45A, "a45a", "Alternate", "GW - Sigmoid Colon (Snake Tales)") \
    X(A46A, "a46a", "Alternate", "GW - Rectum (Snake Tales)") \
    X(A51A, "a51a", "Alternate", "Arsenal Gear (Snake Tales)") \
    X(A61A, "a61a", "Alternate", "Federal Hall (Snake Tales)") \
    \
    /* VR: Sneaking          */ \
    X(VS01A, "vs01a", "VR: Sneaking", "Level 1") \
    X(VS02A, "vs02a", "VR: Sneaking", "Level 2") \
    X(VS03A, "vs03a", "VR: Sneaking", "Level 3") \
    X(VS04A, "vs04a", "VR: Sneaking", "Level 4") \
    X(VS05A, "vs05a", "VR: Sneaking", "Level 5") \
    X(VS06A, "vs06a", "VR: Sneaking", "Level 6") \
    X(VS07A, "vs07a", "VR: Sneaking", "Level 7") \
    X(VS08A, "vs08a", "VR: Sneaking", "Level 8") \
    X(VS09A, "vs09a", "VR: Sneaking", "Level 9") \
    X(VS10A, "vs10a", "VR: Sneaking", "Level 10") \
    \
    /* VR: Variety           */ \
    X(SP01A, "sp01a", "VR: Variety", "Level 1") \
    X(SP02A, "sp02a", "VR: Variety", "Level 2") \
    X(SP03A, "sp03a", "VR: Variety", "Level 3") \
    X(SP04A, "sp04a", "VR: Variety", "Level 4") \
    X(SP05A, "sp05a", "VR: Variety", "Level 5") \
    X(SP06A, "sp06a", "VR: Variety", "Level 6") \
    X(SP07A, "sp07a", "VR: Variety", "Level 7") \
    X(SP08A, "sp08a", "VR: Variety", "Level 8") \
    \
    /* VR: First-Person      */ \
    X(SP21A, "sp21a", "VR: First-Person", "Level 1") \
    X(SP22A, "sp22a", "VR: First-Person", "Level 2") \
    X(SP23A, "sp23a", "VR: First-Person", "Level 3") \
    X(SP24A, "sp24a", "VR: First-Person", "Level 4") \
    X(SP25A, "sp25a", "VR: First-Person", "Level 5") \
    \
    /* VR: Streaking         */ \
    X(ST01A, "st01a", "VR: Streaking", "Level 1") \
    X(ST02A, "st02a", "VR: Streaking", "Level 2") \
    X(ST03A, "st03a", "VR: Streaking", "Level 3") \
    X(ST04A, "st04a", "VR: Streaking", "Level 4") \
    \
    /* VR: Weapons           */ \
    X(WP01A, "wp01a", "VR: Weapons - SOCOM",   "Level 1") \
    X(WP02A, "wp02a", "VR: Weapons - SOCOM",   "Level 2") \
    X(WP03A, "wp03a", "VR: Weapons - SOCOM",   "Level 3") \
    X(WP04A, "wp04a", "VR: Weapons - SOCOM",   "Level 4") \
    X(WP05A, "wp05a", "VR: Weapons - SOCOM",   "Level 5") \
    X(WP11A, "wp11a", "VR: Weapons - M4",      "Level 1") \
    X(WP12A, "wp12a", "VR: Weapons - M4",      "Level 2") \
    X(WP13A, "wp13a", "VR: Weapons - M4",      "Level 3") \
    X(WP14A, "wp14a", "VR: Weapons - M4",      "Level 4") \
    X(WP15A, "wp15a", "VR: Weapons - M4",      "Level 5") \
    X(WP21A, "wp21a", "VR: Weapons - Claymore","Level 1") \
    X(WP22A, "wp22a", "VR: Weapons - Claymore","Level 2") \
    X(WP23A, "wp23a", "VR: Weapons - Claymore","Level 3") \
    X(WP24A, "wp24a", "VR: Weapons - Claymore","Level 4") \
    X(WP25A, "wp25a", "VR: Weapons - Claymore","Level 5") \
    X(WP31A, "wp31a", "VR: Weapons - Grenade", "Level 1") \
    X(WP32A, "wp32a", "VR: Weapons - Grenade", "Level 2") \
    X(WP33A, "wp33a", "VR: Weapons - Grenade", "Level 3") \
    X(WP34A, "wp34a", "VR: Weapons - Grenade", "Level 4") \
    X(WP35A, "wp35a", "VR: Weapons - Grenade", "Level 5") \
    X(WP41A, "wp41a", "VR: Weapons - PSG1",    "Level 1") \
    X(WP42A, "wp42a", "VR: Weapons - PSG1",    "Level 2") \
    X(WP43A, "wp43a", "VR: Weapons - PSG1",    "Level 3") \
    X(WP44A, "wp44a", "VR: Weapons - PSG1",    "Level 4") \
    X(WP45A, "wp45a", "VR: Weapons - PSG1",    "Level 5") \
    X(WP51A, "wp51a", "VR: Weapons - Stinger", "Level 1") \
    X(WP52A, "wp52a", "VR: Weapons - Stinger", "Level 2") \
    X(WP53A, "wp53a", "VR: Weapons - Stinger", "Level 3") \
    X(WP54A, "wp54a", "VR: Weapons - Stinger", "Level 4") \
    X(WP55A, "wp55a", "VR: Weapons - Stinger", "Level 5") \
    X(WP61A, "wp61a", "VR: Weapons - Nikita",  "Level 1") \
    X(WP62A, "wp62a", "VR: Weapons - Nikita",  "Level 2") \
    X(WP63A, "wp63a", "VR: Weapons - Nikita",  "Level 3") \
    X(WP64A, "wp64a", "VR: Weapons - Nikita",  "Level 4") \
    X(WP65A, "wp65a", "VR: Weapons - Nikita",  "Level 5") \
    X(WP71A, "wp71a", "VR: Weapons - No Weapon", "Level 1") \
    X(WP72A, "wp72a", "VR: Weapons - No Weapon", "Level 2") \
    X(WP73A, "wp73a", "VR: Weapons - No Weapon", "Level 3") \
    X(WP74A, "wp74a", "VR: Weapons - No Weapon", "Level 4") \
    X(WP75A, "wp75a", "VR: Weapons - No Weapon", "Level 5")

namespace MGS2Stages
{
#define X(name, id, mode, disp) inline constexpr const char* name = id;
    MGS2_STAGE_LIST
#undef X
}


#define MGS3_STAGE_LIST \
    /* Menus / Intro / Ending */ \
    X(KYLE_OP, "kyle_op", "Menu", "Opening Snake Eater Video") \
    X(TITLE,   "title",   "Menu", "Title Screen / Main Menu") \
    X(THEATER, "theater", "Menu", "Demo Theater") \
    X(ENDING,  "ending",  "Menu", "Credits") \
    \
    /* Virtuous Mission (VM) and early Snake Eater mirrors */ \
    X(V001A, "v001a", "Virtuous Mission", "Dremuchji South") \
    X(V003A, "v003a", "Virtuous Mission", "Dremuchji Swampland") \
    X(V004A, "v004a", "Virtuous Mission", "Dremuchji North") \
    X(V005A, "v005a", "Virtuous Mission", "Dolinovodno Rope Bridge") \
    X(V006A, "v006a", "Virtuous Mission", "Rassvet") \
    X(V006B, "v006b", "Virtuous Mission", "Rassvet") \
    X(V007A, "v007a", "Virtuous Mission", "Dolinovodno Riverbank") \
    \
    X(S001A, "s001a", "Snake Eater", "Dremuchji South") \
    X(S002A, "s002a", "Snake Eater", "Dremuchji East") \
    X(S003A, "s003a", "Snake Eater", "Dremuchji Swampland") \
    X(S004A, "s004a", "Snake Eater", "Dremuchji North") \
    X(S005A, "s005a", "Snake Eater", "Dolinovodno Rope Bridge") \
    X(S006A, "s006a", "Snake Eater", "Rassvet") \
    X(S006B, "s006b", "Snake Eater", "Rassvet") \
    X(S007A, "s007a", "Snake Eater", "VM Bridge Fall Aftermath Zone") \
    X(S012A, "s012a", "Snake Eater", "Chyornyj Prud (Swamp)") \
    X(S021A, "s021a", "Snake Eater", "Bolshaya Past South") \
    X(S022A, "s022a", "Snake Eater", "Bolshaya Past North") \
    X(S023A, "s023a", "Snake Eater", "Bolshaya Past Crevice (Ocelot Battle)") \
    \
    /* Chyornaya Peschera (The Pain) */ \
    X(S031A, "s031a", "Snake Eater", "Chyornaya Peschera Cave Branch") \
    X(S032A, "s032a", "Snake Eater", "Chyornaya Peschera Cave") \
    X(S032B, "s032b", "Snake Eater", "Chyornaya Peschera Cave (The Pain Boss)") \
    X(S033A, "s033a", "Snake Eater", "Chyornaya Peschera Cave Entrance") \
    \
    /* Ponizovje (water / armory / warehouse) */ \
    X(S041A, "s041a", "Snake Eater", "Ponizovje South (Watercraft)") \
    X(S042A, "s042a", "Snake Eater", "Ponizovje Armory (SVD)") \
    X(S043A, "s043a", "Snake Eater", "Ponizovje Warehouse (Exterior)")/*(Kill The End Early)*/\
    X(S044A, "s044a", "Snake Eater", "Ponizovje Warehouse (Interior)") \
    \
    /* Graniny Gorki (The Fear; labs) */ \
    X(S051A, "s051a", "Snake Eater", "Graniny Gorki South (The Fear)") \
    X(S051B, "s051b", "Snake Eater", "Graniny Gorki South (The Fear)") \
    X(S052A, "s052a", "Snake Eater", "Graniny Gorki Lab Exterior: Outside Walls") \
    X(S052B, "s052b", "Snake Eater", "Graniny Gorki Lab Exterior: Inside Walls") \
    X(S053A, "s053a", "Snake Eater", "Graniny Gorki Lab 1F/2F") \
    X(S054A, "s054a", "Snake Eater", "Graniny Gorki Lab Interior") \
    X(S055A, "s055a", "Snake Eater", "Graniny Gorki Lab B1 (Prison Cells)") \
    X(S056A, "s056a", "Snake Eater", "Graniny Gorki Lab B1 (Granin Basement)") \
    \
    /* Svyatogornyj / Sokrovenno (The End / Ocelot Unit) */ \
    X(S045A, "s045a", "Snake Eater", "Svyatogornyj South") \
    X(S061A, "s061a", "Snake Eater", "Svyatogornyj West") \
    X(S062A, "s062a", "Snake Eater", "Svyatogornyj East (M63 House)") \
    X(S063A, "s063a", "Snake Eater", "Sokrovenno South (The End)") \
    X(S063B, "s063b", "Snake Eater", "Sokrovenno South (Ocelot Unit)") \
    X(S064A, "s064a", "Snake Eater", "Sokrovenno West (The End, River)") \
    X(S064B, "s064b", "Snake Eater", "Sokrovenno West (Ocelot Unit, River)") \
    X(S065A, "s065a", "Snake Eater", "Sokrovenno North (The End's Death)") \
    X(S065B, "s065b", "Snake Eater", "Sokrovenno North (To Krasnogorje Tunnel)") \
    X(S066A, "s066a", "Snake Eater", "Krasnogorje Tunnel (Ladder)") \
    \
    /* Krasnogorje Mountain approach */ \
    X(S071A, "s071a", "Snake Eater", "Krasnogorje Mountain Base") \
    X(S072A, "s072a", "Snake Eater", "Krasnogorje Mountainside (Hovercraft)") \
    X(S072B, "s072b", "Snake Eater", "Krasnogorje Mountainside (Hind)") \
    X(S073A, "s073a", "Snake Eater", "Krasnogorje Mountaintop (Before Eva)") \
    X(S073B, "s073b", "Snake Eater", "Krasnogorje Mountaintop (After Eva)") \
    X(S074A, "s074a", "Snake Eater", "Krasnogorje Mountaintop Ruins (Eva Cutscene)") \
    X(S075A, "s075a", "Snake Eater", "Krasnogorje Mountaintop: Behind Ruins") \
    \
    /* Groznyj Grad - all names explicitly prefixed */ \
    X(S081A, "s081a", "Snake Eater", "Groznyj Grad Underground Tunnel (The Fury)") \
    X(S091A, "s091a", "Snake Eater", "Groznyj Grad Southwest") \
    X(S091B, "s091b", "Snake Eater", "Groznyj Grad Southwest (During Escape)") \
    X(S091C, "s091c", "Snake Eater", "Groznyj Grad Southwest (Return)") \
    X(S092A, "s092a", "Snake Eater", "Groznyj Grad Northwest") \
    X(S092B, "s092b", "Snake Eater", "Groznyj Grad Northwest (During Escape)") \
    X(S092C, "s092c", "Snake Eater", "Groznyj Grad Northwest (Return)") \
    X(S093A, "s093a", "Snake Eater", "Groznyj Grad Northeast") \
    X(S093B, "s093b", "Snake Eater", "Groznyj Grad Northeast (During Escape)") \
    X(S093C, "s093c", "Snake Eater", "Groznyj Grad Northeast (Return)") \
    X(S094A, "s094a", "Snake Eater", "Groznyj Grad Southeast") \
    X(S094B, "s094b", "Snake Eater", "Groznyj Grad Southeast (During Escape)") \
    X(S094C, "s094c", "Snake Eater", "Groznyj Grad Southeast (Return)") \
    X(S101A, "s101a", "Snake Eater", "Groznyj Grad Weapons Lab: East Wing") \
    X(S101B, "s101b", "Snake Eater", "Groznyj Grad Weapons Lab: East Wing (C3 Mission)") \
    X(S111A, "s111a", "Snake Eater", "Groznyj Grad Weapons Lab: West Wing Corridor") \
    X(S112A, "s112a", "Snake Eater", "Groznyj Grad Torture Room") \
    X(S113A, "s113a", "Snake Eater", "Groznyj Grad Sewers") \
    X(S121A, "s121a", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing") \
    X(S121B, "s121b", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing (C3 Mission)") \
    X(S122A, "s122a", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing B1 (Volgin)") \
    \
    /* Sorrow / Tikhogornyj */ \
    X(S141A, "s141a", "Snake Eater", "The Sorrow (Boss Battle)") \
    X(S151A, "s151a", "Snake Eater", "Tikhogornyj") \
    X(S152A, "s152a", "Snake Eater", "Tikhogornyj: Behind Waterfall") \
    \
    /* Bike chase / Shagohod / Bridge */ \
    X(S161A, "s161a", "Snake Eater", "Groznyj Grad (Bike Chase 1)") \
    X(S162A, "s162a", "Snake Eater", "Groznyj Grad Runway South (Bike Chase 2)") \
    X(S163A, "s163a", "Snake Eater", "Groznyj Grad Runway (Bike Chase 3)") \
    X(S163B, "s163b", "Snake Eater", "Groznyj Grad Runway (Shagohod Fight)") \
    X(S171A, "s171a", "Snake Eater", "Groznyj Grad Rail Bridge (C3)") \
    X(S171B, "s171b", "Snake Eater", "Groznyj Grad Rail Bridge (Shagohod Battle)") \
    X(S181A, "s181a", "Snake Eater", "Groznyj Grad Rail Bridge North (Escape)") \
    X(S182A, "s182a", "Snake Eater", "Lazorevo South (Bike Chase)") \
    X(S183A, "s183a", "Snake Eater", "Lazorevo North (Final Bike Chase)") \
    \
    /* Finale */ \
    X(S191A, "s191a", "Snake Eater", "Zaozyorje West") \
    X(S192A, "s192a", "Snake Eater", "Zaozyorje East") \
    X(S201A, "s201a", "Snake Eater", "Rokovj Bereg (The Boss Arena)") \
    X(S211A, "s211a", "Snake Eater", "Wig: Interior (SAA Choice)")


namespace MGS3Stages
{
#define X(name, id, mode, disp) constexpr const char* name = id;
    MGS3_STAGE_LIST
#undef X
}
