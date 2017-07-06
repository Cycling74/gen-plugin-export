/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "../jucer_Headers.h"
#include "jucer_StoredSettings.h"
#include "../Application/jucer_Application.h"
#include "../Application/jucer_GlobalPreferences.h"

//==============================================================================
StoredSettings& getAppSettings()
{
    return *ProjucerApplication::getApp().settings;
}

PropertiesFile& getGlobalProperties()
{
    return getAppSettings().getGlobalProperties();
}

//==============================================================================
StoredSettings::StoredSettings()
    : appearance (true), projectDefaults ("PROJECT_DEFAULT_SETTINGS")
{
    updateOldProjectSettingsFiles();
    reload();
    projectDefaults.addListener (this);
}

StoredSettings::~StoredSettings()
{
    projectDefaults.removeListener (this);
    flush();
}

PropertiesFile& StoredSettings::getGlobalProperties()
{
    return *propertyFiles.getUnchecked (0);
}

static PropertiesFile* createPropsFile (const String& filename, bool isProjectSettings)
{
    return new PropertiesFile (ProjucerApplication::getApp()
                                .getPropertyFileOptionsFor (filename, isProjectSettings));
}

PropertiesFile& StoredSettings::getProjectProperties (const String& projectUID)
{
    const auto filename = String ("Projucer_Project_" + projectUID);

    for (auto i = propertyFiles.size(); --i >= 0;)
    {
        auto* const props = propertyFiles.getUnchecked(i);
        if (props->getFile().getFileNameWithoutExtension() == filename)
            return *props;
    }

    auto* p = createPropsFile (filename, true);
    propertyFiles.add (p);
    return *p;
}

void StoredSettings::updateGlobalPreferences()
{
    // update 'invisible' global settings
    updateRecentFiles();
    updateKeyMappings();
}

void StoredSettings::updateRecentFiles()
{
    getGlobalProperties().setValue ("recentFiles", recentFiles.toString());
}

void StoredSettings::updateKeyMappings()
{
    getGlobalProperties().removeValue ("keyMappings");

    if (auto* commandManager = ProjucerApplication::getApp().commandManager.get())
    {
        const ScopedPointer<XmlElement> keys (commandManager->getKeyMappings()->createXml (true));

        if (keys != nullptr)
            getGlobalProperties().setValue ("keyMappings", keys);
    }
}

void StoredSettings::flush()
{
    updateGlobalPreferences();
    saveSwatchColours();

    for (auto i = propertyFiles.size(); --i >= 0;)
        propertyFiles.getUnchecked(i)->saveIfNeeded();
}

void StoredSettings::reload()
{
    propertyFiles.clear();
    propertyFiles.add (createPropsFile ("Projucer", false));

    ScopedPointer<XmlElement> projectDefaultsXml (propertyFiles.getFirst()->getXmlValue ("PROJECT_DEFAULT_SETTINGS"));

    if (projectDefaultsXml != nullptr)
        projectDefaults = ValueTree::fromXml (*projectDefaultsXml);

    // recent files...
    recentFiles.restoreFromString (getGlobalProperties().getValue ("recentFiles"));
    recentFiles.removeNonExistentFiles();

    loadSwatchColours();
}

Array<File> StoredSettings::getLastProjects()
{
    StringArray s;
    s.addTokens (getGlobalProperties().getValue ("lastProjects"), "|", "");

    Array<File> f;
    for (int i = 0; i < s.size(); ++i)
        f.add (File (s[i]));

    return f;
}

void StoredSettings::setLastProjects (const Array<File>& files)
{
    StringArray s;
    for (int i = 0; i < files.size(); ++i)
        s.add (files.getReference(i).getFullPathName());

    getGlobalProperties().setValue ("lastProjects", s.joinIntoString ("|"));
}

void StoredSettings::updateOldProjectSettingsFiles()
{
    // Global properties file hasn't been created yet so create a dummy file
    auto projucerSettingsDirectory = ProjucerApplication::getApp().getPropertyFileOptionsFor ("Dummy", false)
                                                                  .getDefaultFile().getParentDirectory();

    auto newProjectSettingsDir = projucerSettingsDirectory.getChildFile ("ProjectSettings");
    newProjectSettingsDir.createDirectory();

    DirectoryIterator iter (projucerSettingsDirectory, false, "*.settings");
    while (iter.next())
    {
        auto f = iter.getFile();
        auto oldFileName = f.getFileName();

        if (oldFileName.contains ("Introjucer"))
        {
            auto newFileName = oldFileName.replace ("Introjucer", "Projucer");

            if (oldFileName.contains ("_Project"))
                f.moveFileTo (f.getSiblingFile (newProjectSettingsDir.getFileName()).getChildFile (newFileName));
            else
                f.moveFileTo (f.getSiblingFile (newFileName));
        }
    }
}

//==============================================================================
void StoredSettings::loadSwatchColours()
{
    swatchColours.clear();

    #define COL(col)  Colours::col,

    const Colour colours[] =
    {
        #include "jucer_Colours.h"
        Colours::transparentBlack
    };

    #undef COL

    const auto numSwatchColours = 24;
    auto& props = getGlobalProperties();

    for (auto i = 0; i < numSwatchColours; ++i)
        swatchColours.add (Colour::fromString (props.getValue ("swatchColour" + String (i),
                                                               colours [2 + i].toString())));
}

void StoredSettings::saveSwatchColours()
{
    auto& props = getGlobalProperties();

    for (auto i = 0; i < swatchColours.size(); ++i)
        props.setValue ("swatchColour" + String (i), swatchColours.getReference(i).toString());
}

int StoredSettings::ColourSelectorWithSwatches::getNumSwatches() const
{
    return getAppSettings().swatchColours.size();
}

Colour StoredSettings::ColourSelectorWithSwatches::getSwatchColour (int index) const
{
    return getAppSettings().swatchColours [index];
}

void StoredSettings::ColourSelectorWithSwatches::setSwatchColour (int index, const Colour& newColour)
{
    getAppSettings().swatchColours.set (index, newColour);
}

//==============================================================================
static bool doesSDKPathContainFile (const File& relativeTo, const String& path, const String& fileToCheckFor)
{
    auto actualPath = path.replace ("${user.home}", File::getSpecialLocation (File::userHomeDirectory).getFullPathName());
    return relativeTo.getChildFile (actualPath + "/" + fileToCheckFor).existsAsFile();
}

Value StoredSettings::getGlobalPath (const Identifier& key, DependencyPathOS os)
{
    auto v = projectDefaults.getPropertyAsValue (key, nullptr);

    if (v.toString().isEmpty())
    {
        auto defaultPath = getFallbackPath (key, os);
        if (os == TargetOS::getThisOS())
            v = defaultPath;
    }

    return v;
}

String StoredSettings::getFallbackPath (const Identifier& key, DependencyPathOS os)
{
    if (key == Ids::vst3Path)
        return os == TargetOS::windows ? "c:\\SDKs\\VST_SDK\\VST3_SDK"
                                       : "~/SDKs/VST_SDK/VST3_SDK";

    if (key == Ids::rtasPath)
    {
        if (os == TargetOS::windows)   return "c:\\SDKs\\PT_90_SDK";
        if (os == TargetOS::osx)       return "~/SDKs/PT_90_SDK";

        // no RTAS on this OS!
        jassertfalse;
        return {};
    }

    if (key == Ids::aaxPath)
    {
        if (os == TargetOS::windows)   return "c:\\SDKs\\AAX";
        if (os == TargetOS::osx)       return "~/SDKs/AAX" ;

        // no AAX on this OS!
        jassertfalse;
        return {};
    }

    if (key == Ids::androidSDKPath)
        return "${user.home}/Library/Android/sdk";

    if (key == Ids::androidNDKPath)
        return "${user.home}/Library/Android/sdk/ndk-bundle";

    // didn't recognise the key provided!
    jassertfalse;
    return {};
}

bool StoredSettings::isGlobalPathValid (const File& relativeTo, const Identifier& key, const String& path)
{
    String fileToCheckFor;

    if (key == Ids::vst3Path)
    {
        fileToCheckFor = "base/source/baseiids.cpp";
    }
    else if (key == Ids::rtasPath)
    {
        fileToCheckFor = "AlturaPorts/TDMPlugIns/PlugInLibrary/EffectClasses/CEffectProcessMIDI.cpp";
    }
    else if (key == Ids::aaxPath)
    {
        fileToCheckFor = "Interfaces/AAX_Exports.cpp";
    }
    else if (key == Ids::androidSDKPath)
    {
       #if JUCE_WINDOWS
        fileToCheckFor = "platform-tools/adb.exe";
       #else
        fileToCheckFor = "platform-tools/adb";
       #endif
    }
    else if (key == Ids::androidNDKPath)
    {
       #if JUCE_WINDOWS
        fileToCheckFor = "ndk-depends.cmd";
       #else
        fileToCheckFor = "ndk-depends";
       #endif
    }
    else
    {
        // didn't recognise the key provided!
        jassertfalse;
        return false;
    }

    return doesSDKPathContainFile (relativeTo, path, fileToCheckFor);
}
