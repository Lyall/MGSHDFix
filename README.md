# Metal Gear Solid Master Collection Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)</br>
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/MGSHDFix/total.svg)](https://github.com/Lyall/MGSHDFix/releases)

This is a fix that adds custom resolutions, ultrawide support and much more to the Metal Gear Solid Master Collection.<br />

## Games Supported
- Metal Gear 1/2
- Metal Gear Solid 2
- Metal Gear Solid 3

## Metal Gear Solid 1 (Unsupported)
- For Metal Gear Solid 1, using [MGSM2Fix](https://github.com/nuggslet/MGSM2Fix) is recommended.

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
- Increased texture size limits. (MG1/MG2/MGS3)

## Installation
- Grab the latest release of MGSHDFix from [here.](https://github.com/Lyall/MGSHDFix/releases)
- Extract the contents of the release zip in to the the game folder.<br />(e.g. "**steamapps\common\MGS2**" or "**steamapps\common\MGS3**" for Steam).

### Steam Deck/Linux additional instructions
Steam Deck users can also enjoy a native 800p (16:10) experience by installing this mod.
- Open up the Steam properties of either MGS2/MGS3 and put `WINEDLLOVERRIDES="d3d11=n,b" %command%` in the launch options.
- If you're using the missing audio workaround put `WINEDLLOVERRIDES="xaudio2_9=n;d3d11=n,b" %command%` instead.

## Configuration
- See **MGSHDFix.ini** to adjust settings for the fix.

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
| Metal Gear Solid 2 |

| ![ezgif-3-982e93f49a](https://github.com/Lyall/MGSHDFix/assets/695941/5530a42e-6b6a-4eb0-a714-ba3e7c3a1dc3) |
|:--:|
| Metal Gear Solid 3 |

## Credits
[@emoose](https://github.com/emoose) for contributing fixes. <br />
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[Loguru](https://github.com/emilk/loguru) for logging. <br />
[length-disassembler](https://github.com/Nomade040/length-disassembler) for length disassembly.
