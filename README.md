# Metal Gear Solid Master Collection Fix
[![Releases](https://img.shields.io/github/v/release/Lyall/MGSHDFix)](https://github.com/Lyall/MGSHDFix/releases) [![Downloads](https://img.shields.io/github/downloads/Lyall/MGSHDFix/total)](https://github.com/Lyall/MGSHDFix/releases) ![Commits](https://img.shields.io/github/commit-activity/t/Lyall/MGSHDFix) ![License](https://img.shields.io/github/license/Lyall/MGSHDFix)


[MG1 / MG2 Nexus Page](https://www.nexusmods.com/metalgearandmetalgear2mc/mods/9) | [MGS2 Nexus Page](https://www.nexusmods.com/metalgearsolid2mc/mods/49) | [MGS3 Nexus Page](https://www.nexusmods.com/metalgearsolid3mc/mods/139) | **GitHub Repo (You're already here!)**<br />

This is a fix that adds custom resolutions, ultrawide support and much more to the Metal Gear Solid Master Collection.<br />

**Featured by**:  
[IGN (Video Guide)](https://www.ign.com/videos/how-to-fix-the-metal-gear-solid-master-collection-on-pc-with-mods) • [IGN (Best Mods List)](https://www.ign.com/wikis/metal-gear-solid-master-collection-vol-1/Best_Mods) • [Digital Foundry / Eurogamer](https://youtu.be/zkdxOQ2kGMc?t=536) • [PC Gamer](https://www.pcgamer.com/it-only-took-hours-for-modders-to-crowbar-4k-support-into-the-metal-gear-solid-master-collectionnow-theyve-added-ultrawide-high-res-ui-support-and-more/) • [Rock Paper Shotgun](https://www.rockpapershotgun.com/modders-polish-metal-gear-solids-pc-master-collection-with-ultrawide-support-sharper-textures-and-more) • [Ocelot (YouTube)](https://www.youtube.com/watch?v=CwgWJgc58_4) • [GamingOnLinux](https://www.gamingonlinux.com/2023/11/modders-already-improving-the-metal-gear-solid-master-collection/) • [Dextero](https://www.dexerto.com/tech/metal-gear-solid-master-collection-pc-modders-are-fixing-konamis-mistakes-2380637/)


## Games Supported
- Metal Gear 1/2 (MSX)
- Metal Gear Solid 2
- Metal Gear Solid 3

## Other Metal Gear Fix Projects
- MGS Master Collection - Metal Gear Solid 1 and Bonus Content (MG1/2 NES) | MGSM2Fix - [Repo](https://github.com/nuggslet/MGSM2Fix) / [Nexus Page](https://www.nexusmods.com/metalgearsolidmc/mods/5)
- Metal Gear Solid V: The Phantom Pain | MGSVFix - [Repo](https://codeberg.org/Lyall/MGSVFix)
- Metal Gear Solid Delta: Snake Eater | MGSDeltaFix - [Repo](https://codeberg.org/Lyall/MGSDeltaFix) / [Nexus Page](https://www.nexusmods.com/metalgearsoliddeltasnakeeater/mods/27)

## Features
- Custom resolution/ultrawide support.
- Experimental 16:9 HUD option that resizes HUD/movies (MGS2/MGS3).
- Borderless/windowed mode.
- Mouse cursor toggle.
- Mouse sensitivity adjustment (MGS3).
- Launcher skips (see Config Tool to configure).
- Skip intro logos (MGS2/MGS3).
- Option to disable pausing on alt-tab.
- Option to force the game to output stereo audio, which corrects the infamous ["rain is louder than codec conversations"](https://www.pcgamingwiki.com/wiki/Metal_Gear_Solid_2:_Sons_of_Liberty_-_Master_Collection_Version#Rain_audio_is_significantly_louder_than_codec_conversations_.26_other_game_sounds) issue. [PR #162](https://github.com/Lyall/MGSHDFix/pull/162)
- Adjustable anisotropic filtering (MGS2/MGS3).
- Option to disable bilinear texture filtering, giving the games a pixel art/retro appearance. [PR #138](https://github.com/Lyall/MGSHDFix/pull/138)
- Increased texture size limits (MG1/MG2/MGS3).
- Adds support for custom PS2 controller glyphs without overwriting existing textures.
- Option to force Snake / Raiden to wear their sunglasses (and outright disable their sunglasses.)
- Option to continue aiming your gun after firing it while in first-person/while holding lock-on.
- Toggleable wireframe modes.
- Many more!

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
- Fixes UI scaling. [PR #181](github.com/Lyall/MGSHDFix/pull/181)
- Fixes typos in several Snake Tales missions, and in the in-game novel "In The Darkness of Shadow Moses". [PR#201](https://github.com/Lyall/MGSHDFix/pull/201)
- Many more!

## Logging / Warnings for Common Configuration Issues
- Warnings for common mod compatibility & installation issues - which often result in crashes.
- Warnings if your game's audio is muted via the game's main launcher.
- Logging for Steam Input's controller status (ie detected controllers, keybinds, ect.)


## Installation

🚩 **If updating from a previous version of MGSHDFix:**
- Delete `d3d11.dll` from your game folder.
- Delete old MGSHDFix files (e.g., `MGSHDFix Config Tool.exe` and `MGSHDFix.asi`) before installing the update.

### Steps:
1. Grab the latest release of MGSHDFix from [here.](https://github.com/Lyall/MGSHDFix/releases)
2. Extract the contents of the release zip into your game folder.
   - (e.g., `steamapps\common\MGS2` or `steamapps\common\MGS3` for Steam.)
3. Set both "Internal Resolution" & "Internal Upscaling" to Default / Original in the game's launcher. (Resolution is entirely handled by MGSHDFix.)
4. Launch the MGSHDFix Config Tool (in the game's /plugins folder) to generate a settings file if you're installing the mod for the first time.

### Steam Deck/Linux Additional Instructions

**🚩 These steps are only needed if you’re on Steam Deck/Linux. Skip if you’re using Windows.**

- Open up the game properties of either MGS2/MGS3 in Steam and add the following line to the launch options:

      WINEDLLOVERRIDES="wininet,winhttp=n,b" %command%
	   
- MGSHDFix's Config Tool requires **ProtonTricks** to be installed via Linux's **Discover** software store.
- When opening the MGSHDFix Config Tool on Steam Deck/Linux, a Proton Tricks Wine Prefix window will pop up. Select any game and hit "OK" to open the MGSHDFix Config Tool.
   - If you do not have any games in the list, or the MGSHDFix Config Tool fails to launch, add it as a non-steam game and launch it once through Steam to generate a new Proton Tricks Wine Prefix entry.
   - You can remove the Config Tool from your Steam game list and launch it directly after generating this prefix.
   

### Configuration

- See **MGSHDFix Config Tool.exe** in the `/plugins` folder to adjust settings for the fix.

## Support
Please report any issues you notice on our Github [here](https://github.com/Lyall/MGSHDFix/issues/new/choose).

For more immediate problems, you can contact us in the [#HDFix](https://discord.gg/bFv9bZmWDV) channel of the Metal Gear Network Discord.

## Known Issues
This list will contain bugs which may or may not be fixed.

### MGS 2
- Strength of post-processing may be reduced at higher resolutions. ([#35](https://github.com/Lyall/MGSHDFix/issues/35))
- Various visual issues when using the experimental HUD fix. ([#41](https://github.com/Lyall/MGSHDFix/issues/41))

### MGS 3
- Strength of post-processing may be reduced at higher resolutions. ([#35](https://github.com/Lyall/MGSHDFix/issues/35))
- Various visual issues when using the experimental HUD fix. ([#41](https://github.com/Lyall/MGSHDFix/issues/41))

### MGS Master Collection - Community Bug Tracker
- A detailed tracker which catalogs all of the known Master Collection bugs (including issues fixed by MGSHDFix) can be located [here](https://docs.google.com/spreadsheets/d/1WhQSRpkC_A9wBDV0o-Pohh1dMhL1H6nbVzvdluIVWrw/edit?gid=0#gid=0).
- To submit new entries to the tracker, either report a new issue on the MGSHDFix [Github](https://github.com/Lyall/MGSHDFix/issues/new/choose), or use [this form](https://docs.google.com/forms/d/e/1FAIpQLSef8Vx38tHpBsR-dXnawF6X0iad3XU7vmDX29pcmjbaZhQiew/viewform).

## Examples

| ![MGS2 widescreen cutscene preview](screenshots/after/mgs2%20-%20widescreen.gif) |
|:--:|

| Unmodded Metal Gear Solid 2                                                                                                       | MGSHDFix                                                                                                                         |
| --------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs2%20-%20d00t%20-%20rain%201.png" />         | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs2%20-%20d00t%20-%20rain%201.png" />         |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs2%20-%20d05t%20-%20rain%20(olga).png" />    | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs2%20-%20d05t%20-%20rain%20(olga).png" />    |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs2%20-%20w24a%20-%20solidus%20flames%202.png" /> | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs2%20-%20w24a%20-%20solidus%20flames%202.png" /> |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs2%20-%20w32a%20-%20scope.png" />            | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs2%20-%20w32a%20-%20scope.png" />            |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs2%20-%20codec.png" />                       | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs2%20-%20codec.png" />                       |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs2%20-%20w00a%20-%20aiming.png" />            | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs2%20-%20w00a%20-%20aiming.png" />            |
| Unmodded Metal Gear Solid 2                                                                                                       | MGSHDFix                                                                                                                         |

| ![MGS3 widescreen gameplay preview](screenshots/after/mgs3%20-%20widescreen.gif) |
|:--:|

| Unmodded Metal Gear Solid 3                                                                                                       | MGSHDFix                                                                                                                         |
| --------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs3%20-%20thermals.png" />            | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs3%20-%20thermals.png" />            |
| <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs3%20-%20radio%20menu.png" />            | <img width="3840" height="2160" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs3%20-%20radio%20menu.png" />            |
| <img width="389" height="219" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs3%20-%20wig%20reflection.gif" />         | <img width="389" height="219" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs3%20-%20wig%20reflection.gif" />         |
| <img width="389" height="291" alt="mgs2 - d00t - rain 1" src="screenshots/before/mgs3%20-%20river%20reflection.jpg" />                       | <img width="389" height="291" alt="mgs2 - d00t - rain 1" src="screenshots/after/mgs3%20-%20river%20reflection.jpg" />                       |
| Unmodded Metal Gear Solid 3                                                                                                       | MGSHDFix                                                                                                                         |

![MGS3 - Correctly scaled rain on bridge scene](screenshots/after/mgs3%20-%20bridge%20rain.png)
![MGS3 - Correctly scaled rain in The Sorrow's river](screenshots/after/mgs3%20-%20sorrow%20rain%202.png)
![MGS3 - Wireframe mode visual](screenshots/after/mgs3%20-%20wireframe.png)

## Upcoming Fix/Feature Roadmap - (Version Problem Originated)
- MG1 / MG2 - Add Custom Loading Screen Support (2023 MC)
- MG1 / MG2 - Crop Screen Borders (2011 HDC)
- MGS2 - Fix Broken Cutscene Color Filters (2002 Xbox)
- MGS2 - Make the in-game Radar, Cutscene Letterboxing, and Previous Missions reading progress persistent across game sessions. (2001 SoL)
- MGS3 - Fix Cutscene Camera Offset (2011 HDC)
- MGS3 - Fix Angle of Attack Indicator in FPV with NVG & Thermals (2011 HDC)
- MGS3 - Fix Weapons Not Appearing in Holster After Torture (2004 Snake Eater)
- MGS2 / MGS3 - Add Custom Anti-Aliasing Solution (2023 MC)
- MGS2 / MGS3 - Correct Display Gamma & RGB Levels (2011 HDC)
- MGS2 / MGS3 - Correct More Sped Up Effects (2002 Xbox / 2011 HDC)
- MGS2 / MGS3 - Fix Depth of Field Scaling Strength (2002 Xbox / 2011 HDC)
- MGS2 / MGS3 - Swap X/O Buttons on Controller in Menus (2011 HDC)

## Building
```bash
git clone --recursive https://github.com/Lyall/MGSHDFix.git
cd MGSHDFix
```

### Windows
Open MGSFPSUnlock.sln in Visual Studio (2022) and build.

## Credits
[@Lyall](https://codeberg.org/Lyall) for their amazing work making widescreen fix mods, and most importantly, the original creation of this mod!<br />
[@ShizCalev/Afevis](https://github.com/shizcalev) for long-term maintenance (taking over the project in early 2025), and contributing fixes.<br />
[@emoose](https://github.com/emoose), [@cipherxof](https://github.com/cipherxof), [@Bud11](https://github.com/bud11), and [Zenf0](https://next.nexusmods.com/profile/zenf0) for contributing fixes/features. <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
Universal Config Tool (made by ShizCalev/Afevis).
