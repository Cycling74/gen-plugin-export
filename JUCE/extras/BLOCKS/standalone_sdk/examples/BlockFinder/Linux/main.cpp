/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#include "../BlockFinder.h"

// A simple JUCE app containing our BlockFinder. This is a quick way of
// setting up an event loop so we can receive Block topology change events.
class MyJUCEApp  : public juce::JUCEApplicationBase
{
public:
    MyJUCEApp()  {}
    ~MyJUCEApp() {}

    void initialise (const juce::String&) override {}
    void shutdown() override                       {}

    const juce::String getApplicationName() override           { return "BlockFinder"; }
    const juce::String getApplicationVersion() override        { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override                 { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    void suspended() override           {}
    void resumed() override             {}
    void systemRequestedQuit() override {}
    void unhandledException(const std::exception*, const juce::String&,
                            int lineNumber) override {}

private:
    // Our BLOCKS class.
    BlockFinder finder;
};

START_JUCE_APPLICATION (MyJUCEApp)
