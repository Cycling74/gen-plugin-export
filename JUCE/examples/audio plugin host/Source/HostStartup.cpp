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

#include "../JuceLibraryCode/JuceHeader.h"
#include "MainHostWindow.h"
#include "InternalFilters.h"

#if ! (JUCE_PLUGINHOST_VST || JUCE_PLUGINHOST_VST3 || JUCE_PLUGINHOST_AU)
 #error "If you're building the audio plugin host, you probably want to enable VST and/or AU support"
#endif


//==============================================================================
class PluginHostApp  : public JUCEApplication,
                       private AsyncUpdater
{
public:
    PluginHostApp() {}

    void initialise (const String&) override
    {
        // initialise our settings file..

        PropertiesFile::Options options;
        options.applicationName     = "Juce Audio Plugin Host";
        options.filenameSuffix      = "settings";
        options.osxLibrarySubFolder = "Preferences";

        appProperties = new ApplicationProperties();
        appProperties->setStorageParameters (options);

        mainWindow = new MainHostWindow();
        mainWindow->setUsingNativeTitleBar (true);

        commandManager.registerAllCommandsForTarget (this);
        commandManager.registerAllCommandsForTarget (mainWindow);

        mainWindow->menuItemsChanged();

        // Important note! We're going to use an async update here so that if we need
        // to re-open a file and instantiate some plugins, it will happen AFTER this
        // initialisation method has returned.
        // On Windows this probably won't make a difference, but on OSX there's a subtle event loop
        // issue that can happen if a plugin runs one of those irritating modal dialogs while it's
        // being loaded. If that happens inside this method, the OSX event loop seems to be in some
        // kind of special "initialisation" mode and things get confused. But if we load the plugin
        // later when the normal event loop is running, everything's fine.
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        File fileToOpen;

        for (int i = 0; i < getCommandLineParameterArray().size(); ++i)
        {
            fileToOpen = File::getCurrentWorkingDirectory().getChildFile (getCommandLineParameterArray()[i]);

            if (fileToOpen.existsAsFile())
                break;
        }

        if (! fileToOpen.existsAsFile())
        {
            RecentlyOpenedFilesList recentFiles;
            recentFiles.restoreFromString (getAppProperties().getUserSettings()->getValue ("recentFilterGraphFiles"));

            if (recentFiles.getNumFiles() > 0)
                fileToOpen = recentFiles.getFile (0);
        }

        if (fileToOpen.existsAsFile())
            if (GraphDocumentComponent* graph = mainWindow->getGraphEditor())
                if (FilterGraph* ioGraph = graph->graph.get())
                    ioGraph->loadFrom (fileToOpen, true);
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        appProperties = nullptr;
        LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
            mainWindow->tryToQuitApplication();
        else
            JUCEApplicationBase::quit();
    }

    const String getApplicationName() override       { return "Juce Plug-In Host"; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    ApplicationCommandManager commandManager;
    ScopedPointer<ApplicationProperties> appProperties;

private:
    ScopedPointer<MainHostWindow> mainWindow;
};

static PluginHostApp& getApp()                      { return *dynamic_cast<PluginHostApp*>(JUCEApplication::getInstance()); }
ApplicationCommandManager& getCommandManager()      { return getApp().commandManager; }
ApplicationProperties& getAppProperties()           { return *getApp().appProperties; }


// This kicks the whole thing off..
START_JUCE_APPLICATION (PluginHostApp)
