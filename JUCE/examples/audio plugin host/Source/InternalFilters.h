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

#include "FilterGraph.h"


//==============================================================================
/**
    Manages the internal plugin types.
*/
class InternalPluginFormat   : public AudioPluginFormat
{
public:
    //==============================================================================
    InternalPluginFormat();
    ~InternalPluginFormat() {}

    //==============================================================================
    enum InternalFilterType
    {
        audioInputFilter = 0,
        audioOutputFilter,
        midiInputFilter,

        endOfFilterTypes
    };

    const PluginDescription* getDescriptionFor (const InternalFilterType type);

    void getAllTypes (OwnedArray <PluginDescription>& results);

    //==============================================================================
    String getName() const override                                      { return "Internal"; }
    bool fileMightContainThisPluginType (const String&) override         { return true; }
    FileSearchPath getDefaultLocationsToSearch() override                { return FileSearchPath(); }
    bool canScanForPlugins() const override                              { return false; }
    void findAllTypesForFile (OwnedArray <PluginDescription>&, const String&) override     {}
    bool doesPluginStillExist (const PluginDescription&) override        { return true; }
    String getNameOfPluginFromIdentifier (const String& fileOrIdentifier) override   { return fileOrIdentifier; }
    bool pluginNeedsRescanning (const PluginDescription&) override       { return false; }
    StringArray searchPathsForPlugins (const FileSearchPath&, bool, bool) override         { return StringArray(); }

private:
    //==============================================================================
    void createPluginInstance (const PluginDescription& description,
                               double initialSampleRate,
                               int initialBufferSize,
                               void* userData,
                               void (*callback) (void*, AudioPluginInstance*, const String&)) override;
    bool requiresUnblockedMessageThreadDuringCreation (const PluginDescription&) const noexcept override;
private:
    //==============================================================================
    PluginDescription audioInDesc;
    PluginDescription audioOutDesc;
    PluginDescription midiInDesc;
};
