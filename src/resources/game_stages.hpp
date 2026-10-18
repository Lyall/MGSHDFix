#pragma once



struct Stage
{
    const char* sStageId;
    const char* sGameMode;
    const char* sRichPresenceName;
};

//X(BOSS,      "boss",      "Menu", "Boss Survival (Transition)") \

#define MGS2_STAGE_LIST \
    /* Menus                 */ \
    X(INIT,      "init",      "Menu", "Game Initialization") \
    X(N_TITLE,   "n_title",   "Menu", "Title Screen") \
    X(SELECT,    "select",    "Menu", "Developer Menu") \
    X(MSELECT,   "mselect",   "Menu", "VR Missions Menu") \
    X(SSELECT,   "sselect",   "Menu", "Snake Tales Select / Credits") \
    X(TALES,     "tales",     "Menu", "Snake Tales Menu") \
    \
    /* Tanker (Playable)     */ \
    X(W00A, "w00a", "Tanker", "Aft Deck") \
    X(W00B, "w00b", "Tanker", "Deck A - Port") \
    X(W00C, "w00c", "Tanker", "Navigational Deck, Wing") \
    X(W01A, "w01a", "Tanker", "Deck-A Crew's Quarters") \
    X(W01B, "w01b", "Tanker", "Deck-B Crew's Quarters") \
    X(W01C, "w01c", "Tanker", "Deck-C Crew's Quarters") \
    X(W01D, "w01d", "Tanker", "Deck-D Crew's Quarters") \
    X(W01E, "w01e", "Tanker", "Deck-E, The Bridge") \
    X(W01F, "w01f", "Tanker", "Deck-A Crew's Lounge") \
    X(W02A, "w02a", "Tanker", "Engine Room") \
    X(W03A, "w03a", "Tanker", "Deck-2, Port") \
    X(W03B, "w03b", "Tanker", "Deck-2, Starboard") \
    X(W04A, "w04a", "Tanker", "Hold No.1") \
    X(W04B, "w04b", "Tanker", "Hold No.2") \
    X(W04C, "w04c", "Tanker", "Hold No.3") \
    \
    /* Tanker (Cutscenes)    */ \
    X(D00T,   "d00t",   "Tanker", "George Washington Bridge") /*Tanker Opening*/ \
    X(D01T,   "d01t",   "Tanker", "Navigational Deck, Wing") /*Russian Invasion*/\
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
    X(W11A, "w11a", "Plant", "Strut A Deep Sea Dock") \
    X(W11B, "w11b", "Plant", "Strut A Deep Sea Dock (Bomb Disposal)") \
    X(W11C, "w11c", "Plant", "Strut A Deep Sea Dock (Fortune)") \
    X(W12A, "w12a", "Plant", "Strut A Roof") \
    X(W12B, "w12b", "Plant", "Strut A Pump Room") \
    X(W12C, "w12c", "Plant", "Strut A Roof (Bomb)") \
    X(W13A, "w13a", "Plant", "AB Connecting Bridge") \
    X(W13B, "w13b", "Plant", "AB Connecting Bridge (Sensor B)") \
    X(W14A, "w14a", "Plant", "Strut B Transformer Room") \
    X(W15A, "w15a", "Plant", "BC Connecting Bridge") \
    X(W15B, "w15b", "Plant", "BC Connecting Bridge (After Stillman)") \
    X(W16A, "w16a", "Plant", "Strut C Dining Hall") \
    X(W16B, "w16b", "Plant", "Strut C Dining Hall (Post Stillman)") \
    X(W17A, "w17a", "Plant", "CD Connecting Bridge") \
    X(W18A, "w18a", "Plant", "Strut D Sediment Pool") \
    X(W19A, "w19a", "Plant", "DE Connecting Bridge") \
    X(W20A, "w20a", "Plant", "Strut E Parcel Room") \
    X(W20B, "w20b", "Plant", "Strut E Heliport") \
    X(W20C, "w20c", "Plant", "Strut E Heliport (Bomb)") \
    X(W20D, "w20d", "Plant", "Strut E Heliport (Post Ninja)") \
    X(W21A, "w21a", "Plant", "EF Connecting Bridge") \
    X(W21B, "w21b", "Plant", "EF Connecting Bridge 2") \
    X(W22A, "w22a", "Plant", "Strut F Warehouse") \
    X(W23A, "w23a", "Plant", "FA Connecting Bridge") \
    X(W23B, "w23b", "Plant", "FA Connecting Bridge (Post Shell 2)") \
    X(W24A, "w24a", "Plant", "Shell 1 Core, 1F") \
    X(W24B, "w24b", "Plant", "Shell 1 Core, B1") \
    X(W24C, "w24c", "Plant", "Shell 1 Core, B1 Hall") \
    X(W24D, "w24d", "Plant", "Shell 1 Core, B2 Computer Room") \
    X(W24E, "w24e", "Plant", "Shell 1 Core, B1 Hall") \
    X(W25A, "w25a", "Plant", "Shell 1-2 Connecting Bridge") \
    X(W25B, "w25b", "Plant", "Shell 1-2 Connecting Bridge - LG Connecting Bridge") \
    X(W25C, "w25c", "Plant", "Strut L Perimeter - KL Connecting Bridge") \
    X(W25D, "w25d", "Plant", "KL Connecting Bridge") \
    X(W28A, "w28a", "Plant", "Strut L Sewage Treatment Facility") \
    X(W31A, "w31a", "Plant", "Shell 2 Core, 1F Air Purification Room") \
    X(W31B, "w31b", "Plant", "Shell 2 Core, B1 Filtration Chamber No.1") \
    X(W31C, "w31c", "Plant", "Shell 2 Core, B1 Filtration Chamber No.2") \
    X(W31D, "w31d", "Plant", "Shell 2 Core, 1F Air Purification Room (With Emma)") \
    X(W31F, "w31f", "Plant", "Shell 2 Core, B1 Filtration Chamber No.2") \
    X(W32A, "w32a", "Plant", "Strut L Oil Fence") \
    X(W32B, "w32b", "Plant", "Strut L Oil Fence (Vamp)") \
    X(W41A, "w41a", "Plant", "Arsenal Gear - Stomach") \
    X(W42A, "w42a", "Plant", "Arsenal Gear - Jejunum") \
    X(W43A, "w43a", "Plant", "Arsenal Gear - Ascending Colon") \
    X(W44A, "w44a", "Plant", "Arsenal Gear - Ileum") \
    X(W45A, "w45a", "Plant", "Arsenal Gear - Sigmoid Colon") \
    X(W46A, "w46a", "Plant", "Arsenal Gear - Rectum") \
    X(W51A, "w51a", "Plant", "Arsenal Gear") \
    X(W61A, "w61a", "Plant", "Federal Hall") \
    \
    /* Plant (Cutscenes)     */ \
    X(MUSEUM,   "museum",   "Plant", "Briefing") \
    X(WEBDEMO,  "webdemo",  "Plant", "Web Demo") \
    X(WMOVIE,   "wmovie",   "Plant", "Full-Screen Movie") \
    X(ENDING,   "ending",   "Plant", "Results Screen") \
    X(D001P01,  "d001p01",  "Plant", "Plant Opening") \
    X(D001P02,  "d001p02",  "Plant", "Sea Dock Cutscene") \
    X(D005P01,  "d005p01",  "Plant", "Raiden On Elevator") \
    X(D005P03,  "d005p03",  "Plant", "Strut A Roof Cutscene") \
    X(D010P01,  "d010p01",  "Plant", "Meeting Vamp") \
    X(D012P01,  "d012p01",  "Plant", "Meeting Fortune") \
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
    X(A00A, "a00a", "Alternate", "Aft Deck") \
    X(A00B, "a00b", "Alternate", "Deck A - Port") \
    X(A00C, "a00c", "Alternate", "Navigational Deck, Wing (Unused Alt)") \
    X(A01A, "a01a", "Alternate", "Deck-A Crew's Quarters") \
    X(A01B, "a01b", "Alternate", "Deck-B Crew's Quarters") \
    X(A01C, "a01c", "Alternate", "Deck-C Crew's Quarters") \
    X(A01D, "a01d", "Alternate", "Deck-D Crew's Quarters") \
    X(A01E, "a01e", "Alternate", "Deck-E, The Bridge") \
    X(A01F, "a01f", "Alternate", "Deck-A Crew's Lounge") \
    X(A02A, "a02a", "Alternate", "Engine Room") \
    X(A02B, "a02b", "Alternate", "Engine Room") \
    X(A03A, "a03a", "Alternate", "Deck-2, Port") \
    X(A03B, "a03b", "Alternate", "Deck-2, Starboard") \
    X(A04A, "a04a", "Alternate", "Hold No.1") \
    X(A04B, "a04b", "Alternate", "Hold No.2") \
    X(A04C, "a04c", "Alternate", "Hold No.3") \
    /* Alternate Plant (Bomb Disposal / Eliminate / Hold-Up / Photograph) */ \
    X(A20C, "a20c", "Alternate", "Strut E Heliport (Bomb)") \
    X(A24F, "a24f", "Alternate", "Shell 1 Core, B1 Hall") \
    X(A24G, "a24g", "Alternate", "Shell 1 Core, B2 Computer Room") \
    X(A31B, "a31b", "Alternate", "Shell 2 Core, B1 Filtration Chamber No.1") \
    X(A41A, "a41a", "Alternate", "Arsenal Gear - Stomach") \
    X(A41B, "a41b", "Alternate", "Arsenal Gear - Stomach") \
    X(A42A, "a42a", "Alternate", "Arsenal Gear - Jejunum") \
    X(A43A, "a43a", "Alternate", "Arsenal Gear - Ascending Colon") \
    X(A45A, "a45a", "Alternate", "Arsenal Gear - Sigmoid Colon") \
    \
    /* Snake Tales (shared) */ \
    X(A12B, "a12b", "Snake Tales", "Strut A Pump Room (A Wrongdoing / Dead Man Whispers)") \
    X(A13A, "a13a", "Snake Tales", "AB Connecting Bridge (Dead Man Whispers / External Gazer)") \
    X(A13B, "a13b", "Snake Tales", "AB Connecting Bridge (Sensor B) (Dead Man Whispers / External Gazer)") \
    X(A14A, "a14a", "Snake Tales", "Strut B Transformer Room (Big Shell Evil / Dead Man Whispers / External Gazer)") \
    X(A15A, "a15a", "Snake Tales", "BC Connecting Bridge (Big Shell Evil / Dead Man Whispers / External Gazer)") \
    X(A15B, "a15b", "Snake Tales", "BC Connecting Bridge (After Stillman) (Big Shell Evil / Dead Man Whispers / External Gazer)") \
    X(A16A, "a16a", "Snake Tales", "Strut C Dining Hall (Big Shell Evil / Dead Man Whispers / External Gazer)") \
    X(A17A, "a17a", "Snake Tales", "CD Connecting Bridge (Big Shell Evil / Dead Man Whispers / External Gazer)") \
    X(A18A, "a18a", "Snake Tales", "Strut D Sediment Pool (Big Shell Evil / Dead Man Whispers)") \
    X(A19A, "a19a", "Snake Tales", "DE Connecting Bridge (Big Shell Evil / Dead Man Whispers)") \
    X(A20A, "a20a", "Snake Tales", "Strut E Parcel Room (A Wrongdoing / Big Shell Evil / Dead Man Whispers)") \
    X(A21A, "a21a", "Snake Tales", "EF Connecting Bridge (A Wrongdoing / Big Shell Evil)") \
    X(A21B, "a21b", "Snake Tales", "EF Connecting Bridge 2 (A Wrongdoing / Big Shell Evil)") \
    X(A22A, "a22a", "Snake Tales", "Strut F Warehouse (A Wrongdoing / Big Shell Evil / Dead Man Whispers)") \
    X(A23A, "a23a", "Snake Tales", "FA Connecting Bridge (A Wrongdoing / Dead Man Whispers)") \
    X(A23B, "a23b", "Snake Tales", "FA Connecting Bridge (A Wrongdoing / Dead Man Whispers)") \
    \
    /* Snake Tales (A Wrongdoing) */ \
    X(A12A, "a12a", "Snake Tales", "Strut A Roof (A Wrongdoing)") \
    X(A24A, "a24a", "Snake Tales", "Shell 1 Core, 1F (A Wrongdoing)") \
    X(A24B, "a24b", "Snake Tales", "Shell 1 Core, B1 (A Wrongdoing)") \
    X(A24C, "a24c", "Snake Tales", "Shell 1 Core, B1 Hall (A Wrongdoing)") \
    X(A24D, "a24d", "Snake Tales", "Shell 1 Core, B2 Computer Room (A Wrongdoing)") \
    \
    /* Snake Tales (Big Shell Evil) */ \
    X(A20B, "a20b", "Snake Tales", "Strut E Heliport (Big Shell Evil)") \
    X(A20E, "a20e", "Snake Tales", "Strut E Heliport (Big Shell Evil)") \
    X(A22B, "a22b", "Snake Tales", "Strut F Warehouse (Big Shell Evil)") \
    X(A25A, "a25a", "Snake Tales", "Shell 1-2 Connecting Bridge (Big Shell Evil)") \
    \
    /* Snake Tales (Dead Man Whispers) */ \
    X(A14B, "a14b", "Snake Tales", "Strut B Transformer Room (Dead Man Whispers)") \
    X(A25D, "a25d", "Snake Tales", "KL Connecting Bridge (Dead Man Whispers)") \
    X(A28A, "a28a", "Snake Tales", "Strut L Sewage Treatment Facility (Dead Man Whispers)") \
    X(A31A, "a31a", "Snake Tales", "Shell 2 Core, 1F Air Purification Room (Dead Man Whispers)") \
    X(A31C, "a31c", "Snake Tales", "Shell 2 Core, B1 Filtration Chamber No.2 (Dead Man Whispers)") \
    \
    /* Snake Tales (External Gazer) */ \
    X(A13C, "a13c", "Snake Tales", "AB Connecting Bridge (External Gazer)") \
    X(A46A, "a46a", "Snake Tales", "Arsenal Gear - Rectum (External Gazer)") \
    X(A61A, "a61a", "Snake Tales", "Federal Hall (External Gazer)") \
    X(TA00A,  "ta00a",  "Snake Tales", "Aft Deck (External Gazer)") \
    X(TA01A,  "ta01a",  "Snake Tales", "Deck-A Crew's Quarters (External Gazer)") \
    X(TA01B,  "ta01b",  "Snake Tales", "Deck-B Crew's Quarters (External Gazer)") \
    X(TA01C,  "ta01c",  "Snake Tales", "Deck-C Crew's Quarters (External Gazer)") \
    X(TA01D,  "ta01d",  "Snake Tales", "Deck-D Crew's Quarters (External Gazer)") \
    X(TA01E,  "ta01e",  "Snake Tales", "Deck-E, The Bridge (External Gazer)") \
    X(TA01F,  "ta01f",  "Snake Tales", "Deck-A Crew's Lounge (External Gazer)") \
    X(TA02A,  "ta02a",  "Snake Tales", "Engine Room (External Gazer)") \
    X(TA12A,  "ta12a",  "Snake Tales", "Strut A Roof (External Gazer)") \
    X(TA20A,  "ta20a",  "Snake Tales", "Strut E Heliport (External Gazer)") \
    X(TA22A,  "ta22a",  "Snake Tales", "Philanthropy's Hideout (External Gazer)") \
    X(TA24A,  "ta24a",  "Snake Tales", "Shell 1 Core, 1F (External Gazer)") \
    X(TA31A,  "ta31a",  "Snake Tales", "Shell 2 Core, 1F Air Purification Room (External Gazer)") \
    X(TA42A,  "ta42a",  "Snake Tales", "Arsenal Gear - Jejunum (External Gazer)") \
    X(TSP03A, "tsp03a", "Snake Tales", "VR: Variety - Level 3 (External Gazer)") \
    X(TVS03A, "tvs03a", "Snake Tales", "VR: Sneaking - Level 3 (External Gazer)") \
    X(TVS05A, "tvs05a", "Snake Tales", "VR: Sneaking - Level 5 (Eliminate All) (External Gazer)") \
    X(TVS06A, "tvs06a", "Snake Tales", "VR: Sneaking - Level 6 (External Gazer)") \
    X(TVS08A, "tvs08a", "Snake Tales", "VR: Sneaking - Level 8 (External Gazer)") \
    X(TWP03A, "twp03a", "Snake Tales", "VR: Weapons - SOCOM - Level 3 (External Gazer)") \
    X(TWP34A, "twp34a", "Snake Tales", "VR: Weapons - Claymore - Level 4 (External Gazer)") \
    X(TWP43A, "twp43a", "Snake Tales", "VR: Weapons - PSG1 - Level 3 (External Gazer)") \
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
    X(SP01A, "sp01a", "VR: Variety", "Athletic Course (Stage 1 & 2)") \
    X(SP02A, "sp02a", "VR: Variety", "Athletic Course (Stage 3)") \
    X(SP03A, "sp03a", "VR: Variety", "Mob Survival (Stage 7)") \
    X(SP06A, "sp06a", "VR: Variety", "Sniping Stage (Stage 4)") \
    X(SP07A, "sp07a", "VR: Variety", "Escape Stage (Stage 6)") \
    X(SP08A, "sp08a", "VR: Variety", "Dark Stage (Stage 5)") \
    \
    /* VR: First-Person      */ \
    X(SP21A, "sp21a", "VR: First-Person", "Level 1") \
    X(SP22A, "sp22a", "VR: First-Person", "Level 2 & 3") \
    X(SP24A, "sp24a", "VR: First-Person", "Level 4 (Dark Stage)") \
    X(SP25A, "sp25a", "VR: First-Person", "Level 5 (Bomb Disposal)") \
    \
    /* VR: Streaking         */ \
    X(ST01A, "st01a", "VR: Streaking", "Level 1") \
    X(ST02A, "st02a", "VR: Streaking", "Level 2") \
    X(ST03A, "st03a", "VR: Streaking", "Level 3") \
    X(ST04A, "st04a", "VR: Streaking", "Level 4") \
    X(ST05A, "st05a", "VR: Streaking", "Level 5") \
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
    X(TITLE,   "title",   "Menu", "Title Screen / Main Menu") \
    X(THEATER, "theater", "Menu", "Demo Theater") \
    X(ENDING,  "ending",  "Menu", "Credits") \
    X(SELECT,  "select",  "Menu", "Developer Menu") \
    X(NET,     "net",     "Menu", "Network Mode") \
    \
    \
    /* Virtuous Mission (VM) and early Snake Eater mirrors */ \
    X(V000A_0, "v000a_0", "Virtuous Mission", "Virtuous Mission Intro") \
    X(V000A_1, "v000a_1", "Virtuous Mission", "Virtuous Mission Intro") \
    X(V001A, "v001a", "Virtuous Mission", "Dremuchji South") \
    X(V001A_0, "v001a_0", "Virtuous Mission", "Dremuchji South") \
    X(V003A, "v003a", "Virtuous Mission", "Dremuchji Swampland") \
    X(V004A, "v004a", "Virtuous Mission", "Dremuchji North") \
    X(V004A_0, "v004a_0", "Virtuous Mission", "Dremuchji North") \
    X(V005A, "v005a", "Virtuous Mission", "Dolinovodno Rope Bridge") \
    X(V005A_0, "v005a_0", "Virtuous Mission", "Dolinovodno Rope Bridge") \
    X(V005A_1, "v005a_1", "Virtuous Mission", "Dolinovodno Rope Bridge") \
    X(V005A_2, "v005a_2", "Virtuous Mission", "Dolinovodno Rope Bridge") \
    X(V006A, "v006a", "Virtuous Mission", "Rassvet") \
    X(V006A_0, "v006a_0", "Virtuous Mission", "Rassvet") \
    X(V006B, "v006b", "Virtuous Mission", "Rassvet") \
    X(V007A, "v007a", "Virtuous Mission", "Dolinovodno Riverbank") \
    X(V007A_0, "v007a_0", "Virtuous Mission", "Dolinovodno Riverbank") \
    X(V008A_0, "v008a_0", "Virtuous Mission", "Virtuous Mission Ending") \
    \
    X(S000A_0, "s000a_0", "Snake Eater", "Snake Eater Intro") \
    X(S000A_1, "s000a_1", "Snake Eater", "Snake Eater Intro") \
    X(S001A, "s001a", "Snake Eater", "Dremuchji South") \
    X(S002A, "s002a", "Snake Eater", "Dremuchji East") \
    X(S003A, "s003a", "Snake Eater", "Dremuchji Swampland") \
    X(S004A, "s004a", "Snake Eater", "Dremuchji North") \
    X(S004A_0, "s004a_0", "Snake Eater", "Dremuchji North") \
    X(S005A, "s005a", "Snake Eater", "Dolinovodno Rope Bridge") \
    X(S006A, "s006a", "Snake Eater", "Rassvet") \
    X(S006A_0, "s006a_0", "Snake Eater", "Rassvet") \
    X(S006B, "s006b", "Snake Eater", "Rassvet") \
    X(S012A, "s012a", "Snake Eater", "Chyornyj Prud (Swamp)") \
    X(S021A, "s021a", "Snake Eater", "Bolshaya Past South") \
    X(S022A, "s022a", "Snake Eater", "Bolshaya Past North") \
    X(S023A, "s023a", "Snake Eater", "Bolshaya Past Crevice (Ocelot Battle)") \
    X(S023A_0, "s023a_0", "Snake Eater", "Bolshaya Past Crevice (Ocelot Battle)") \
    \
    /* Chyornaya Peschera (The Pain) */ \
    X(S031A, "s031a", "Snake Eater", "Chyornaya Peschera Cave Branch") \
    X(S032A, "s032a", "Snake Eater", "Chyornaya Peschera Cave") \
    X(S032B, "s032b", "Snake Eater", "Chyornaya Peschera Cave (The Pain Boss)") \
    X(S032B_0, "s032b_0", "Snake Eater", "Chyornaya Peschera Cave (The Pain Boss)") \
    X(S033A, "s033a", "Snake Eater", "Chyornaya Peschera Cave Entrance") \
    \
    /* Ponizovje (water / armory / warehouse) */ \
    X(S041A, "s041a", "Snake Eater", "Ponizovje South (Watercraft)") \
    X(S042A, "s042a", "Snake Eater", "Ponizovje Armory (SVD)") \
    X(S043A, "s043a", "Snake Eater", "Ponizovje Warehouse (Exterior)")/*(Kill The End Early)*/\
    X(S043A_0, "s043a_0", "Snake Eater", "Ponizovje Warehouse (Exterior)") \
    X(S044A, "s044a", "Snake Eater", "Ponizovje Warehouse (Interior)") \
    X(S044A_0, "s044a_0", "Snake Eater", "Ponizovje Warehouse (Interior)") \
    \
    /* Graniny Gorki (The Fear; labs) */ \
    X(KYLE_OP, "kyle_op", "Snake Eater", "Opening Snake Eater Video") \
    X(S051A, "s051a", "Snake Eater", "Graniny Gorki South (The Fear)") \
    X(S051B, "s051b", "Snake Eater", "Graniny Gorki South (The Fear)") \
    X(S051B_0, "s051b_0", "Snake Eater", "Graniny Gorki South (The Fear)") \
    X(S052A, "s052a", "Snake Eater", "Graniny Gorki Lab Exterior: Outside Walls") \
    X(S052B, "s052b", "Snake Eater", "Graniny Gorki Lab Exterior: Inside Walls") \
    X(S053A, "s053a", "Snake Eater", "Graniny Gorki Lab 1F/2F") \
    X(S054A_0, "s054a_0", "Snake Eater", "Graniny Gorki Lab Interior") \
    X(S055A, "s055a", "Snake Eater", "Graniny Gorki Lab B1 (Prison Cells)") \
    X(S056A, "s056a", "Snake Eater", "Graniny Gorki Lab B1 (Granin Basement)") \
    \
    /* Svyatogornyj / Sokrovenno (The End / Ocelot Unit) */ \
    X(S045A, "s045a", "Snake Eater", "Svyatogornyj South") \
    X(S061A, "s061a", "Snake Eater", "Svyatogornyj West") \
    X(S062A, "s062a", "Snake Eater", "Svyatogornyj East (M63 House)") \
    X(S063A, "s063a", "Snake Eater", "Sokrovenno South (The End)") \
    X(S063A_0, "s063a_0", "Snake Eater", "Sokrovenno South (The End)") \
    X(S063B, "s063b", "Snake Eater", "Sokrovenno South (Ocelot Unit)") \
    X(S064A, "s064a", "Snake Eater", "Sokrovenno West (The End, River)") \
    X(S064A_0, "s064a_0", "Snake Eater", "Sokrovenno West (The End, River)") \
    X(S064B, "s064b", "Snake Eater", "Sokrovenno West (Ocelot Unit, River)") \
    X(S065A, "s065a", "Snake Eater", "Sokrovenno North (The End's Death)") \
    X(S065A_0, "s065a_0", "Snake Eater", "Sokrovenno North (The End's Death)") \
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
    X(S074A_0, "s074a_0", "Snake Eater", "Krasnogorje Mountaintop Ruins (Eva Cutscene)") \
    X(S075A, "s075a", "Snake Eater", "Krasnogorje Mountaintop: Behind Ruins") \
    X(S075A_0, "s075a_0", "Snake Eater", "Krasnogorje Mountaintop: Behind Ruins") \
    \
    /* Groznyj Grad - all names explicitly prefixed */ \
    X(S081A, "s081a", "Snake Eater", "Groznyj Grad Underground Tunnel (The Fury)") \
    X(S081A_0, "s081a_0", "Snake Eater", "Groznyj Grad Underground Tunnel (The Fury)") \
    X(S091A, "s091a", "Snake Eater", "Groznyj Grad Southwest") \
    X(S091A_0, "s091a_0", "Snake Eater", "Groznyj Grad Southwest") \
    X(S091B, "s091b", "Snake Eater", "Groznyj Grad Southwest (During Escape)") \
    X(S091C, "s091c", "Snake Eater", "Groznyj Grad Southwest (Return)") \
    X(S092A, "s092a", "Snake Eater", "Groznyj Grad Northwest") \
    X(S092A_0, "s092a_0", "Snake Eater", "Groznyj Grad Northwest") \
    X(S092B, "s092b", "Snake Eater", "Groznyj Grad Northwest (During Escape)") \
    X(S092C, "s092c", "Snake Eater", "Groznyj Grad Northwest (Return)") \
    X(S093A, "s093a", "Snake Eater", "Groznyj Grad Northeast") \
    X(S093A_0, "s093a_0", "Snake Eater", "Groznyj Grad Northeast") \
    X(S093B, "s093b", "Snake Eater", "Groznyj Grad Northeast (During Escape)") \
    X(S093C, "s093c", "Snake Eater", "Groznyj Grad Northeast (Return)") \
    X(S094A, "s094a", "Snake Eater", "Groznyj Grad Southeast") \
    X(S094B, "s094b", "Snake Eater", "Groznyj Grad Southeast (During Escape)") \
    X(S094C, "s094c", "Snake Eater", "Groznyj Grad Southeast (Return)") \
    X(S101A, "s101a", "Snake Eater", "Groznyj Grad Weapons Lab: East Wing") \
    X(S101A_0, "s101a_0", "Snake Eater", "Groznyj Grad Weapons Lab: East Wing") \
    X(S101B, "s101b", "Snake Eater", "Groznyj Grad Weapons Lab: East Wing (C3 Mission)") \
    X(S111A, "s111a", "Snake Eater", "Groznyj Grad Weapons Lab: West Wing Corridor") \
    X(S111A_0, "s111a_0", "Snake Eater", "Groznyj Grad Weapons Lab: West Wing Corridor") \
    X(S112A, "s112a", "Snake Eater", "Groznyj Grad Torture Room") \
    X(S112A_0, "s112a_0", "Snake Eater", "Groznyj Grad Torture Room") \
    X(S113A, "s113a", "Snake Eater", "Groznyj Grad Sewers") \
    X(S121A, "s121a", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing") \
    X(S121A_0, "s121a_0", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing") \
    X(S121A_1, "s121a_1", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing") \
    X(S122A, "s122a", "Snake Eater", "Groznyj Grad Weapons Lab: Main Wing B1 (Volgin)") \
    X(S131A_0, "s131a_0", "Snake Eater", "Groznyj Grad (Post-Volgin)") \
    \
    /* Sorrow / Tikhogornyj */ \
    X(S141A, "s141a", "Snake Eater", "The Sorrow (Boss Battle)") \
    X(S141A_0, "s141a_0", "Snake Eater", "The Sorrow (Boss Battle)") \
    X(S151A, "s151a", "Snake Eater", "Tikhogornyj") \
    X(S151A_0, "s151a_0", "Snake Eater", "Tikhogornyj") \
    X(S152A, "s152a", "Snake Eater", "Tikhogornyj: Behind Waterfall") \
    X(S152A_0, "s152a_0", "Snake Eater", "Tikhogornyj: Behind Waterfall") \
    \
    /* Bike chase / Shagohod / Bridge */ \
    X(S161A, "s161a", "Snake Eater", "Groznyj Grad (Bike Chase 1)") \
    X(S161A_0, "s161a_0", "Snake Eater", "Groznyj Grad (Bike Chase 1)") \
    X(S161A_1, "s161a_1", "Snake Eater", "Groznyj Grad (Bike Chase 1)") \
    X(S161A_2, "s161a_2", "Snake Eater", "Groznyj Grad (Bike Chase 1)") \
    X(S162A, "s162a", "Snake Eater", "Groznyj Grad Runway South (Bike Chase 2)") \
    X(S162A_0, "s162a_0", "Snake Eater", "Groznyj Grad Runway South (Bike Chase 2)") \
    X(S163A, "s163a", "Snake Eater", "Groznyj Grad Runway (Bike Chase 3)") \
    X(S163A_0, "s163a_0", "Snake Eater", "Groznyj Grad Runway (Bike Chase 3)") \
    X(S163B, "s163b", "Snake Eater", "Groznyj Grad Runway (Shagohod Fight)") \
    X(S164A_0, "s164a_0", "Snake Eater", "Groznyj Grad Runway (Shagohod Fight)") \
    X(S171A, "s171a", "Snake Eater", "Groznyj Grad Rail Bridge (C3)") \
    X(S171A_0, "s171a_0", "Snake Eater", "Groznyj Grad Rail Bridge (C3)") \
    X(S171B, "s171b", "Snake Eater", "Groznyj Grad Rail Bridge (Shagohod Battle)") \
    X(S171B_0, "s171b_0", "Snake Eater", "Groznyj Grad Rail Bridge (Shagohod Battle)") \
    X(S181A, "s181a", "Snake Eater", "Groznyj Grad Rail Bridge North (Escape)") \
    X(S182A, "s182a", "Snake Eater", "Lazorevo South (Bike Chase)") \
    X(S183A, "s183a", "Snake Eater", "Lazorevo North (Final Bike Chase)") \
    \
    /* Finale */ \
    X(S191A, "s191a", "Snake Eater", "Zaozyorje West") \
    X(S191A_0, "s191a_0", "Snake Eater", "Zaozyorje West") \
    X(S191B, "s191b", "Snake Eater", "Zaozyorje West") \
    X(S192A, "s192a", "Snake Eater", "Zaozyorje East") \
    X(S192A_0, "s192a_0", "Snake Eater", "Zaozyorje East") \
    X(S201A, "s201a", "Snake Eater", "Rokovj Bereg (The Boss)") \
    X(S201A_0, "s201a_0", "Snake Eater", "Rokovj Bereg (The Boss)") \
    X(S201A_1, "s201a_1", "Snake Eater", "Rokovj Bereg (The Boss)") \
    X(S211A, "s211a", "Snake Eater", "Wig: Interior")\
    X(S211A_0, "s211a_0", "Snake Eater", "Wig: Interior")\
    X(S211A_1, "s211a_1", "Snake Eater", "Wig: Interior")\
    X(S211A_3, "s211a_3", "Snake Eater", "Wig: Interior")\
    X(S221A_0, "s221a_0", "Snake Eater", "Great Falls, VA")\
    X(S223A_0, "s223a_0", "Snake Eater", "Washington, D.C. / Arlington, VA") \
    X(T001A_0, "t001a_0", "Snake Eater", "? (JP STAGE)")


namespace MGS3Stages
{
#define X(name, id, mode, disp) constexpr const char* name = id;
    MGS3_STAGE_LIST
#undef X
}

#define MG1_STAGE_LIST \
    X(INIT,    "init",    "Init", "Init") \
    X(MG1,     "mg1",     "Metal Gear (MSX)", "Metal Gear") \
    X(MG2,     "mg2",     "Metal Gear 2: Solid Snake", "Metal Gear 2: Solid Snake") \
    X(R_SNA01, "r_sna01", "Snake", "Snake")

namespace MG1Stages
{
#define X(name, id, mode, disp) constexpr const char* name = id;
    MG1_STAGE_LIST
#undef X
}

struct MGRoomName
{
    int nArea;
    int nRoom;
    int nSortOrder; // Lower values sort first within the area.
    const char* pszLocation;
    const char* pszName;
};

inline constexpr MGRoomName astMG1RoomNames[] =
{
    { 0, 0, 0, "Building 1 Exterior", "Infiltration Point" },
    { 1, 0, 0, "Building 1 (Floor I)", "Southwest" },
    { 1, 1, 10, "Building 1 (Floor I)", "Southwest" },
    { 1, 2, 20, "Building 1 (Floor I)", "West" },
    { 1, 3, 30, "Building 1 (Floor I)", "West" },
    { 1, 4, 40, "Building 1 (Floor I)", "West" },
    { 1, 5, 50, "Building 1 (Floor I)", "Northwest" },
    { 1, 6, 60, "Building 1 (Floor I)", "South" },
    { 1, 7, 70, "Building 1 (Floor I)", "Center" },
    { 1, 8, 80, "Building 1 (Floor I)", "Center" },
    { 1, 9, 90, "Building 1 (Floor I)", "Center" },
    { 1, 10, 100, "Building 1 (Floor I)", "North" },
    { 1, 11, 110, "Building 1 (Floor I)", "South" },
    { 1, 12, 120, "Building 1 (Floor I)", "Center" },
    { 1, 13, 130, "Building 1 (Floor I)", "Courtyard" },
    { 1, 14, 140, "Building 1 (Floor I)", "North" },
    { 1, 15, 150, "Building 1 (Floor I)", "Southeast" },
    { 1, 16, 160, "Building 1 (Floor I)", "East" },
    { 1, 17, 170, "Building 1 (Floor I)", "East" },
    { 1, 18, 180, "Building 1 (Floor I)", "East" },
    { 1, 19, 190, "Building 1 (Floor I)", "Northeast" },
    { 1, 20, 200, "Building 1 (Floor I)", "Northeast" },
    { 1, 21, 210, "Building 1 (Floor I)", "Northwest" },
    { 1, 26, 260, "Building 1 (Floor I)", "Ration Truck" },
    { 1, 27, 270, "Building 1 (Floor I)", "Level 1 Keycard Truck" },
    { 1, 28, 280, "Building 1 (Floor I)", "Binoculars Truck" },
    { 1, 29, 290, "Building 1 (Floor I)", "Crate Prison Cell" },
    { 1, 30, 300, "Building 1 (Floor I)", "Handgun Truck" },
    { 1, 31, 310, "Building 1 (Floor I)", "Truck Interior" },
    { 1, 32, 320, "Building 1 (Floor I)", "Mine Truck" },
    { 1, 33, 330, "Building 1 (Floor I)", "Gas Mask Room" },
    { 1, 34, 340, "Building 1 (Floor I)", "Level 4 Keycard Truck" },
    { 1, 35, 350, "Building 1 (Floor I)", "Truck Interior" },
    { 1, 36, 360, "Building 1 (Floor I)", "Empty Crate Cell" },
    { 1, 37, 370, "Building 1 (Floor I)", "Prisoner Cell" },
    { 1, 38, 380, "Building 1 (Floor I)", "Submachine Gun Room" },
    { 1, 39, 390, "Building 1 (Floor I)", "Prisoner Cell" },
    { 2, 0, 0, "Building 1 (Floor II)", "Southwest" },
    { 2, 1, 10, "Building 1 (Floor II)", "West" },
    { 2, 2, 20, "Building 1 (Floor II)", "West" },
    { 2, 3, 30, "Building 1 (Floor II)", "West" },
    { 2, 4, 40, "Building 1 (Floor II)", "West" },
    { 2, 5, 50, "Building 1 (Floor II)", "Northwest" },
    { 2, 6, 60, "Building 1 (Floor II)", "Northwest" },
    { 2, 7, 70, "Building 1 (Floor II)", "South" },
    { 2, 8, 80, "Building 1 (Floor II)", "North" },
    { 2, 9, 90, "Building 1 (Floor II)", "South" },
    { 2, 10, 100, "Building 1 (Floor II)", "South" },
    { 2, 11, 110, "Building 1 (Floor II)", "North" },
    { 2, 12, 120, "Building 1 (Floor II)", "Southeast" },
    { 2, 13, 130, "Building 1 (Floor II)", "East" },
    { 2, 14, 140, "Building 1 (Floor II)", "East" },
    { 2, 15, 150, "Building 1 (Floor II)", "Northeast" },
    { 2, 16, 160, "Building 1 (Floor II)", "Southwest" },
    { 2, 26, 260, "Building 1 (Floor II)", "Infrared Goggles Room" },
    { 2, 27, 270, "Building 1 (Floor II)", "Mine Room" },
    { 2, 28, 280, "Building 1 (Floor II)", "Crate Prison Cell" },
    { 2, 29, 290, "Building 1 (Floor II)", "Rolling Trap Room" },
    { 2, 30, 300, "Building 1 (Floor II)", "Parachute Room" },
    { 2, 31, 310, "Building 1 (Floor II)", "Ammo and Explosives Room" },
    { 2, 32, 320, "Building 1 (Floor II)", "Prisoner Cell" },
    { 2, 33, 330, "Building 1 (Floor II)", "Prisoner Cell" },
    { 3, 0, 0, "Building 1 (Floor III)", "Southwest" },
    { 3, 1, 10, "Building 1 (Floor III)", "West" },
    { 3, 2, 20, "Building 1 (Floor III)", "West" },
    { 3, 3, 30, "Building 1 (Floor III)", "Northwest" },
    { 3, 4, 40, "Building 1 (Floor III)", "South" },
    { 3, 5, 50, "Building 1 (Floor III)", "North" },
    { 3, 6, 60, "Building 1 (Floor III)", "South" },
    { 3, 7, 70, "Building 1 (Floor III)", "North" },
    { 3, 8, 80, "Building 1 (Floor III)", "North" },
    { 3, 9, 90, "Building 1 (Floor III)", "North" },
    { 3, 10, 100, "Building 1 (Floor III)", "Southeast" },
    { 3, 11, 110, "Building 1 (Floor III)", "East" },
    { 3, 12, 120, "Building 1 (Floor III)", "East" },
    { 3, 13, 130, "Building 1 (Floor III)", "Northeast" },
    { 3, 14, 140, "Building 1 (Floor III)", "Northeast" },
    { 3, 26, 260, "Building 1 (Floor III)", "Prisoner Cell" },
    { 3, 27, 270, "Building 1 (Floor III)", "RC Missile Room" },
    { 3, 28, 280, "Building 1 (Floor III)", "Prison Cell Ladder" },
    { 3, 29, 290, "Building 1 (Floor III)", "Ammo Room" },
    { 3, 30, 300, "Building 1 (Floor III)", "Prisoner Cell" },
    { 3, 31, 310, "Building 1 (Floor III)", "Explosives and Rolling Trap" },
    { 3, 32, 320, "Building 1 (Floor III)", "Crate Room" },
    { 3, 33, 330, "Building 1 (Floor III)", "Grenade Launcher Room" },
    { 3, 34, 340, "Building 1 (Floor III)", "Cardboard Box Room" },
    { 3, 35, 350, "Building 1 (Floor III)", "Level 2 Keycard Room" },
    { 3, 36, 360, "Building 1 (Floor III)", "Ration Room" },
    { 3, 37, 370, "Building 1 (Floor III)", "Prisoner Cell" },
    { 4, 0, 0, "Building 1 (Floor B-I)", "Southwest" },
    { 4, 1, 10, "Building 1 (Floor B-I)", "West" },
    { 4, 2, 20, "Building 1 (Floor B-I)", "Underground Passage" },
    { 4, 3, 30, "Building 1 (Floor B-I)", "South" },
    { 4, 4, 40, "Building 1 (Floor B-I)", "South" },
    { 4, 5, 50, "Building 1 (Floor B-I)", "Center" },
    { 4, 6, 60, "Building 1 (Floor B-I)", "North" },
    { 4, 7, 70, "Building 1 (Floor B-I)", "Southeast" },
    { 4, 8, 80, "Building 1 (Floor B-I)", "East" },
    { 4, 9, 90, "Building 1 (Floor B-I)", "Northeast" },
    { 4, 26, 260, "Building 1 (Floor B-I)", "Breakable Wall Cell" },
    { 4, 27, 270, "Building 1 (Floor B-I)", "Underground Cell" },
    { 4, 28, 280, "Building 1 (Floor B-I)", "Breakable Wall Pitfall" },
    { 4, 29, 290, "Building 1 (Floor B-I)", "Prisoner Cell" },
    { 4, 30, 300, "Building 1 (Floor B-I)", "Ammo and Level 3 Keycard" },
    { 4, 31, 310, "Building 1 (Floor B-I)", "Confiscated Equipment" },
    { 4, 32, 320, "Building 1 (Floor B-I)", "Body Armor Room" },
    { 4, 33, 330, "Building 1 (Floor B-I)", "Enemy Uniform Room" },
    { 4, 34, 340, "Building 1 (Floor B-I)", "Ammo and Explosives Room" },
    { 4, 35, 350, "Building 1 (Floor B-I)", "Bomb Blast Suit Room" },
    { 5, 0, 0, "Desert (Part I)", "South Entrance" },
    { 5, 1, 10, "Desert (Part I)", "South" },
    { 5, 2, 20, "Desert (Part I)", "Center" },
    { 5, 3, 30, "Desert (Part I)", "Center" },
    { 5, 4, 40, "Desert (Part I)", "North" },
    { 5, 5, 50, "Desert (Part I)", "North" },
    { 5, 6, 60, "Desert (Part I)", "Center" },
    { 5, 26, 260, "Desert (Part I)", "Truck Interior" },
    { 5, 27, 270, "Desert (Part I)", "Explosives Truck" },
    { 5, 28, 280, "Desert (Part I)", "Ration Truck" },
    { 6, 0, 0, "Building 1 Elevators", "Elevator Shaft" },
    { 6, 1, 10, "Building 1 Elevators", "Basement Landing" },
    { 6, 2, 20, "Building 1 Elevators", "Roof Landing" },
    { 7, 0, 0, "Building 1 (Floor IV)", "Southwest" },
    { 7, 1, 10, "Building 1 (Floor IV)", "West" },
    { 7, 2, 20, "Building 1 (Floor IV)", "West" },
    { 7, 3, 30, "Building 1 (Floor IV)", "Northwest" },
    { 7, 4, 40, "Building 1 (Floor IV)", "South" },
    { 7, 5, 50, "Building 1 (Floor IV)", "Center" },
    { 7, 6, 60, "Building 1 (Floor IV)", "Center" },
    { 7, 7, 70, "Building 1 (Floor IV)", "North" },
    { 7, 8, 80, "Building 1 (Floor IV)", "South" },
    { 7, 9, 90, "Building 1 (Floor IV)", "North" },
    { 7, 10, 100, "Building 1 (Floor IV)", "Southeast" },
    { 7, 11, 110, "Building 1 (Floor IV)", "East" },
    { 7, 12, 120, "Building 1 (Floor IV)", "East" },
    { 7, 13, 130, "Building 1 (Floor IV)", "Northeast" },
    { 7, 14, 140, "Building 1 (Floor IV)", "Center" },
    { 7, 15, 150, "Building 1 (Floor IV)", "Southwest" },
    { 7, 26, 260, "Building 1 (Floor IV)", "Mine Detector Room" },
    { 7, 27, 270, "Building 1 (Floor IV)", "Prisoner Cell" },
    { 7, 28, 280, "Building 1 (Floor IV)", "Ammo Room" },
    { 7, 29, 290, "Building 1 (Floor IV)", "RC Missile Room" },
    { 8, 0, 0, "Building 2 (Floor I)", "Southwest" },
    { 8, 1, 10, "Building 2 (Floor I)", "Southwest" },
    { 8, 2, 20, "Building 2 (Floor I)", "West" },
    { 8, 3, 30, "Building 2 (Floor I)", "Entrance" },
    { 8, 4, 40, "Building 2 (Floor I)", "South" },
    { 8, 5, 50, "Building 2 (Floor I)", "Center" },
    { 8, 6, 60, "Building 2 (Floor I)", "Southeast" },
    { 8, 7, 70, "Building 2 (Floor I)", "Southeast" },
    { 8, 8, 80, "Building 2 (Floor I)", "East" },
    { 8, 9, 90, "Building 2 (Floor I)", "Waterway" },
    { 8, 10, 100, "Building 2 (Floor I)", "East" },
    { 8, 11, 110, "Building 2 (Floor I)", "Northeast" },
    { 8, 12, 120, "Building 2 (Floor I)", "Northeast" },
    { 8, 26, 260, "Building 2 (Floor I)", "Flashlight Room" },
    { 8, 27, 270, "Building 2 (Floor I)", "Ammo and Mine Room" },
    { 8, 28, 280, "Building 2 (Floor I)", "Antenna Room" },
    { 8, 29, 290, "Building 2 (Floor I)", "Explosives Room" },
    { 8, 30, 300, "Building 2 (Floor I)", "Crate Passage" },
    { 8, 31, 310, "Building 2 (Floor I)", "Prisoner Cell" },
    { 8, 32, 320, "Building 2 (Floor I)", "Crate Passage" },
    { 9, 0, 0, "Building 2 (Floor II)", "Southwest" },
    { 9, 1, 10, "Building 2 (Floor II)", "West" },
    { 9, 2, 20, "Building 2 (Floor II)", "Northwest" },
    { 9, 3, 30, "Building 2 (Floor II)", "South" },
    { 9, 4, 40, "Building 2 (Floor II)", "Center" },
    { 9, 5, 50, "Building 2 (Floor II)", "North" },
    { 9, 6, 60, "Building 2 (Floor II)", "Southeast" },
    { 9, 7, 70, "Building 2 (Floor II)", "East" },
    { 9, 8, 80, "Building 2 (Floor II)", "Northeast" },
    { 9, 26, 260, "Building 2 (Floor II)", "Ammo Room" },
    { 9, 27, 270, "Building 2 (Floor II)", "Prisoner Cell" },
    { 9, 28, 280, "Building 2 (Floor II)", "Prisoner Cell" },
    { 9, 29, 290, "Building 2 (Floor II)", "Rolling Trap and Pitfall" },
    { 9, 30, 300, "Building 2 (Floor II)", "Rocket Launcher Room" },
    { 9, 31, 310, "Building 2 (Floor II)", "Prisoner Cell" },
    { 9, 32, 320, "Building 2 (Floor II)", "Antidote Room" },
    { 9, 33, 330, "Building 2 (Floor II)", "Compass Room" },
    { 10, 0, 0, "Building 2 (Floor B-I)", "Northwest" },
    { 10, 1, 10, "Building 2 (Floor B-I)", "Northwest" },
    { 10, 2, 20, "Building 2 (Floor B-I)", "Northwest" },
    { 10, 3, 30, "Building 2 (Floor B-I)", "North" },
    { 10, 4, 40, "Building 2 (Floor B-I)", "North" },
    { 10, 5, 50, "Building 2 (Floor B-I)", "North" },
    { 10, 6, 60, "Building 2 (Floor B-I)", "Northeast" },
    { 10, 7, 70, "Building 2 (Floor B-I)", "Northeast" },
    { 10, 8, 80, "Building 2 (Floor B-I)", "Northeast" },
    { 10, 9, 90, "Building 2 (Floor B-I)", "Underground Passage" },
    { 10, 10, 100, "Building 2 (Floor B-I)", "West" },
    { 10, 11, 110, "Building 2 (Floor B-I)", "Underground Passage" },
    { 10, 12, 120, "Building 2 (Floor B-I)", "Southwest" },
    { 10, 13, 130, "Building 2 (Floor B-I)", "Southwest" },
    { 10, 26, 260, "Building 2 (Floor B-I)", "Ammo Room" },
    { 10, 27, 270, "Building 2 (Floor B-I)", "Ration Room" },
    { 10, 28, 280, "Building 2 (Floor B-I)", "Level 6 Keycard Room" },
    { 10, 29, 290, "Building 2 (Floor B-I)", "Prison Cell Pitfall" },
    { 10, 30, 300, "Building 2 (Floor B-I)", "Prisoner Cell" },
    { 10, 31, 310, "Building 2 (Floor B-I)", "Camera Passage" },
    { 10, 32, 320, "Building 2 (Floor B-I)", "Supply Room Rolling Trap" },
    { 10, 33, 330, "Building 2 (Floor B-I)", "Prisoner Cell" },
    { 11, 0, 0, "Desert (Part II)", "South Entrance" },
    { 11, 1, 10, "Desert (Part II)", "North" },
    { 11, 2, 20, "Desert (Part II)", "North" },
    { 11, 3, 30, "Desert (Part II)", "Center" },
    { 11, 4, 40, "Desert (Part II)", "Center" },
    { 11, 5, 50, "Desert (Part II)", "South" },
    { 11, 26, 260, "Desert (Part II)", "Truck Interior" },
    { 11, 27, 270, "Desert (Part II)", "Ammo Truck" },
    { 11, 28, 280, "Desert (Part II)", "Truck Interior" },
    { 11, 29, 290, "Desert (Part II)", "Ammo Truck" },
    { 11, 30, 300, "Desert (Part II)", "Truck Interior" },
    { 11, 31, 310, "Desert (Part II)", "Truck Interior" },
    { 11, 32, 320, "Desert (Part II)", "Truck Interior" },
    { 12, 0, 0, "Building 2 Elevators", "South Elevator Basement" },
    { 12, 1, 10, "Building 2 Elevators", "South Elevator Roof" },
    { 12, 2, 20, "Building 2 Elevators", "North Elevator Basement" },
    { 12, 3, 30, "Building 2 Elevators", "North Elevator Roof" },
    { 13, 0, 0, "Building 2 (Floor III)", "Northwest" },
    { 13, 1, 10, "Building 2 (Floor III)", "Northwest" },
    { 13, 2, 20, "Building 2 (Floor III)", "South" },
    { 13, 3, 30, "Building 2 (Floor III)", "Center" },
    { 13, 4, 40, "Building 2 (Floor III)", "Center" },
    { 13, 5, 50, "Building 2 (Floor III)", "North" },
    { 13, 6, 60, "Building 2 (Floor III)", "Southeast" },
    { 13, 26, 260, "Building 2 (Floor III)", "Prisoner Cell" },
    { 13, 27, 270, "Building 2 (Floor III)", "Level 5 Keycard Room" },
    { 13, 28, 280, "Building 2 (Floor III)", "Side Room" },
    { 14, 0, 0, "Building 3 (Floor I)", "Entry Corridor" },
    { 14, 1, 10, "Building 3 (Floor I)", "Entrance" },
    { 14, 2, 20, "Building 3 (Floor I)", "North" },
    { 14, 3, 30, "Building 3 (Floor I)", "East Room" },
    { 14, 26, 260, "Building 3 (Floor I)", "Crate Room Pitfall" },
    { 14, 27, 270, "Building 3 (Floor I)", "Ammo Room" },
    { 14, 28, 280, "Building 3 (Floor I)", "Dirty Duck and Hostages" },
    { 15, 0, 0, "Building 3 (B100)", "South" },
    { 15, 1, 10, "Building 3 (B100)", "South" },
    { 15, 2, 20, "Building 3 (B100)", "Center" },
    { 15, 3, 30, "Building 3 (B100)", "Center" },
    { 15, 4, 40, "Building 3 (B100)", "Southeast" },
    { 15, 5, 50, "Building 3 (B100)", "North" },
    { 15, 6, 60, "Building 3 (B100)", "Metal Gear Hangar" },
    { 15, 7, 70, "Building 3 (B100)", "Northwest" },
    { 15, 26, 260, "Building 3 (B100)", "Prisoner Cell" },
    { 15, 27, 270, "Building 3 (B100)", "Oxygen Cylinder Room" },
    { 16, 0, 0, "Building 3 Elevator", "Elevator B100" },
    { 16, 1, 10, "Building 3 Elevator", "Lower Shaft" },
    { 16, 2, 20, "Building 3 Elevator", "Upper Shaft" },
    { 16, 3, 30, "Building 3 Elevator", "Elevator 1F" },
    { 17, 0, 0, "Escape Ladders", "Bottom" },
    { 17, 1, 10, "Escape Ladders", "Middle" },
    { 17, 2, 20, "Escape Ladders", "Surface Exit" },
};

inline constexpr MGRoomName astMG2RoomNames[] =
{
    { 0, 0, 0, "Zanzibar Bldg. (Exterior)", "Infiltration Point" },
    { 0, 1, 10, "Zanzibar Bldg. (Exterior)", "Southwest" },
    { 0, 2, 20, "Zanzibar Bldg. (Exterior)", "South" },
    { 0, 3, 30, "Zanzibar Bldg. (Exterior)", "Southeast" },
    { 0, 4, 40, "Zanzibar Bldg. (Exterior)", "West" },
    { 0, 5, 50, "Zanzibar Bldg. (Exterior)", "Central" },
    { 0, 6, 60, "Zanzibar Bldg. (Exterior)", "East" },

    { 0, 7, 70, "Zanzibar Bldg. (Exterior)", "Northwest" },
    { 0, 8, 80, "Zanzibar Bldg. (Exterior)", "Front Door" },
    { 0, 9, 90, "Zanzibar Bldg. (Exterior)", "Northeast" },


    { 0, 10, 100, "Zanzibar Bldg. (Vents)", "West" },
    { 0, 11, 110, "Zanzibar Bldg. (Vents)", "Center" },
    { 0, 12, 120, "Zanzibar Bldg. (Vents)", "East" },


    { 0, 13, 130, "Zanzibar Bldg. (Exterior)", "Truck (South)" }, // Provisional room name.
    { 0, 14, 140, "Zanzibar Bldg. (Exterior)", "Truck (North)" },


    { 1, 0, 0, "Building 1 (Floor I)", "Southwest" },
    { 1, 1, 10, "Building 1 (Floor I)", "Front Entrance" },
    { 1, 2, 20, "Building 1 (Floor I)", "Vent Entrance" },
    { 1, 3, 30, "Building 1 (Floor I)", "Southeast" },
    { 1, 4, 40, "Building 1 (Floor I)", "West" },
    { 1, 5, 50, "Building 1 (Floor I)", "West" },
    { 1, 6, 60, "Building 1 (Floor I)", "Center" },
    { 1, 7, 70, "Building 1 (Floor I)", "East" },
    { 1, 8, 80, "Building 1 (Floor I)", "Southeast" },
    { 1, 9, 90, "Building 1 (Floor I)", "West" },
    { 1, 10, 100, "Building 1 (Floor I)", "West" },
    { 1, 11, 110, "Building 1 (Floor I)", "Center" },
    { 1, 12, 120, "Building 1 (Floor I)", "East" },
    { 1, 13, 130, "Building 1 (Floor I)", "East" },
    { 1, 14, 140, "Building 1 (Floor I)", "Northwest" },
    { 1, 15, 150, "Building 1 (Floor I)", "Northwest" },
    { 1, 16, 160, "Building 1 (Floor I)", "North" },
    { 1, 17, 170, "Building 1 (Floor I)", "Northeast" },
    { 1, 18, 180, "Building 1 (Floor I)", "North" },
    { 1, 19, 190, "Building 1 (Floor I)", "Northwest" },
    { 1, 20, 200, "Building 1 (Floor I)", "North" },
    { 1, 21, 210, "Building 1 (Floor I)", "Northeast" },
    { 1, 22, 220, "Building 1 (Floor I)", "Northeast" },
    { 1, 23, 230, "Building 1 (Floor I)", "Stinger Missile Room" },
    { 1, 24, 240, "Building 1 (Floor I)", "RC Missile Room" },
    { 1, 25, 250, "Building 1 (Floor I)", "Hang Glider Room" },
    { 1, 26, 260, "Building 1 (Floor I)", "Blue Card Room" },
    { 1, 27, 270, "Building 1 (Floor I)", "Red Card Room" },
    { 1, 28, 280, "Building 1 (Floor I)", "Green Card Room" },
    { 1, 29, 290, "Building 1 (Floor I)", "Northeast" },
    { 1, 30, 300, "Building 1 (Floor I)", "Elevator" },
    { 1, 31, 310, "Building 1 (Floor I)", "Elevator" },
    { 1, 32, 320, "Building 1 (Floor I)", "Truck (R22)" },
    { 2, 0, 0, "Building 1 (Floor II)", "Southwest" },
    { 2, 1, 10, "Building 1 (Floor II)", "Southwest" },
    { 2, 2, 20, "Building 1 (Floor II)", "Southeast" },
    { 2, 3, 30, "Building 1 (Floor II)", "West" },
    { 2, 4, 40, "Building 1 (Floor II)", "Center" },
    { 2, 5, 50, "Building 1 (Floor II)", "East" },
    { 2, 6, 60, "Building 1 (Floor II)", "Northwest" },
    { 2, 7, 70, "Building 1 (Floor II)", "Northwest" },
    { 2, 8, 80, "Building 1 (Floor II)", "Northeast" },
    { 2, 9, 90, "Building 1 (Floor II)", "West" },
    { 2, 10, 100, "Building 1 (Floor II)", "Northeast" },
    { 2, 11, 110, "Building 1 (Floor II)", "East" },
    { 2, 12, 120, "Building 1 (Floor II)", "Elevator" },
    { 2, 13, 130, "Building 1 (Floor II)", "Elevator" },
    { 3, 0, 0, "Building 1 (Floor III)", "South" },
    { 3, 1, 10, "Building 1 (Floor III)", "Southeast" },
    { 3, 2, 20, "Building 1 (Floor III)", "Southeast" },
    { 3, 3, 30, "Building 1 (Floor III)", "West" },
    { 3, 4, 40, "Building 1 (Floor III)", "West" },
    { 3, 5, 50, "Building 1 (Floor III)", "Center" },
    { 3, 6, 60, "Building 1 (Floor III)", "East" },
    { 3, 7, 70, "Building 1 (Floor III)", "East" },
    { 3, 8, 80, "Building 1 (Floor III)", "West" },
    { 3, 9, 90, "Building 1 (Floor III)", "West" },
    { 3, 10, 100, "Building 1 (Floor III)", "Center" },
    { 3, 11, 110, "Building 1 (Floor III)", "East" },
    { 3, 12, 120, "Building 1 (Floor III)", "East" },
    { 3, 13, 130, "Building 1 (Floor III)", "Northwest" },
    { 3, 14, 140, "Building 1 (Floor III)", "North" },
    { 3, 15, 150, "Building 1 (Floor III)", "Northeast" },
    { 3, 16, 160, "Building 1 (Floor III)", "Northeast" },
    { 3, 17, 170, "Building 1 (Floor III)", "Northwest" },
    { 3, 18, 180, "Building 1 (Floor III)", "North" },
    { 3, 19, 190, "Building 1 (Floor III)", "Northeast" },
    { 3, 20, 200, "Building 1 (Floor III)", "Northeast" },
    { 3, 21, 210, "Building 1 (Floor III)", "West" },
    { 3, 22, 220, "Building 1 (Floor III)", "Ration Room" },
    { 3, 23, 230, "Building 1 (Floor III)", "Mine Room" },
    { 3, 24, 240, "Building 1 (Floor III)", "RC Missile Room" },
    { 3, 25, 250, "Building 1 (Floor III)", "Gas Grenade Room" },
    { 3, 26, 260, "Building 1 (Floor III)", "East" },
    { 3, 27, 270, "Building 1 (Floor III)", "Gas Mask Room" },
    { 3, 28, 280, "Building 1 (Floor III)", "Mine Detector Room" },
    { 3, 29, 290, "Building 1 (Floor III)", "Northeast" },
    { 3, 30, 300, "Building 1 (Floor III)", "West" },
    { 3, 31, 310, "Building 1 (Floor III)", "Chute" },
    { 3, 32, 320, "Building 1 (Floor III)", "Elevator" },
    { 3, 33, 330, "Building 1 (Floor III)", "Elevator" },
    { 4, 0, 0, "Building 1 (Floor IV)", "Southwest" },
    { 4, 1, 10, "Building 1 (Floor IV)", "Southwest" },
    { 4, 2, 20, "Building 1 (Floor IV)", "South" },
    { 4, 3, 30, "Building 1 (Floor IV)", "Southeast" },
    { 4, 4, 40, "Building 1 (Floor IV)", "Southeast" },
    { 4, 5, 50, "Building 1 (Floor IV)", "Southwest" },
    { 4, 6, 60, "Building 1 (Floor IV)", "West" },
    { 4, 7, 70, "Building 1 (Floor IV)", "Center" },
    { 4, 8, 80, "Building 1 (Floor IV)", "East" },
    { 4, 9, 90, "Building 1 (Floor IV)", "East" },
    { 4, 10, 100, "Building 1 (Floor IV)", "West" },
    { 4, 11, 110, "Building 1 (Floor IV)", "West" },
    { 4, 12, 120, "Building 1 (Floor IV)", "Center" },
    { 4, 13, 130, "Building 1 (Floor IV)", "East" },
    { 4, 14, 140, "Building 1 (Floor IV)", "East" },
    { 4, 15, 150, "Building 1 (Floor IV)", "Northwest" },
    { 4, 16, 160, "Building 1 (Floor IV)", "Northwest" },
    { 4, 17, 170, "Building 1 (Floor IV)", "North" },
    { 4, 18, 180, "Building 1 (Floor IV)", "Northeast" },
    { 4, 19, 190, "Building 1 (Floor IV)", "Northeast" },
    { 4, 20, 200, "Building 1 (Floor IV)", "Northwest" },
    { 4, 21, 210, "Building 1 (Floor IV)", "Northwest" },
    { 4, 22, 220, "Building 1 (Floor IV)", "North" },
    { 4, 23, 230, "Building 1 (Floor IV)", "Northeast" },
    { 4, 24, 240, "Building 1 (Floor IV)", "Southeast" },
    { 4, 25, 250, "Building 1 (Floor IV)", "Northwest" },
    { 4, 26, 260, "Building 1 (Floor IV)", "Ration Room" },
    { 4, 27, 270, "Building 1 (Floor IV)", "Cold Medicine Room" },
    { 4, 28, 280, "Building 1 (Floor IV)", "Northwest" },
    { 4, 29, 290, "Building 1 (Floor IV)", "Uniform Room" },
    { 4, 30, 300, "Building 1 (Floor IV)", "North" },
    { 4, 31, 310, "Building 1 (Floor IV)", "Locker Interior" },
    { 4, 32, 320, "Building 1 (Floor IV)", "Cassette Tape Locker" },
    { 4, 33, 330, "Building 1 (Floor IV)", "Bucket Room" },
    { 4, 34, 340, "Building 1 (Floor IV)", "Southwest" },
    { 4, 35, 350, "Building 1 (Floor IV)", "Northeast" },
    { 4, 36, 360, "Building 1 (Floor IV)", "Northeast" },
    { 4, 37, 370, "Building 1 (Floor IV)", "East" },
    { 4, 38, 380, "Building 1 (Floor IV)", "Narrow Shaft" },
    { 4, 39, 390, "Building 1 (Floor IV)", "Elevator" },
    { 4, 40, 400, "Building 1 (Floor IV)", "Elevator" },
    { 4, 41, 410, "Building 1 (Floor IV)", "Southeast" },
    { 5, 0, 0, "Building 1 (Floor B-I)", "Southwest" },
    { 5, 1, 10, "Building 1 (Floor B-I)", "South" },
    { 5, 2, 20, "Building 1 (Floor B-I)", "Southeast" },
    { 5, 3, 30, "Building 1 (Floor B-I)", "West" },
    { 5, 4, 40, "Building 1 (Floor B-I)", "Center" },
    { 5, 5, 50, "Building 1 (Floor B-I)", "East" },
    { 5, 6, 60, "Building 1 (Floor B-I)", "Northwest" },
    { 5, 7, 70, "Building 1 (Floor B-I)", "North" },
    { 5, 8, 80, "Building 1 (Floor B-I)", "Northeast" },
    { 5, 9, 90, "Building 1 (Floor B-I)", "Explosives Room" },
    { 5, 10, 100, "Building 1 (Floor B-I)", "Ammo Room" },
    { 5, 11, 110, "Building 1 (Floor B-I)", "Submachine Gun Room" },
    { 5, 12, 120, "Building 1 (Floor B-I)", "Grenade Room" },
    { 5, 13, 130, "Building 1 (Floor B-I)", "Oxygen Tank Room" },
    { 5, 14, 140, "Building 1 (Floor B-I)", "RC Missile Room" },
    { 5, 15, 150, "Building 1 (Floor B-I)", "Stinger Missile Room" },
    { 5, 16, 160, "Building 1 (Floor B-I)", "Gas Grenade Room" },
    { 5, 17, 170, "Building 1 (Floor B-I)", "Camouflage Mat Room" },
    { 5, 18, 180, "Building 1 (Floor B-I)", "Infrared Goggles Room" },
    { 5, 19, 190, "Building 1 (Floor B-I)", "Boarded Ammo Room" },
    { 5, 20, 200, "Building 1 (Floor B-I)", "Body Armor Room" },
    { 5, 21, 210, "Building 1 (Floor B-I)", "Grenade Room" },
    { 5, 22, 220, "Building 1 (Floor B-I)", "Elevator" },
    { 5, 23, 230, "Building 1 (Floor B-I)", "Elevator" },
    { 6, 0, 0, "Sewers", "Southwest" },
    { 6, 1, 10, "Sewers", "Southwest" },
    { 6, 2, 20, "Sewers", "South" },
    { 6, 3, 30, "Sewers", "Southeast" },
    { 6, 4, 40, "Sewers", "Southeast" },
    { 6, 5, 50, "Sewers", "Southwest" },
    { 6, 6, 60, "Sewers", "Southwest" },
    { 6, 7, 70, "Sewers", "South" },
    { 6, 8, 80, "Sewers", "Southeast" },
    { 6, 9, 90, "Sewers", "Southeast" },
    { 6, 10, 100, "Sewers", "Southwest" },
    { 6, 11, 110, "Sewers", "Southwest" },
    { 6, 12, 120, "Sewers", "South" },
    { 6, 13, 130, "Sewers", "Southeast" },
    { 6, 14, 140, "Sewers", "Southeast" },
    { 6, 15, 150, "Sewers", "Southwest" },
    { 6, 16, 160, "Sewers", "Waste Disposal" },
    { 6, 17, 170, "Sewers", "Southeast" },
    { 6, 18, 180, "Sewers", "West" },
    { 6, 19, 190, "Sewers", "East" },
    { 6, 20, 200, "Sewers", "West" },
    { 6, 21, 210, "Sewers", "East" },
    { 6, 22, 220, "Sewers", "West" },
    { 6, 23, 230, "Sewers", "East" },
    { 6, 24, 240, "Sewers", "Northwest" },
    { 6, 25, 250, "Sewers", "Northeast" },
    { 6, 26, 260, "Sewers", "Northwest" },
    { 6, 27, 270, "Sewers", "Northeast" },
    { 6, 28, 280, "Sewers", "Northwest" },
    { 6, 29, 290, "Sewers", "Northwest" },
    { 6, 30, 300, "Sewers", "North" },
    { 6, 31, 310, "Sewers", "Northeast" },
    { 6, 32, 320, "Sewers", "Northeast" },
    { 6, 33, 330, "Sewers", "Northwest" },
    { 6, 34, 340, "Sewers", "Northwest" },
    { 6, 35, 350, "Sewers", "North" },
    { 6, 36, 360, "Holly's Cell", "West" },
    { 6, 37, 370, "Holly's Cell", "Center" },
    { 6, 38, 380, "Holly's Cell", "East" },
    { 6, 39, 390, "Sewers", "Explosives Room" },
    { 6, 40, 400, "Sewers", "Ammo Room" },
    { 6, 41, 410, "Sewers", "Elevator" },
    { 6, 42, 420, "Sewers", "Elevator" },
    { 6, 43, 430, "Sewers", "Elevator" },
    { 7, 0, 0, "", "Lost Woods" },
    { 7, 1, 10, "Swamp & Desert", "Southwest" },
    { 7, 2, 20, "Swamp & Desert", "South" },
    { 7, 3, 30, "Swamp & Desert", "Southeast" },
    { 7, 4, 40, "Swamp & Desert", "Southeast" },
    { 7, 5, 50, "Swamp & Desert", "Southwest" },
    { 7, 6, 60, "Swamp & Desert", "Southwest" },
    { 7, 7, 70, "Swamp & Desert", "South" },
    { 7, 8, 80, "Swamp & Desert", "Southeast" },
    { 7, 9, 90, "Swamp & Desert", "Southeast" },
    { 7, 10, 100, "Swamp & Desert", "Southwest" },
    { 7, 11, 110, "Desert", "South" },
    { 7, 12, 120, "Lost Woods", "Detention Camp" },
    { 7, 13, 130, "Swamp & Desert", "Center" },
    { 7, 14, 140, "Swamp & Desert", "Center" },
    { 7, 15, 150, "Swamp & Desert", "Southwest" },
    { 7, 16, 160, "Swamp & Desert", "North" },
    { 7, 17, 170, "Swamp & Desert", "North" },
    { 7, 18, 180, "Swamp & Desert", "Northeast" },
    { 7, 19, 190, "Swamp & Desert", "North" },
    { 7, 20, 200, "Swamp & Desert", "Northeast" },
    { 7, 21, 210, "Desert", "North" },
    { 7, 22, 220, "Running Man's Shed", "Northwest" },
    { 7, 23, 230, "Running Man's Shed", "Northeast" },
    { 7, 24, 240, "Running Man's Shed", "West" },
    { 7, 25, 250, "Running Man's Shed", "East" },
    { 7, 26, 260, "Running Man's Shed", "Southwest" },
    { 7, 27, 270, "Dr. Pettrovich Madnar's Cell", "East" },
    { 7, 28, 280, "Detention Camp", "Cell Interior" },
    { 7, 29, 290, "Detention Camp", "West Cell Interior" },
    { 7, 30, 300, "Detention Camp", "Elevator" },
    { 7, 31, 310, "Swamp & Desert", "Truck (R14)" },
    { 7, 32, 320, "Swamp & Desert", "Truck (R14)" },
    { 8, 0, 0, "Building 1 (Floor B-III)", "Southeast" },
    { 8, 1, 10, "Building 1 (Floor B-III)", "Southwest" },
    { 8, 2, 20, "Building 1 (Floor B-III)", "Southeast" },
    { 8, 3, 30, "Building 1 (Floor B-III)", "Southwest" },
    { 8, 4, 40, "Building 1 (Floor B-III)", "Southeast" },
    { 8, 5, 50, "Building 1 (Floor B-III)", "Southwest" },
    { 8, 6, 60, "Building 1 (Floor B-III)", "Southeast" },
    { 8, 7, 70, "Building 1 (Floor B-III)", "West" },
    { 8, 8, 80, "Building 1 (Floor B-III)", "West" },
    { 8, 9, 90, "Building 1 (Floor B-III)", "East" },
    { 8, 10, 100, "Building 1 (Floor B-III)", "East" },
    { 8, 11, 110, "Building 1 (Floor B-III)", "East" },
    { 8, 12, 120, "Building 1 (Floor B-III)", "Northeast" },
    { 8, 13, 130, "Building 1 (Floor B-III)", "Northeast" },
    { 8, 14, 140, "Building 1 (Floor B-III)", "Northeast" },
    { 8, 15, 150, "Building 1 (Floor B-III)", "Northeast" },
    { 8, 16, 160, "Building 1 (Floor B-III)", "Elevator" },
    { 8, 17, 170, "Building 1 (Floor B-III)", "Elevator" },
    { 8, 18, 180, "Building 1 (Floor B-III)", "Elevator" },
    { 9, 0, 0, "Building 2 Entrance", "West" },
    { 9, 1, 10, "Building 2 Entrance", "Center" },
    { 9, 2, 20, "Building 2 Entrance", "East" },
    { 9, 3, 30, "Building 2 Entrance", "West" },
    { 9, 4, 40, "Building 2 Entrance", "Center" },
    { 9, 5, 50, "Building 2 Entrance", "Truck (R2)" },
    { 10, 0, 0, "Grass Field & Biology Lab", "South" },
    { 10, 1, 10, "Grass Field & Biology Lab", "Southeast" },
    { 10, 2, 20, "Detention Camp", "Southwest" },
    { 10, 3, 30, "Detention Camp", "Entrance" },
    { 10, 4, 40, "Detention Camp", "South" },
    { 10, 5, 50, "Grass Field & Biology Lab", "South" },
    { 10, 6, 60, "Grass Field & Biology Lab", "Southeast" },
    { 10, 7, 70, "Detention Camp", "West" },
    { 10, 8, 80, "Detention Camp", "West" },
    { 10, 9, 90, "Detention Camp", "Center" },
    { 10, 10, 100, "", "Bio Lab" },
    { 10, 11, 110, "Detention Camp", "Northwest" },
    { 10, 12, 120, "Detention Camp", "Northwest" },
    { 10, 13, 130, "Detention Camp", "North" },
    { 10, 14, 140, "Detention Camp", "East" },
    { 10, 15, 150, "Detention Camp", "Southwest" },
    { 10, 16, 160, "Detention Camp", "Southeast" },
    { 10, 17, 170, "Detention Camp", "Southwest" },
    { 10, 18, 180, "Detention Camp", "Northeast" },
    { 10, 19, 190, "Detention Camp", "Northeast" },
    { 10, 20, 200, "Grass Field & Biology Lab", "Northwest" },
    { 10, 21, 210, "Grass Field & Biology Lab", "North" },
    { 10, 22, 220, "Grass Field & Biology Lab", "Northeast" },
    { 10, 23, 230, "Detention Camp", "East" },
    { 10, 24, 240, "Grass Field & Biology Lab", "North" },
    { 10, 25, 250, "Detention Camp", "North" },
    { 11, 0, 0, "Building 2 (Floor I)", "Southwest" },
    { 11, 1, 10, "Building 2 (Floor I)", "South Entrance" },
    { 11, 2, 20, "Building 2 (Floor I)", "Warehouse" },
    { 11, 3, 30, "Building 2 (Floor I)", "Northwest" },
    { 11, 4, 40, "Building 2 (Floor I)", "North" },
    { 11, 5, 50, "Building 2 (Floor I)", "Northeast" },
    { 11, 6, 60, "Building 2 (Floor I)", "North" },
    { 11, 7, 70, "Building 2 (Floor I)", "North" },
    { 11, 8, 80, "Building 2 (Floor I)", "Northeast" },
    { 11, 9, 90, "Building 2 (Floor I)", "Northeast" },
    { 11, 10, 100, "Building 2 (Floor I)", "North" },
    { 11, 11, 110, "Building 2 (Floor I)", "Elevator" },
    { 11, 12, 120, "Building 2 (Floor I)", "Elevator" },
    { 11, 13, 130, "Building 2 (Floor I)", "Truck (R9)" },
    { 12, 0, 0, "Building 2 (Floor X)", "South" },
    { 12, 1, 10, "Building 2 (Floor X)", "Southeast" },
    { 12, 2, 20, "Building 2 (Floor X)", "Stairwell Entrance" },
    { 12, 3, 30, "Building 2 (Floor X)", "North" },
    { 12, 4, 40, "Building 2 (Floor X)", "Northeast" },
    { 12, 5, 50, "Building 2 (Floor X)", "South" },
    { 12, 6, 60, "Building 2 (Floor X)", "Southeast" },
    { 12, 7, 70, "Building 2 (Floor X)", "North" },
    { 12, 8, 80, "Building 2 (Floor X)", "Elevator" },
    { 12, 9, 90, "Building 2 (Floor X)", "Elevator" },
    { 13, 0, 0, "Building 2 (Floor XX)", "South" },
    { 13, 1, 10, "Building 2 (Floor XX)", "Stairwell Entrance" },
    { 13, 2, 20, "Building 2 (Floor XX)", "North" },
    { 13, 3, 30, "Building 2 (Floor XX)", "Northeast" },
    { 13, 4, 40, "Building 2 (Floor XX)", "North" },
    { 13, 5, 50, "Building 2 (Floor XX)", "Northeast" },
    { 13, 6, 60, "Building 2 (Floor XX)", "Southeast" },
    { 13, 7, 70, "Building 2 (Floor XX)", "West" },
    { 13, 8, 80, "Building 2 (Floor XX)", "East" },
    { 13, 9, 90, "Building 2 (Floor XX)", "Elevator" },
    { 13, 10, 100, "Building 2 (Floor XX)", "South" },
    { 13, 11, 110, "Building 2 (Floor XX)", "Elevator" },
    { 14, 0, 0, "Building 2 (Floor XXX)", "Southwest" },
    { 14, 1, 10, "Building 2 (Floor XXX)", "South" },
    { 14, 2, 20, "Building 2 (Floor XXX)", "West" },
    { 14, 3, 30, "Building 2 (Floor XXX)", "Center" },
    { 14, 4, 40, "Building 2 (Floor XXX)", "North" },
    { 14, 5, 50, "Building 2 (Floor XXX)", "Northeast" },
    { 14, 6, 60, "Building 2 (Floor XXXI)", "Southwest" },
    { 14, 7, 70, "Building 2 (Floor XXXI)", "Roof" },
    { 14, 8, 80, "Building 2 (Floor XXXI)", "East" },
    { 14, 9, 90, "Building 2 (Floor XXXI)", "Northwest" },
    { 14, 10, 100, "Building 2 (Floor XXXI)", "Northeast" },
    { 14, 11, 110, "Building 2 (Floor XXX)", "Northeast" },
    { 15, 0, 0, "Bridge of Sorrow", "South Truck Yard" },
    { 15, 1, 10, "Bridge of Sorrow", "Truckyard Fence" },
    { 15, 2, 20, "Bridge of Sorrow", "South Crate Yard" },
    { 15, 3, 30, "Bridge of Sorrow", "Generator Shed" },
    { 15, 4, 40, "Bridge of Sorrow", "Bridge" },
    { 15, 5, 50, "Bridge of Sorrow", "Northwest Truck Yard" },
    { 15, 6, 60, "Bridge of Sorrow", "Cliffside Crates" },
    { 15, 7, 70, "Bridge of Sorrow", "North Pass" },
    { 15, 8, 80, "Bridge of Sorrow", "Twin Truck Yard" },
    { 15, 9, 90, "Bridge of Sorrow", "Tank and Sandbags" },
    { 15, 10, 100, "Bridge of Sorrow", "Northeast Truck Yard" },
    { 15, 11, 110, "Bridge of Sorrow", "Truck (R0)" },
    { 15, 12, 120, "Bridge of Sorrow", "Truck (R1)" },
    { 15, 13, 130, "Bridge of Sorrow", "Truck (R1)" },
    { 15, 14, 140, "Bridge of Sorrow", "Truck (R1)" },
    { 15, 15, 150, "Bridge of Sorrow", "Truck (R1)" },
    { 15, 16, 160, "Bridge of Sorrow", "Truck (R8)" },
    { 15, 17, 170, "Bridge of Sorrow", "Truck (R8)" },
    { 15, 18, 180, "Bridge of Sorrow", "Truck (R10)" },
    { 15, 19, 190, "Bridge of Sorrow", "Truck (R5)" },
    { 16, 0, 0, "Spiral Staircase", "12F West Landing" },
    { 16, 1, 10, "Spiral Staircase", "11F East Landing" },
    { 16, 2, 20, "Spiral Staircase", "13F West Landing" },
    { 16, 3, 30, "Spiral Staircase", "12F East Landing" },
    { 16, 4, 40, "Spiral Staircase", "14F West Landing" },
    { 16, 5, 50, "Spiral Staircase", "13F East Landing" },
    { 16, 6, 60, "Spiral Staircase", "11F West Landing" },
    { 16, 7, 70, "Spiral Staircase", "10F Entrance" },
    { 16, 8, 80, "Spiral Staircase", "11F-12F West Stairs" },
    { 16, 9, 90, "Spiral Staircase", "11F-12F East Stairs" },
    { 16, 10, 100, "Spiral Staircase", "12F-13F West Stairs" },
    { 16, 11, 110, "Spiral Staircase", "12F-13F East Stairs" },
    { 16, 12, 120, "Spiral Staircase", "13F-14F West Stairs" },
    { 16, 13, 130, "Spiral Staircase", "13F-14F East Stairs" },
    { 16, 14, 140, "Spiral Staircase", "10F-11F West Stairs" },
    { 16, 15, 150, "Spiral Staircase", "10F-11F East Stairs" },
    { 16, 16, 160, "Spiral Staircase", "15F West Landing" },
    { 16, 17, 170, "Spiral Staircase", "14F East Landing" },
    { 16, 18, 180, "Spiral Staircase", "16F West Landing" },
    { 16, 19, 190, "Spiral Staircase", "15F East Landing" },
    { 16, 20, 200, "Spiral Staircase", "17F West Landing" },
    { 16, 21, 210, "Spiral Staircase", "16F East Landing" },
    { 16, 22, 220, "Spiral Staircase", "20F Exit" },
    { 16, 23, 230, "Spiral Staircase", "14F-15F West Stairs" },
    { 16, 24, 240, "Spiral Staircase", "14F-15F East Stairs" },
    { 16, 25, 250, "Spiral Staircase", "15F-16F West Stairs" },
    { 16, 26, 260, "Spiral Staircase", "15F-16F East Stairs" },
    { 16, 27, 270, "Spiral Staircase", "16F-17F West Stairs" },
    { 16, 28, 280, "Spiral Staircase", "16F-17F East Stairs" },
    { 16, 29, 290, "Spiral Staircase", "18F West Landing" },
    { 16, 30, 300, "Spiral Staircase", "17F East Landing" },
    { 16, 31, 310, "Spiral Staircase", "19F West Landing" },
    { 16, 32, 320, "Spiral Staircase", "18F East Landing" },
    { 16, 33, 330, "Spiral Staircase", "20F West Landing" },
    { 16, 34, 340, "Spiral Staircase", "19F East Landing" },
    { 16, 35, 350, "Spiral Staircase", "17F-18F West Stairs" },
    { 16, 36, 360, "Spiral Staircase", "17F-18F East Stairs" },
    { 16, 37, 370, "Spiral Staircase", "18F-19F West Stairs" },
    { 16, 38, 380, "Spiral Staircase", "18F-19F East Stairs" },
    { 16, 39, 390, "Spiral Staircase", "19F-20F West Stairs" },
    { 16, 40, 400, "Spiral Staircase", "19F-20F East Stairs" },
    { 17, 0, 0, "Escape Tunnel (Part II)", "Southwest" },
    { 17, 1, 10, "Escape Tunnel (Part II)", "Southeast" },
    { 17, 2, 20, "Escape Tunnel (Part II)", "Southeast" },
    { 17, 3, 30, "Escape Tunnel (Part II)", "East" },
    { 17, 4, 40, "Escape Tunnel (Part II)", "Northeast" },
    { 17, 5, 50, "Escape Tunnel (Part II)", "Northeast" },
    { 18, 0, 0, "Big Boss' Hideout", "Southeast" },
    { 18, 1, 10, "Big Boss' Hideout", "West" },
    { 18, 2, 20, "Big Boss' Hideout", "Big Boss Battle" },
    { 18, 3, 30, "Big Boss' Hideout", "East" },
    { 18, 4, 40, "Big Boss' Hideout", "Southeast" },
    { 18, 5, 50, "Big Boss' Hideout", "West" },
    { 18, 6, 60, "Big Boss' Hideout", "West" },
    { 18, 7, 70, "Big Boss' Hideout", "Center" },
    { 18, 8, 80, "Big Boss' Hideout", "East" },
    { 18, 9, 90, "Big Boss' Hideout", "Northwest" },
    { 18, 10, 100, "Big Boss' Hideout", "Escape Route" },
    { 18, 11, 110, "Big Boss' Hideout", "North" },
    { 18, 12, 120, "Escape Tunnel (Part I)", "South" },
    { 18, 13, 130, "Escape Tunnel (Part I)", "South" },
    { 18, 14, 140, "Escape Tunnel (Part I)", "South" },
    { 18, 15, 150, "Escape Tunnel (Part I)", "Center" },
    { 18, 16, 160, "Escape Tunnel (Part I)", "North" },
    { 18, 17, 170, "Escape Tunnel (Part I)", "North" },
    { 18, 18, 180, "Escape Tunnel (Part I)", "North" },
    { 18, 19, 190, "Escape Tunnel (Part I)", "North" },
    { 18, 20, 200, "Big Boss' Hideout", "Spray and Keycard Rooms" },
    { 18, 21, 210, "Big Boss' Hideout", "Lighter and Keycard Rooms" },
    { 18, 22, 220, "Big Boss' Hideout", "Elevator" },
    { 19, 0, 0, "Detention Camp Cellar", "Southwest" },
    { 19, 1, 10, "Detention Camp Cellar", "Southeast" },
    { 19, 2, 20, "Detention Camp Cellar", "West" },
    { 19, 3, 30, "Detention Camp Cellar", "East" },
    { 19, 4, 40, "Detention Camp Cellar", "Northeast" },
    { 19, 5, 50, "Dr. Marv's Cell", "South" },
    { 19, 6, 60, "Dr. Marv's Cell", "North" },
    { 19, 7, 70, "Dr. Marv's Cell", "Center" },
    { 19, 8, 80, "Dr. Marv's Cell", "Chute 1" },
    { 19, 9, 90, "Dr. Marv's Cell", "Chute 2" },
    { 19, 10, 100, "Dr. Marv's Cell", "Chute 3" },
    { 19, 11, 110, "Dr. Marv's Cell", "Chute 4" },
    { 19, 12, 120, "Detention Camp Cellar", "Elevator" },
};

[[nodiscard]] inline const MGRoomName* FindMGRoomName(bool bMG2, int nArea, int nRoom)
{
    const auto FindName = [nArea, nRoom](const auto& astNames) -> const MGRoomName*
    {
        for (const auto& stName : astNames)
        {
            if (stName.nArea == nArea && stName.nRoom == nRoom)
            {
                return &stName;
            }
        }
        return nullptr;
    };
    return bMG2 ? FindName(astMG2RoomNames) : FindName(astMG1RoomNames);
}

[[nodiscard]] inline std::string GetMGRoomName(bool bMG2, int nArea, int nRoom)
{
    const MGRoomName* pRoom = FindMGRoomName(bMG2, nArea, nRoom);
    if (!pRoom)
    {
        return "Unnamed room";
    }
    return *pRoom->pszLocation ? std::string(pRoom->pszLocation) + ": " + pRoom->pszName : pRoom->pszName;
}
