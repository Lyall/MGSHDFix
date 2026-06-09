// ReSharper disable IdentifierTypo
#pragma once

namespace MGS2_Defines
{

//// GM_SeSet
    constexpr int GM_MAX_VOL = (0x3F);		/* Maximum volume */
    constexpr int GM_MAX_PAN = (0x1F);		/* Maximum pan */
    constexpr int GM_VOL_BOMB = (24);		/* Minimum volume for bomb-type sounds */
    constexpr int GM_PAN_CENTER = (0x20);		/* Center pan */
    constexpr int GM_MAX_TRACK = (0x20);

    ///  GM_SeSet Sounds
    ///  P_ = Player Sounds, W_ = Weapon Sounds, I_ = Item Sounds, E_ = Enemy, V_ = Voice, S_ = System
    ///
    constexpr int SD_W_SOCOM01 = 0x01;		// SOCOM shot
    constexpr int SD_P_FOOTL01 = 0x02;		// Normal left footstep
    constexpr int SD_P_FOOTR01 = 0x03;		// Normal right footstep
    constexpr int SD_W_EMPTY01 = 0x04;		// SOCOM dry fire
    constexpr int SD_P_HOFUKU05 = 0x05;		// Crawl left
    constexpr int SD_P_HOFUKU06 = 0x06;		// Crawl right
    constexpr int SD_P_SENAKA02 = 0x07;		// Press against wall
    constexpr int SD_P_STAND02 = 0x08;		// Crouch or stand up
    constexpr int SD_W_EQUIP02 = 0x09;		// Ready SOCOM
    constexpr int SD_P_HEARTB01 = 0x0A;		// Heartbeat
    constexpr int SD_S_FULL0005 = 0x0B;		// Item inventory full
    constexpr int SD_S_KAIHUKU1 = 0x0C;		// Life recovery
    constexpr int SD_S_ITEM0003 = 0x0D;		// Item appears
    constexpr int SD_S_RADAR001 = 0x0E;		// Radar restored
    constexpr int SD_S_OVER04 = 0x0F;		// Game over
    
    constexpr int SD_P_DROP01 = 0x10;		// Jump
    constexpr int SD_I_CAMERA02 = 0x11;		// Digital camera shutter
    constexpr int SD_P_FOOTL02 = 0x12;		// Soft footstep (left)
    constexpr int SD_P_FOOTR02 = 0x13;		// Soft footstep (right)
    constexpr int SD_S_EQUIP01 = 0x14;		// Confirm SOCOM equip
    constexpr int SD_S_IDISP02 = 0x15;		// Display item menu
    constexpr int SD_S_IGET02 = 0x16;		// Obtain item
    constexpr int SD_S_ISEL02 = 0x17;		// Select item
    constexpr int SD_V_PDMG01 = 0x18;		// Snake damage 1
    constexpr int SD_V_PDMG02 = 0x19;		// Snake damage 2
    constexpr int SD_V_POUT0001 = 0x1A;		// Snake out
    constexpr int SD_W_RIFLE02 = 0x1B;		// PSG1 shot
    constexpr int SD_P_HIKIZU01 = 0x1C;		// Drag corpse
    constexpr int SD_P_KINUZU01 = 0x1D;		// Clothing rustle 1
    constexpr int SD_P_KINUZU02 = 0x1E;		// Clothing rustle 2




    constexpr int SD_S_IDEC02 = 0x22;		// Use item
    constexpr int SD_S_BUZZER01 = 0x23;		// Card key rejection buzzer
    constexpr int SD_I_ZOOM01 = 0x24;		// Binocular zoom
    constexpr int SD_E_ATARU02 = 0x25;		// Strike with gun stock
    constexpr int SD_E_CAP_MO01 = 0x26;		// Remove injection cap
    constexpr int SD_W_RICOCH02 = 0x27;		// Ricochet (weak)
    constexpr int SD_W_RICOCH01 = 0x28;		// Ricochet (strong)
    constexpr int SD_W_EXPLOS01 = 0x29;		// Explosion (normal)
    constexpr int SD_S_SIREN06 = 0x2A;		// Alarm
    constexpr int SD_W_BOUND02 = 0x2B;		// Grenade bounce
    constexpr int SD_W_PIN02 = 0x2C;		// Pull grenade pin
    constexpr int SD_E_FAMAS03 = 0x2D;		// Enemy FAMAS shot
    constexpr int SD_E_MORPHI01 = 0x2E;		// Injection
    constexpr int SD_W_EQUIP03 = 0x2F;		// SOCOM reload
    
    constexpr int SD_W_FAMAS02 = 0x30;		// FAMAS shot
    constexpr int SD_W_C4PUT02 = 0x31;		// Place C4
    constexpr int SD_W_C4SW01 = 0x32;		// C4 switch
    constexpr int SD_E_DOWN03 = 0x33;		// Enemy knocked down
    constexpr int SD_P_PUNCH02 = 0x34;		// Punch hit
    constexpr int SD_P_KICK02 = 0x35;		// Kick hit
    constexpr int SD_E_E_SWIN01 = 0x36;		// Enemy kick swing
    constexpr int SD_P_WALL02 = 0x37;		// Knock on wall
    constexpr int SD_P_P_SWIN02 = 0x38;		// Punch swing
    constexpr int SD_P_K_SWIN02 = 0x39;		// Kick swing
    constexpr int SD_W_CHAF0003 = 0x3A;		// Chaff dispersal
    constexpr int SD_P_UNTEIL01 = 0x3B;		// Elude movement (left)
    constexpr int SD_P_UNTEIR01 = 0x3C;		// Elude movement (right)
    constexpr int SD_E_EEQUIP01 = 0x3D;		// Enemy reload
    constexpr int SD_E_S_DOWN01 = 0x3E;		// Weak knockdown
    constexpr int SD_E_W_DOWN01 = 0x3F;		// Hit wall
    
    constexpr int SD_W_EQUIP04 = 0x40;		// Load M9 round
    constexpr int SD_W_EXPLOS02 = 0x41;		// Explosion (large)
    constexpr int SD_W_EXPLOS03 = 0x42;		// Explosion (chaff or stun)
    constexpr int SD_W_EQUIP05 = 0x43;		// Handgun reload
    constexpr int SD_A_DOORMOV1 = 0x44;		// Door moves
    constexpr int SD_A_DOORCLO1 = 0x45;		// Door closes
    constexpr int SD_A_DOOROPN1 = 0x46;		// Door opens
    constexpr int SD_V_PDMG03 = 0x47;		// Snake damage 3 (fall damage)
    constexpr int SD_W_MAGAZI01 = 0x48;		// Empty magazine falls
    constexpr int SD_E_ECODBR01 = 0x49;		// Enemy radio destroyed
    constexpr int SD_E_FMSSIR01 = 0x4A;		// Enemy silenced rifle
    constexpr int SD_W_EQUDEC01 = 0x4B;		// Handgun decock
    constexpr int SD_W_MISSIL01 = 0x4C;		// Missile launch
    constexpr int SD_W_MISILE03 = 0x4D;		// Missile acceleration
    constexpr int SD_E_HIBANA02 = 0x4E;		// Sparks from destroyed radio
    constexpr int SD_W_SIGNAL01 = 0x4F;		// Missile radar sound
    
    constexpr int SD_V_PDONMU01 = 0x50;		// Snake: "Don't move"
    constexpr int SD_E_HIZA01 = 0x51;		// Enemy kneels
    constexpr int SD_W_SOSIRE01 = 0x52;		// Silenced SOCOM shot
    constexpr int SD_E_BIKKRI01 = 0x53;		// Surprise mark (!)
    constexpr int SD_V_PELUDE01 = 0x54;		// Snake straining during elude
    constexpr int SD_V_PFALL01 = 0x55;		// Snake falling scream
    constexpr int SD_V_PVOMIT01 = 0x56;		// Snake vomiting
    constexpr int SD_A_RICDAN06 = 0x57;		// First-person impact on worn cardboard box
    constexpr int SD_S_R_CALL01 = 0x58;		// Codec call (from other party)
    constexpr int SD_S_R_FACE01 = 0x59;		// Switch character camera
    constexpr int SD_S_R_CANCEL = 0x5A;		// Cancel codec screen
    constexpr int SD_S_R_CURSOR = 0x5B;		// Select codec contact
    constexpr int SD_S_R_DISP01 = 0x5C;		// Codec connection noise
    constexpr int SD_S_R_SND01 = 0x5D;		// Codec send (from player)
    constexpr int SD_E_CAMMOV01 = 0x5E;		// Camera pan 1
    constexpr int SD_E_CAMFND01 = 0x5F;		// Camera focuses
    
    constexpr int SD_S_R_TUNE01 = 0x60;		// Select codec frequency
    constexpr int SD_S_R_WINDW1 = 0x61;		// Open codec character window
    constexpr int SD_S_R_WINDW2 = 0x62;		// Close codec character window
    constexpr int SD_A_CODEC01 = 0x63;		// Enemy codec disconnect noise
    constexpr int SD_S_KAIFAL01 = 0x64;		// Tumble down stairs
    constexpr int SD_W_CASE03 = 0x65;		// Shell casing falls

    constexpr int SD_A_HASIGO1L = 0x67;		// Ladder footstep (left)
    constexpr int SD_A_HASIGO1R = 0x68;		// Ladder footstep (right)
    constexpr int SD_A_MAGAZI03 = 0x69;		// Magazine hits enemy
    constexpr int SD_S_O2DAMAGE = 0x6A;		// Life gauge decreases
    constexpr int SD_V_KACHA01 = 0x6B;		// Enemy clothing rustle during movement 1 (weak)
    constexpr int SD_V_KACHA02 = 0x6C;		// Enemy clothing rustle during movement 2 (weak)
    constexpr int SD_V_KACHA03 = 0x6D;		// Enemy clothing rustle during movement 3 (strong)
    constexpr int SD_V_KACHA04 = 0x6E;		// Enemy clothing rustle during movement 4 (strong)
    constexpr int SD_A_MITCLO01 = 0x6F;		// Close watertight door
    
    constexpr int SD_A_MITNOT01 = 0x70;		// Watertight door will not open
    constexpr int SD_A_MITOPN01 = 0x71;		// Open watertight door
    constexpr int SD_I_MASK01 = 0x72;		// Mask breathing 1, exhale (mask first-person view)
    constexpr int SD_A_MITROL01 = 0x73;		// Turn watertight door handle
    constexpr int SD_W_SIGNAL02 = 0x74;		// Missile fuel depleted warning
    constexpr int SD_E_SHTFRE01 = 0x75;		// Enemy shotgun fire
    constexpr int SD_E_SHTREL01 = 0x76;		// Enemy shotgun pump action
    constexpr int SD_A_LOCKCLO1 = 0x77;		// Locker door closes (clunk)
    constexpr int SD_S_RADAR003 = 0x78;		// Radar jamming (unavailable)
    constexpr int SD_A_LOCKMOV1 = 0x79;		// Locker door moves (creak)
    constexpr int SD_A_LOCKOPN1 = 0x7A;		// Locker door opens (click)
    constexpr int SD_A_LOCKHIT1 = 0x7B;		// Hit head on locker in first-person view
    constexpr int SD_A_LOCKOPN2 = 0x7C;		// Locker door will not open (clank)
    constexpr int SD_A_LOCKDWN1 = 0x7D;		// Locker door comes off
    constexpr int SD_A_LOCKDWN2 = 0x7F;		// Locker door bounces
    
    constexpr int SD_A_LOCKDWN3 = 0x80;		// Locker door falls over
    constexpr int SD_A_LOCKMIL1 = 0x81;		// Knock on chest of locker poster
    constexpr int SD_A_MOROTI01 = 0x82;		// M9 syringe falls
    constexpr int SD_A_LOCKDMG1 = 0x83;		// Locker door creaks and shifts
    constexpr int SD_A_LOCKATK1 = 0x84;		// Attack hits locker door
    constexpr int SD_A_ITEMKIE1 = 0x85;		// Item disappears when time expires
    constexpr int SD_A_M_NINE01 = 0x86;		// M9 syringe sticks in target
    constexpr int SD_V_PKIAI01 = 0x87;		// Snake exertion
    constexpr int SD_P_GUNPNC01 = 0x88;		// Small-arms punch hit
    constexpr int SD_A_DAMBRA01 = 0x89;		// Worn cardboard box breaks



    constexpr int SD_S_R_SEL01 = 0x8D;		// Display codec shortcut window
    constexpr int SD_E_KOFURI01 = 0x8E;		// Enemy hip movement during hold-up
    constexpr int SD_E_EHEART01 = 0x8F;		// Directional microphone heartbeat 1
    
    constexpr int SD_E_EHEART02 = 0x90;		// Directional microphone heartbeat 2
    constexpr int SD_E_EHEART03 = 0x91;		// Directional microphone heartbeat 3
    constexpr int SD_E_EHEART04 = 0x92;		// Directional microphone heartbeat 4
    constexpr int SD_S_MICNOIZE = 0x93;		// Continuous microphone noise (14 fps loop)
    constexpr int SD_S_MICSWING = 0x94;		// Microphone movement noise (7 fps loop)
    constexpr int SD_P_STAND03 = 0x95;		// Cartwheel landing (Raiden)
    constexpr int SD_I_SPRAY01 = 0x96;		// Start coolant spray
    constexpr int SD_I_SPRAY02 = 0x97;		// Coolant spray in progress (call 01 once, then call 02 continuously at 15 fps)
    constexpr int SD_A_FREEZE01 = 0x98;		// Timed C4 freezing, "crackling" (15 fps loop)
    constexpr int SD_A_FREEZE02 = 0x99;		// Timed C4 freezing complete
    constexpr int SD_A_C4LEDBR1 = 0x9A;		// Timed C4 LED blink (call 6 frames before maximum brightness)
    constexpr int SD_S_PHONE001 = 0x9B;		// Cell phone ringing: "Riririri..."
    constexpr int SD_S_PHONE002 = 0x9C;		// Answer cell phone: "Beep" (button press)
    constexpr int SD_S_PHONE000 = 0x9D;		// Mute cell phone (when switching to codec screen)
    constexpr int SD_S_CAM_OK01 = 0x9E;		// Camera shot accepted
    constexpr int SD_S_CAM_NG01 = 0x9F;		// Camera shot rejected
    
    constexpr int SD_S_PHONEMEL = 0xA0;		// Cell phone ringtone (main theme)
    constexpr int SD_A_DOGTAG01 = 0xA1;		// Dog tag falling sound
    constexpr int SD_I_MASK02 = 0xA2;		// Mask breathing 2, inhale (mask first-person view)
    constexpr int SD_E_ABAKAN01 = 0xA3;		// Enemy AN-94 fire
    constexpr int SD_E_M4EGRE01 = 0xA4;		// Enemy M4 grenade launch
    constexpr int SD_E_M4ENEM01 = 0xA5;		// Enemy M4 fire
    constexpr int SD_E_MACALL01 = 0xA6;		// Enemy Makarov fire
    constexpr int SD_W_AKS74U01 = 0xA7;		// AK shot
    constexpr int SD_W_AKSIRE01 = 0xA8;		// Silenced AK shot
    constexpr int SD_W_M_FOUR01 = 0xA9;		// M4 shot
    constexpr int SD_W_SOCSIR12 = 0xAA;		// Silenced SOCOM shot
    constexpr int SD_W_RIFSIR01 = 0xAB;		// PSG1-T shot
    constexpr int SD_W_LANTURE2 = 0xAC;		// RGB-6 shot
    constexpr int SD_W_NIKFIRE1 = 0xAD;		// Nikita missile launch
    constexpr int SD_W_NIKSRCH1 = 0xAE;		// Nikita missile acceleration (first-person view)
    constexpr int SD_W_MINEEXP1 = 0xAF;		// Claymore mine explosion
    
    constexpr int SD_W_MINEPUT1 = 0xB0;		// Place Claymore mine
    constexpr int SD_W_BOOKPUT1 = 0xB1;		// Place adult magazine
    constexpr int SD_I_SENS_B01 = 0xB2;		// Sensor B signal (speeds up as target approaches)

    constexpr int SD_E_CAMMOV02 = 0xB4;		// Camera pan 2
    constexpr int SD_E_CAMMOV03 = 0xB5;		// Camera pan 3
    constexpr int SD_E_CAMMOV04 = 0xB6;		// Camera pan 4
    constexpr int SD_E_CAMCHA01 = 0xB7;		// Camera chaff pan 1
    constexpr int SD_E_CAMCHA02 = 0xB8;		// Camera chaff pan 2
    constexpr int SD_E_CAMCHA03 = 0xB9;		// Camera chaff pan 3
    constexpr int SD_E_CAMCHA04 = 0xBA;		// Camera chaff pan 4
    constexpr int SD_A_RICCOMP1 = 0xBB;		// Bullet impacts console
    constexpr int SD_A_SPARK01 = 0xBC;		// Blue cord-like electrical discharge
    constexpr int SD_P_ELEBTN01 = 0xBD;		// Press central building elevator button
    constexpr int SD_P_ELEMOV01 = 0xBE;		// Central building elevator operating (persistent sound effect) SE_JOUCHUU_OFF
    constexpr int SD_P_ELEOPN01 = 0xBF;		// Central building elevator door moves
    
    constexpr int SD_P_ELECLS01 = 0xC0;		// Central building elevator door closes
    constexpr int SD_P_NKTPNC01 = 0xC1;		// Nikita hit
    constexpr int SD_P_NKTSWI01 = 0xC2;		// Swing Nikita
    constexpr int SD_P_DCTDANSA = 0xC3;		// Climb over duct ledge
    constexpr int SD_A_ELECHM01 = 0xC4;		// Central building elevator arrival chime
    constexpr int SD_A_RICCOMP2 = 0xC5;		// Console damaged
    constexpr int SD_S_R_VTR_FF = 0xC6;		// Codec fast-forward (3 fps loop)
    constexpr int SD_A_ELESTP01 = 0xC7;		// Central building elevator arrives while waiting (arrival at 125 ms / 75 fps)
    constexpr int SD_A_NORD_ON1 = 0xC8;		// Node terminal proximity ON

    constexpr int SD_S_R_FACE02 = 0xCA;		// Switch character camera (player side)
    constexpr int SD_W_EQUIP06 = 0xCB;		// RGB-6 reload 1, insert round
    constexpr int SD_W_EQUIP07 = 0xCC;		// RGB-6 reload 2, close cylinder
    constexpr int SD_I_FNAEAT01 = 0xCD;		// Woodlouse consumes ration item
    constexpr int SD_I_FNAESC01 = 0xCE;		// Item woodlouse escapes
    constexpr int SD_S_COUNTDW1 = 0xCF;		// Timer countdown warning
    
    constexpr int SD_S_COUNTRV1 = 0xD0;		// Timer count-up (loop synchronized with digits)
    constexpr int SD_A_FIRERED1 = 0xD1;		// Fire burning 1 (random 6-16 fps loop)
    constexpr int SD_A_FIRERED2 = 0xD2;		// Fire burning 2 (use for additional simultaneous fires)
    constexpr int SD_A_FIRERED3 = 0xD3;		// Fire burning 3
    constexpr int SD_A_FIRERED4 = 0xD4;		// Fire burning 4
    constexpr int SD_A_FIRESMK1 = 0xD5;		// Fire extinguishes and smoke rises 1 (random 6-16 fps loop)
    constexpr int SD_A_FIRESMK2 = 0xD6;		// Fire extinguishes and smoke rises 2 (use for additional simultaneous effects)
    constexpr int SD_A_FIRESMK3 = 0xD7;		// Fire extinguishes and smoke rises 3
    constexpr int SD_A_FIRESMK4 = 0xD8;		// Fire extinguishes and smoke rises 4
    constexpr int SD_A_FIREBOK1 = 0xD9;		// Magazine or person burning 1 (random 6-16 fps loop)
    constexpr int SD_A_FIREBOK2 = 0xDA;		// Magazine or person burning 2 (use for additional simultaneous effects)
    constexpr int SD_A_FLYFLY01 = 0xDB;		// Fly buzzing
    constexpr int SD_I_SIREEQU1 = 0xDC;		// Attach silencer to SOCOM or AK
    constexpr int SD_P_ELEOPED1 = 0xDD;		// Central building elevator door fully opens
    constexpr int SD_A_EXPLOSW1 = 0xDE;		// Underwater explosion sound
    constexpr int SD_I_EROBFA01 = 0xDF;		// Adult magazine falls while open
    
    constexpr int SD_I_EROBBR01 = 0xE0;		// Bullet impacts adult magazine
    constexpr int SD_I_FNAPET01 = 0xE1;		// Item woodlouse attaches to ration
    
    
    constexpr int SD_S_N_COMP01 = 0xE7;		// Node terminal loading complete
    constexpr int SD_S_N_LOAD01 = 0xE8;		// Node terminal loading (6 fps loop)
    
    constexpr int SD_S_TYPING01 = 0xEA;		// Normal passcode typing sound
    constexpr int SD_S_TYPING02 = 0xEB;		// Final passcode typing sound
    constexpr int SD_S_WINOPNL1 = 0xEC;		// Open window frame (left to right)
    constexpr int SD_S_WINOPNR1 = 0xED;		// Open window frame (right to left)
    constexpr int SD_S_WINCLSL1 = 0xEE;		// Close window frame (left to right)
    constexpr int SD_S_WINCLSR1 = 0xEF;		// Close window frame (right to left)
    
    constexpr int SD_S_B_READY1 = 0xF0;		// Boss Survival: "READY!"
    constexpr int SD_S_B_FIGHT1 = 0xF1;		// Boss Survival: "GO!"
    constexpr int SD_S_SAVEOK01 = 0xF2;		// Save complete jingle
    constexpr int SD_S_N_START1 = 0xF3;		// Node terminal start and finish
    constexpr int SD_S_TYPING03 = 0xF4;		// Switching cursor sound
    constexpr int SD_S_LINEMOV1 = 0xF5;		// Line movement effect sound
    constexpr int SD_S_TWINKY02 = 0xF6;		// "YOU WIN" text blinking
    constexpr int SD_S_V_CANS02 = 0xF7;		// SP mode cancel sound
    constexpr int SD_S_CUR01 = 0xF8;		// Cursor movement (title and briefing)
    constexpr int SD_S_WIN01 = 0xF9;		// Display window (title and briefing)
    constexpr int SD_S_START01 = 0xFA;		// Confirm sound (title and briefing)
    constexpr int SD_S_START001 = 0xFB;		// Game start

    constexpr int SD_S_HEXOUT01 = 0xFD;		// At end of hexagonal fade
    constexpr int SD_S_WINOPN01 = 0xFE;		// Open window frame (from center)
    constexpr int SD_S_WINCLS01 = 0xFF;		// Close window frame (to center)
    
    constexpr int SD_S_HEX_IN01 = 0xB3;		// At start of hexagonal fade
    constexpr int SD_S_GO_TYPE1 = 0xFC;		// Game over text output sound
    
    constexpr int SD_E_BROODSW1 = 0x1F;		// Tengu soldier blood spray
    constexpr int SD_A_SWIGUARD = 0x20;		// Ready sword guard
    constexpr int SD_A_SWINGBAK = 0x21;		// Sword backswing
    constexpr int SD_A_SWINGBIT = 0x66;		// Sword thrust swing
    constexpr int SD_A_SWINGCUT = 0x7E;		// Sword slash swing
    constexpr int SD_A_SWINGROL = 0x8A;		// Sword spinning swing
    constexpr int SD_A_SWORDBIT = 0x8B;		// Sword thrust hit
    constexpr int SD_A_SWORDCLO = 0x8C;		// Sheathe sword
    constexpr int SD_A_SWORDCNG = 0xC9;		// Switch to non-lethal sword strike
    constexpr int SD_A_SWORDCUT = 0xE2;		// Sword slash hit
    constexpr int SD_A_SWORDHAD = 0xE3;		// Sword clash
    constexpr int SD_A_SWORDHIT = 0xE4;		// Non-lethal sword strike hit
    constexpr int SD_A_SWORDRIC = 0xE5;		// Sword deflects bullet
    constexpr int SD_A_SWORDOPN = 0xE6;		// Begin equipping sword
    constexpr int SD_A_SWORDSET = 0xE9;		// Ready sword
    
    constexpr int SD_S_TWINKY01 = 0x5BE;		// "FIGHT!" text blinking
    

}
