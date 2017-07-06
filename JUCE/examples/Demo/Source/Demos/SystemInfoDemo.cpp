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

#include "../JuceDemoHeader.h"

static String getMacAddressList()
{
    Array<MACAddress> macAddresses;
    MACAddress::findAllAddresses (macAddresses);

    String addressList;
    for (int i = 0; i < macAddresses.size(); ++i)
        addressList << "   " << macAddresses[i].toString() << newLine;

    return addressList;
}

static String getFileSystemRoots()
{
    Array<File> roots;
    File::findFileSystemRoots (roots);

    StringArray rootList;
    for (int i = 0; i < roots.size(); ++i)
        rootList.add (roots[i].getFullPathName());

    return rootList.joinIntoString (", ");
}

static String getIPAddressList()
{
    Array<IPAddress> addresses;
    IPAddress::findAllAddresses (addresses);

    String addressList;

    for (int i = 0; i < addresses.size(); ++i)
        addressList << "   " << addresses[i].toString() << newLine;

    return addressList;
}

static const char* getDisplayOrientation()
{
    switch (Desktop::getInstance().getCurrentOrientation())
    {
        case Desktop::upright:              return "Upright";
        case Desktop::upsideDown:           return "Upside-down";
        case Desktop::rotatedClockwise:     return "Rotated Clockwise";
        case Desktop::rotatedAntiClockwise: return "Rotated Anti-clockwise";
        default: jassertfalse; break;
    }

    return nullptr;
}

static String getDisplayInfo()
{
    const Desktop::Displays& displays = Desktop::getInstance().getDisplays();

    String displayDesc;

    for (int i = 0; i < displays.displays.size(); ++i)
    {
        const Desktop::Displays::Display display = displays.displays.getReference(i);

        displayDesc
          << "Display " << (i + 1) << (display.isMain ? " (main)" : "") << ":" << newLine
          << "  Total area: " << display.totalArea.toString() << newLine
          << "  User area:  " << display.userArea.toString() << newLine
          << "  DPI: " << display.dpi << newLine
          << "  Scale: " << display.scale << newLine
          << newLine;
    }


    displayDesc << "Orientation: " << getDisplayOrientation() << newLine;

    return displayDesc;
}

static String getAllSystemInfo()
{
    String systemInfo;

    systemInfo
      << "Here are a few system statistics..." << newLine
      << newLine
      << "Time and date:    " << Time::getCurrentTime().toString (true, true) << newLine
      << "System up-time:   " << RelativeTime::milliseconds ((int64) Time::getMillisecondCounterHiRes()).getDescription() << newLine
      << "Compilation date: " << Time::getCompilationDate().toString (true, false) << newLine
      << newLine
      << "Operating system: " << SystemStats::getOperatingSystemName() << newLine
      << "Host name:        " << SystemStats::getComputerName() << newLine
      << "Device type:      " << SystemStats::getDeviceDescription() << newLine
      << "User logon name:  " << SystemStats::getLogonName() << newLine
      << "Full user name:   " << SystemStats::getFullUserName() << newLine
      << "User region:      " << SystemStats::getUserRegion() << newLine
      << "User language:    " << SystemStats::getUserLanguage() << newLine
      << "Display language: " << SystemStats::getDisplayLanguage() << newLine
      << newLine;

    systemInfo
      << "Number of logical CPUs:  " << SystemStats::getNumCpus() << newLine
      << "Number of physical CPUs: " << SystemStats::getNumPhysicalCpus() << newLine
      << "Memory size:     " << SystemStats::getMemorySizeInMegabytes() << " MB" << newLine
      << "CPU vendor:      " << SystemStats::getCpuVendor() << newLine
      << "CPU model:       " << SystemStats::getCpuModel() << newLine
      << "CPU speed:       " << SystemStats::getCpuSpeedInMegaherz() << " MHz" << newLine
      << "CPU has MMX:     " << (SystemStats::hasMMX()    ? "yes" : "no") << newLine
      << "CPU has SSE:     " << (SystemStats::hasSSE()    ? "yes" : "no") << newLine
      << "CPU has SSE2:    " << (SystemStats::hasSSE2()   ? "yes" : "no") << newLine
      << "CPU has SSE3:    " << (SystemStats::hasSSE3()   ? "yes" : "no") << newLine
      << "CPU has SSSE3:   " << (SystemStats::hasSSSE3()  ? "yes" : "no") << newLine
      << "CPU has SSE4.1:  " << (SystemStats::hasSSE41()  ? "yes" : "no") << newLine
      << "CPU has SSE4.2:  " << (SystemStats::hasSSE42()  ? "yes" : "no") << newLine
      << "CPU has 3DNOW:   " << (SystemStats::has3DNow()  ? "yes" : "no") << newLine
      << "CPU has AVX:     " << (SystemStats::hasAVX()    ? "yes" : "no") << newLine
      << "CPU has AVX2:    " << (SystemStats::hasAVX2()   ? "yes" : "no") << newLine
      << "CPU has Neon:    " << (SystemStats::hasNeon()   ? "yes" : "no") << newLine
      << newLine;

    systemInfo
      << "Current working directory:  " << File::getCurrentWorkingDirectory().getFullPathName() << newLine
      << "Current application file:   " << File::getSpecialLocation (File::currentApplicationFile).getFullPathName() << newLine
      << "Current executable file:    " << File::getSpecialLocation (File::currentExecutableFile) .getFullPathName() << newLine
      << "Invoked executable file:    " << File::getSpecialLocation (File::invokedExecutableFile) .getFullPathName() << newLine
      << newLine;

    systemInfo
      << "User home folder:               " << File::getSpecialLocation (File::userHomeDirectory)             .getFullPathName() << newLine
      << "User desktop folder:            " << File::getSpecialLocation (File::userDesktopDirectory)          .getFullPathName() << newLine
      << "User documents folder:          " << File::getSpecialLocation (File::userDocumentsDirectory)        .getFullPathName() << newLine
      << "User application data folder:   " << File::getSpecialLocation (File::userApplicationDataDirectory)  .getFullPathName() << newLine
      << "User music folder:              " << File::getSpecialLocation (File::userMusicDirectory)            .getFullPathName() << newLine
      << "User movies folder:             " << File::getSpecialLocation (File::userMoviesDirectory)           .getFullPathName() << newLine
      << "User pictures folder:           " << File::getSpecialLocation (File::userPicturesDirectory)         .getFullPathName() << newLine
      << "Common application data folder: " << File::getSpecialLocation (File::commonApplicationDataDirectory).getFullPathName() << newLine
      << "Common documents folder:        " << File::getSpecialLocation (File::commonDocumentsDirectory)      .getFullPathName() << newLine
      << "Local temp folder:              " << File::getSpecialLocation (File::tempDirectory)                 .getFullPathName() << newLine
      << newLine;

    systemInfo
      << "File System roots: "          << getFileSystemRoots() << newLine
      << "Free space in home folder: "  << File::descriptionOfSizeInBytes (File::getSpecialLocation (File::userHomeDirectory)
                                                                                .getBytesFreeOnVolume()) << newLine
      << newLine
      << getDisplayInfo() << newLine
      << "Network IP addresses: " << newLine << getIPAddressList() << newLine
      << "Network card MAC addresses: " << newLine << getMacAddressList() << newLine;

    DBG (systemInfo);
    return systemInfo;
}

class SystemInfoDemo  : public Component
{
public:
    SystemInfoDemo()
    {
        addAndMakeVisible (resultsBox);
        resultsBox.setReadOnly (true);
        resultsBox.setMultiLine (true);
        resultsBox.setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
        resultsBox.setFont (Font (Font::getDefaultMonospacedFontName(), 12.0f, Font::plain));
        resultsBox.setText (getAllSystemInfo());
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::windowBackground,
                                           Colour::greyLevel (0.93f)));
    }

    void resized() override
    {
        resultsBox.setBounds (getLocalBounds().reduced (8));
    }

private:
    TextEditor resultsBox;

    void lookAndFeelChanged() override
    {
        resultsBox.applyFontToAllText (resultsBox.getFont());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SystemInfoDemo)
};

// This static object will register this demo type in a global list of demos..
static JuceDemoType<SystemInfoDemo> demo ("02 System Info");
