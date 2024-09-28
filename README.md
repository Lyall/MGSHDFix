# Metal Gear Solid Master Collection Fix
[![Patreon-Button](https://github.com/Lyall/FISTFix/assets/695941/19c468ac-52af-4790-b4eb-5187c06af949)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/MGSHDFix/total.svg)](https://github.com/Lyall/MGSHDFix/releases)

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
- Correct gameplay/cutscene aspect ratio (MGS2/MGS3).
- Launcher skip (see ini to configure).
- Skip intro logos (MGS2/MGS3).
- Adjustable anisotropic filtering (MGS2/MGS3).
- Increased texture size limits (MG1/MG2/MGS3).
- Fixed water surface rendering (MGS3). See [PR #71](https://github.com/Lyall/MGSHDFix/pull/71) for a breakdown of the issue.

## Installation
- Grab the latest release of MGSHDFix from [here.](https://github.com/Lyall/MGSHDFix/releases)
- Extract the contents of the release zip in to the the game folder.<br />(e.g. "**steamapps\common\MGS2**" or "**steamapps\common\MGS3**" for Steam).

### Steam Deck/Linux additional instructions
Steam Deck users can also enjoy a native 800p (16:10) experience by installing this mod.
- Open up the Steam properties of either MGS2/MGS3 and put `WINEDLLOVERRIDES="wininet,winhttp=n,b" %command%` in the launch options.

## Configuration
- See **MGSHDFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

### MGS 2
- Strength of post-processing may be reduced at higher resolutions. ([#35](https://github.com/Lyall/MGSHDFix/issues/35))
- Various visual issues when using the experimental HUD fix. ([#41](https://github.com/Lyall/MGSHDFix/issues/41))
- Prerendered camera pictures are black. ([#60](https://github.com/Lyall/MGSHDFix/issues/60))
- Vector based graphic effects (such as rain) do not get scaled up at higher resolutions. ([#90](https://github.com/Lyall/MGSHDFix/issues/90) & [#96](https://github.com/Lyall/MGSHDFix/issues/96))

### MGS 3
- Strength of post-processing may be reduced at higher resolutions. ([#35](https://github.com/Lyall/MGSHDFix/issues/35))
- Various visual issues when using the experimental HUD fix. ([#41](https://github.com/Lyall/MGSHDFix/issues/41))
- Vector based graphic effects (such as rain) do not get scaled up at higher resolutions. ([#90](https://github.com/Lyall/MGSHDFix/issues/90) & [#96](https://github.com/Lyall/MGSHDFix/issues/96))
  
## Screenshots

| ![ezgif-3-82fd6eedda](https://github.com/Lyall/MGSHDFix/assets/695941/b01453c7-b4ee-4903-bd34-340371873ecb) |
|:--:|
| Metal Gear Solid 2 |

| ![ezgif-3-982e93f49a](https://github.com/Lyall/MGSHDFix/assets/695941/5530a42e-6b6a-4eb0-a714-ba3e7c3a1dc3) |
|:--:|
| Metal Gear Solid 3 |

## Credits
[@emoose](https://github.com/emoose) & [@cipherxof](https://github.com/cipherxof) for contributing fixes. <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
