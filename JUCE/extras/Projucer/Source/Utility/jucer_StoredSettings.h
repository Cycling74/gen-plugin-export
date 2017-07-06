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

#pragma once

#include <map>
#include "../Application/jucer_AppearanceSettings.h"

//==============================================================================
class StoredSettings : public ValueTree::Listener
{
public:
    StoredSettings();
    ~StoredSettings();

    PropertiesFile& getGlobalProperties();
    PropertiesFile& getProjectProperties (const String& projectUID);

    void flush();
    void reload();

    //==============================================================================
    RecentlyOpenedFilesList recentFiles;

    Array<File> getLastProjects();
    void setLastProjects (const Array<File>& files);

    //==============================================================================
    Array<Colour> swatchColours;

    struct ColourSelectorWithSwatches    : public ColourSelector
    {
        ColourSelectorWithSwatches() {}

        int getNumSwatches() const override;
        Colour getSwatchColour (int index) const override;
        void setSwatchColour (int index, const Colour& newColour) override;
    };

    //==============================================================================
    AppearanceSettings appearance;

    StringArray monospacedFontNames;

    //==============================================================================
    Value getGlobalPath (const Identifier& key, DependencyPathOS);
    String getFallbackPath (const Identifier& key, DependencyPathOS);

    bool isGlobalPathValid (const File& relativeTo, const Identifier& key, const String& path);

private:
    OwnedArray<PropertiesFile> propertyFiles;
    ValueTree projectDefaults;

    void changed()
    {
        ScopedPointer<XmlElement> data (projectDefaults.createXml());
        propertyFiles.getUnchecked (0)->setValue ("PROJECT_DEFAULT_SETTINGS", data);
    }

    void updateGlobalPreferences();
    void updateRecentFiles();
    void updateKeyMappings();

    void loadSwatchColours();
    void saveSwatchColours();

    void updateOldProjectSettingsFiles();

    //==============================================================================
    void valueTreePropertyChanged (ValueTree&, const Identifier&) override  { changed(); }
    void valueTreeChildAdded (ValueTree&, ValueTree&) override              { changed(); }
    void valueTreeChildRemoved (ValueTree&, ValueTree&, int) override       { changed(); }
    void valueTreeChildOrderChanged (ValueTree&, int, int) override         { changed(); }
    void valueTreeParentChanged (ValueTree&) override                       { changed(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StoredSettings)
};

StoredSettings& getAppSettings();
PropertiesFile& getGlobalProperties();
