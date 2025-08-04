# Metal Gear Solid Master Collection Fix
[![Releases](https://img.shields.io/github/v/release/Lyall/MGSHDFix)](https://github.com/Lyall/MGSHDFix/releases) [![Downloads](https://img.shields.io/github/downloads/Lyall/MGSHDFix/total)](https://github.com/Lyall/MGSHDFix/releases) ![Commits](https://img.shields.io/github/commit-activity/t/Lyall/MGSHDFix) ![License](https://img.shields.io/github/license/Lyall/MGSHDFix)


[MG1 / MG2 Nexus Page](https://www.nexusmods.com/metalgearandmetalgear2mc/mods/9) | [MGS2 Nexus Page](https://www.nexusmods.com/metalgearsolid2mc/mods/49) | [MGS3 Nexus Page](https://www.nexusmods.com/metalgearsolid3mc/mods/139) | **GitHub Repo (You're already here!)**<br />

This is a fix that adds custom resolutions, ultrawide support and much more to the Metal Gear Solid Master Collection.<br />

## Games Supported
- Metal Gear 1/2 (MSX)
- Metal Gear Solid 2
- Metal Gear Solid 3

## Metal Gear Solid 1 / Metal Gear 1/2 (NES)
- For Metal Gear Solid 1 and the Vol 1. Bonus Content (MG1/2 NES), using [MGSM2Fix](https://github.com/nuggslet/MGSM2Fix) is recommended.

## Features
- Custom resolution/ultrawide support.
- Experimental 16:9 HUD option that resizes HUD/movies (MGS2/MGS3).
- Borderless/windowed mode.
- Mouse cursor toggle.
- Mouse sensitivity adjustment (MGS3).
- Launcher skips (see ini to configure).
- Skip intro logos (MGS2/MGS3).
- Option to disable pausing on alt-tab.
- Option to force the game to output stereo audio, which corrects the infamous ["rain is louder than codec conversations"](https://www.pcgamingwiki.com/wiki/Metal_Gear_Solid_2:_Sons_of_Liberty_-_Master_Collection_Version#Rain_audio_is_significantly_louder_than_codec_conversations_.26_other_game_sounds) issue. [PR #162](https://github.com/Lyall/MGSHDFix/pull/162)
- Adjustable anisotropic filtering (MGS2/MGS3).
- Option to disable bilinear texture filtering, giving the games a pixel art/retro appearance. [PR #138](https://github.com/Lyall/MGSHDFix/pull/138)
- Increased texture size limits (MG1/MG2/MGS3).
- Adds support for custom PS2 controller glyphs without overwriting existing textures.
- Option to force Snake / Raiden to wear their sunglasses (and outright disable their sunglasses.)
- Option to continue aiming your gun after firing it while in first-person/while holding lock-on.

## Bug Fixes
- Fixes the collection's games sometimes defaulting to intergrated graphics processors on systems with multiple GPUs (due to Nvidia/AMD driver misconfiguration.)
- Fixes gameplay/cutscene aspect ratio for ultrawide resolutions (MGS2/MGS3).
- Fixes window size on displays with High DPI scaling enabled. [PR #127](https://github.com/Lyall/MGSHDFix/pull/127)
- Fixes the monitor going to sleep during long cutscenes (for Windows only, Linux needs to be [fixed by Valve](https://github.com/ValveSoftware/Proton/issues/8881).
- Fixes the Steam Cloud related ["DAMAGED SAVE" / "CORRUPT SAVE"](https://www.pcgamingwiki.com/wiki/Metal_Gear_Solid_2:_Sons_of_Liberty_-_Master_Collection_Version#Save_File_Appears_as_DAMAGED_FILE) issue. 
- Fixes water surface rendering (MGS3). See [PR #71](https://github.com/Lyall/MGSHDFix/pull/71) for a breakdown of the issue.
- Fixes crashes, audio desync, timer delays, and broken loading zones bugs caused by alt-tabbing the game. (For speedrunners who utilize this bug to skip forced codec calls, this bugfix can be forced off in the ini.)
- Fixes the bug where your character would start aiming right away after re-equipping a gun that was drawn when you put it away. 
- Fixes the bug where your character would stop aiming their gun while holding L1 when you fully tilt your joystick.
- Fixes various visual effects which ran at double speed, causing them to end early compared to on the PS2 (these issue even occur on PCSX2/PS2 emulation) (MGS2).
- Fixes vector effects / line based rendering scaling (ie rain, lasers, bullet trails.) [PR #140](https://github.com/Lyall/MGSHDFix/pull/140)
- Fixes broken skybox initialization procs (MGS2). [PR#142](https://github.com/Lyall/MGSHDFix/pull/142)

## Logging / Warnings for Common Configuration Issues
- Warnings for common mod compatibility & installation issues - which often result in crashes.
- Warnings if your game's audio is muted via the game's main launcher.
- Logging for Steam Input's controller status (ie detected controllers, keybinds, ect.)


## Installation

🚩 **If updating from a previous version of MGSHDFix:**
- Delete `d3d11.dll` from your game folder.
- Delete old MGSHDFix files (e.g., `MGSHDFix.ini` and `MGSHDFix.asi`) before installing the update.

### Steps:
1. Grab the latest release of MGSHDFix from [here.](https://github.com/Lyall/MGSHDFix/releases)
2. Extract the contents of the release zip into your game folder.
   - (e.g., `steamapps\common\MGS2` or `steamapps\common\MGS3` for Steam.)
3. Set both "Internal Resolution" & "Internal Upscaling" to Default / Original in the game's launcher. (Resolution is entirely handled by MGSHDFix.)

### Steam Deck/Linux Additional Instructions

**🚩 These steps are only needed if you’re on Steam Deck/Linux. Skip if you’re using Windows.**

- Open up the game properties of either MGS2/MGS3 in Steam and add the following line to the launch options:

       `WINEDLLOVERRIDES="wininet,winhttp=n,b" %command%` 

### Configuration

- See **MGSHDFix.ini** in the `/plugins` folder to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

### MGS 2
- Strength of post-processing may be reduced at higher resolutions. ([#35](https://github.com/Lyall/MGSHDFix/issues/35))
- Various visual issues when using the experimental HUD fix. ([#41](https://github.com/Lyall/MGSHDFix/issues/41))

### MGS 3
- Strength of post-processing may be reduced at higher resolutions. ([#35](https://github.com/Lyall/MGSHDFix/issues/35))
- Various visual issues when using the experimental HUD fix. ([#41](https://github.com/Lyall/MGSHDFix/issues/41))

## Screenshots

| ![ezgif-3-82fd6eedda](https://github.com/Lyall/MGSHDFix/assets/695941/b01453c7-b4ee-4903-bd34-340371873ecb) |
|:--:|
![2131640_20250428235619_1](https://github.com/user-attachments/assets/610b1318-5d8c-4c36-ada2-3c883f38b33b)
![2131640_20250428225953_1](https://github.com/user-attachments/assets/2bf8fbdf-e274-4454-83c3-1a8c82f5c11c)
![2131640_20250501025301_1](https://github.com/user-attachments/assets/7872b59e-27e9-4e29-a19f-9f468007f9d0)
![2131640_20250501025407_1](https://github.com/user-attachments/assets/7dd0b26c-915d-4549-9f74-03346ecea812)
| Metal Gear Solid 2 |

| ![ezgif-3-982e93f49a](https://github.com/Lyall/MGSHDFix/assets/695941/5530a42e-6b6a-4eb0-a714-ba3e7c3a1dc3) |
|:--:|
![2131650_20250429005837_1](https://github.com/user-attachments/assets/b5ea0b75-3944-422e-a0c9-0c542d22a55c)
![vlcsnap-2025-04-30-01h48m54s182](https://github.com/user-attachments/assets/725a0b36-1862-4e91-b524-82aff39616ef)
| Metal Gear Solid 3 |

## Building
```bash
git clone --recursive https://github.com/Lyall/MGSHDFix.git
cd MGSHDFix
```

### Windows
Open MGSFPSUnlock.sln in Visual Studio (2022) and build.

## Credits
[@Lyall](https://codeberg.org/Lyall) for their amazing work making widescreen fix mods, and most importantly, the original creation of this mod!<br />
[@ShizCalev/Afevis](https://github.com/shizcalev) for long-term maintenance and contributing fixes.<br />
[@emoose](https://github.com/emoose), [@cipherxof](https://github.com/cipherxof), [@Bud11](https://github.com/bud11), and [Zenf0](https://next.nexusmods.com/profile/zenf0) for contributing fixes/features. <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
