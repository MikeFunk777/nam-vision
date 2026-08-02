# NAM Division

[![Build](https://github.com/kylewetton/nam-division/actions/workflows/build-native.yml/badge.svg)](https://github.com/kylewetton/nam-division/actions/workflows/build-native.yml)

NAM Division is a VST3/Audio Unit plug-in and standalone application for
[Neural Amp Modeler](https://github.com/sdatkinson/neural-amp-modeler), built
with [iPlug2](https://iplug2.github.io).

This project is a modified fork of
[NeuralAmpModelerPlugin](https://github.com/sdatkinson/NeuralAmpModelerPlugin)
by Steven Atkinson.

## Installation

Pre-built Windows and macOS installers will be available from
[Releases](https://github.com/kylewetton/nam-division/releases). No official
release has been published yet.

## Supported Platforms

NAM Division currently targets Windows 10 (64-bit) or later and macOS 10.15
(Catalina) or later.

For Linux support, there is an LV2 plugin available: https://github.com/mikeoliphant/neural-amp-modeler-lv2.

## About

NAM Division retains the original NeuralAmpModelerPlugin commit history and
copyright attribution. See [LICENSE](LICENSE) and
[ThirdPartyNotices.txt](NeuralAmpModeler/installer/ThirdPartyNotices.txt) for
license details.

The upstream project began as a cleaned-up version of the original iPlug2-based
NAM plug-in, with refactoring based on recommendations from the iPlug2
developers.

## Rough edges

### Standalone I/O
The I/O for the standalone doesn't inherit the stability of most plugin hosts (DAWs), so it's a bit sparser on features. For complex routing, the plugin (VST3/AU) inside a plugin host is still the most reliable option.

### Graphics backend
If you're having trouble with NAM crashing before the GUI comes up, then you might have an unsupported graphics configuration. Usually, this is when you have a dedicated graphics card (like an nVIDIA GPU) and you're using the integrated (CPU) graphics on a Windows system. To fix this, Go to the control panel, pick NAM (or your DAW), and make sure that it uses your graphics card. (If you know more and can help fix this, please make an Issue and let me know more!)
