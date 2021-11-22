# `gen~` Plugin Export

This is an example project that should help you get started with building your own VST3 plugins and iOS applications with the help of the code-export feature of `gen~`, part of [Max 8](https://cycling74.com/max7/) made by [Cycling '74](https://cycling74.com/).

It is based on the JUCE Framework. Please be aware that the JUCE has its own license terms (mostly GPL with the availability of commercial licenses). See their [website](http://www.juce.com/) for further details.

## File structure

Most of the code is located within the `misc/` directory. So that the launch patcher is available from the `Extras` dropdown in Max, it lives in the `extras/` directory.

Some notable files/directories:

| Location | Explanation |
| ------------ | ------------- |
| extras/GenPluginExport.maxpat			| main Max patcher to automate building plugins |
| misc/exported-code/					| the folder where gen~ will export C++ code |
| misc/CoreAudioUtilityClasses/		| required for building Audio Units |
| misc/Source-App/					| Source for iOS Application - feel free to edit (includes sample UI) |
| misc/Source-Plugin/				| Source for Audio Plugins - feel free to edit |
| misc/JUCE/					        | The JUCE framework - do not edit these |


### Build locations
| Location | Explanation |
| ------------ | ------------- |
| misc/build/App-Builds/					| iOS projects |
| misc/build/AU-Builds/ 					| AudioIUnit projects |
| misc/build/VST3-Builds/ 				| VST3 projects |
---

## Prerequisites

- Download and install [CMake](https://cmake.org/download/). Version 3.18 or higher is required.
- (MacOS) Download and install [Xcode](https://developer.apple.com/xcode/resources/). We have tested using Xcode 12.
- (Windows) Download and install [Visual Studio 2019](https://visualstudio.microsoft.com/vs/). Community Edition is enough!


## How to use

Everything you need to build a plugin is outlined in `GenPluginExport.maxpat`.

In short here are the steps:
1. Begin with a `gen~` patcher. **In the inspector, change the export name attribute to `C74_GENPLUGIN`.**
2. Export the C++ code from `gen~` using the `codeexport` message. Export to the `misc/exported-code` directory of this package. You should eventually see 
   - `misc/exported-code/gen_dsp/`
   - `misc/exported-code/C74_GENPLUGIN.cpp`
   - `misc/exported-code/C74_GENPLUGIN.h`
3. Open a Terminal (MacOS) or PowerShell (Windows) window. Run following command to move to the `misc/build` directory:
   - `cd && cd "Documents/Max 8/Packages/gen~ Plugin Export/misc/build`
   - (This assumes that this package is installed at `Documents/Max 8/Packages/gen~ Plugin Export/`)
4. In the same Terminal/PowerShell window, generate the CMake project with `cmake ..`
5. Again, in the same window, build the plugin with `cmake --build --config Release .`

## Customization

Plugin building is based on the [JUCE Framework](http://www.juce.com/). Please refer to tutorials from JUCE on building UIs, for instance.

- If you want to add or remove files from your Xcode or Visual Studio project, do not edit the projects directly (they will be rebuilt on each export), but use the Projucer files in the root folder.

Enjoy!