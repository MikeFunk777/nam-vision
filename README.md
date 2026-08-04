# NAM Division

[![Build](https://github.com/kylewetton/nam-division/actions/workflows/build-native.yml/badge.svg)](https://github.com/kylewetton/nam-division/actions/workflows/build-native.yml)

![NAM Division](/docs/images/use.gif)

> This plugin is a facelift of the original its based on, the biggest advantages are an easier way of jumping between amps and cabs, and a nice visual reference of the gear you're using.

NAM Division is a VST3/Audio Unit plug-in and standalone application for
[Neural Amp Modeler](https://github.com/sdatkinson/neural-amp-modeler), built
with [iPlug2](https://iplug2.github.io).

This project is a modified fork of
[NeuralAmpModelerPlugin](https://github.com/sdatkinson/NeuralAmpModelerPlugin)
by Steven Atkinson.

## Installation

For pre-built Windows and macOS installers, check
[Releases](https://github.com/kylewetton/nam-division/releases).

## How to Use (in 4 steps)

**This requires a simple but specific folder structure, so this is where we will get started.**

The below folder structure shows how to set up your collection of both NAM files and IR files to be read by the plugin automatically.

### 1.

Keep an `Amps` folder (for all your nams) and a `Cabs` folder (for IRs), these can be named anything you want.

```
my-nam-collection/
│   ├── Amps/ 
│   └── Cabs/
```

### 2.

Inside each, keep your nam collections foldered by amp name, **this folder name is read by the plugin, so make it human readable**, same goes for the cabs.

```
my-nam-collection/
│   ├── Amps/ 
│   │   └── Peavy 6505/
│   │       └── red-channel-III.nam
│   │       └── red-channel-IV.nam            
│   └── Cabs/
│       └── Marshall 1960A/
│           └── 57-off-centre.wav
│           └── 57-centre.wav
```
You can have sub-folders inside an amp or cab, you will be prompted to select one or you can select `load all`. Note that loading a large batch of IRs seems to be as fast as loading one, but loading a large collection of nams seems to slow down the UI slightly.

```
my-nam-collection/
│   ├── Amps/ 
│   │   └── Peavy 6505/
│   │       └── Green channel/
│   │       └── Red channel/
│   └── Cabs/
│       └── Marshall 1960A/
│           └── 57-off-centre.wav
│           └── 57-centre.wav
```

### 3.

Finally, use the image template in the next section to save a thumbnail for that amp. This thumbnail must be a .jpg, you can name it whatever you want. 

Here's an example:

```
my-nam-collection/
│   ├── Amps/ 
│   │   └── Peavy 6505/
│   │       └── red-channel-III.nam
│   │       └── red-channel-IV.nam
│   │       └── peavy-thumbnail.jpg
│   └── Cabs/
│       └── Marshall 1960A/
│           └── 57-off-centre.wav
│           └── 57-centre.wav
│           └── marshall-cab.jpg
```

**Download the latest release and find the "nam-division-collection" folder for an example nam and IR to get started along with the folder structure.**

In the example folder, you'll find a selection of nam captures from here

https://www.tone3000.com/tones/6505-full-pack-70977

and IRs from here

https://www.tone3000.com/tones/marshall-1960bv-v30-and-g12t75-51086

### 4.

Once you've launched the Plugin for the first time, head to the settings page in the top right corner and point the plugin to your `Amps` folder and your `Cabs` folder. You'll only need to do this once. 

![Set folders](/docs/images/set-folders.png)

Then head back and click on a head or cab, it will bring up your gallery of amps.

![Amp gallery](/docs/images/amp-gallery.png)

![Cab gallery](/docs/images/cab-gallery.png)

## Thumbnail templates

The plugin really shines when the thumbnails are set up correct. There are only two and the easiest way is to use the template to position the gear correctly.

These templates are both available in the release package.

**Do your best to isolate the amp and cab onto a white background, find the best front on photo that serves this purpose.**

### Amp template

First notice the two different shades of green.

**For amp heads**, position the head so that it's 'sitting on' the bottom of the dark green.

**For combo amps**, position it centred in the entire green area.

<img src="docs/images/amp-template.jpg" alt="Amp template" width="640">

### Cab template

Use the green box as a bounding area, position the cab/amp-as-cab so that it's using the bottom of the box as the floor. Notice the template shows a 4x12 and a 2x12 as examples.

<img src="docs/images/cab-template.jpg" alt="Cab template" width="640">


## Supported Platforms

NAM Division currently targets Windows 10 (64-bit) or later and macOS 10.15
(Catalina) or later.

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
