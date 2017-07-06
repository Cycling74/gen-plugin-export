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
#include "jucer_NewProjectWizardClasses.h"
#include "../Project/jucer_ProjectType.h"
#include "../Project/jucer_Module.h"
#include "../Project Saving/jucer_ProjectExporter.h"
#include "../Application/jucer_MainWindow.h"
#include "../Utility/jucer_SlidingPanelComponent.h"

struct NewProjectWizardClasses
{
    class WizardComp;
    #include "jucer_NewProjectWizard.h"

    #include "jucer_ProjectWizard_GUIApp.h"
    #include "jucer_ProjectWizard_Console.h"
    #include "jucer_ProjectWizard_AudioPlugin.h"
    #include "jucer_ProjectWizard_StaticLibrary.h"
    #include "jucer_ProjectWizard_DLL.h"
    #include "jucer_ProjectWizard_openGL.h"
    #include "jucer_ProjectWizard_Animated.h"
    #include "jucer_ProjectWizard_AudioApp.h"
    #include "jucer_ProjectWizard_Blank.h"

    #include "jucer_NewProjectWizardComponent.h"
    #include "jucer_TemplateThumbnailsComponent.h"
    #include "jucer_StartPageComponent.h"

    //==============================================================================
    static int getNumWizards() noexcept
    {
        return 9;
    }

    static NewProjectWizard* createWizardType (int index)
    {
        switch (index)
        {
            case 0:     return new NewProjectWizardClasses::GUIAppWizard();
            case 1:     return new NewProjectWizardClasses::AnimatedAppWizard();
            case 2:     return new NewProjectWizardClasses::OpenGLAppWizard();
            case 3:     return new NewProjectWizardClasses::ConsoleAppWizard();
            case 4:     return new NewProjectWizardClasses::AudioAppWizard();
            case 5:     return new NewProjectWizardClasses::AudioPluginAppWizard();
            case 6:     return new NewProjectWizardClasses::StaticLibraryWizard();
            case 7:     return new NewProjectWizardClasses::DynamicLibraryWizard();
            case 8:     return new NewProjectWizardClasses::BlankAppWizard();
            default:    jassertfalse; break;
        }

        return nullptr;
    }

    static StringArray getWizardNames()
    {
        StringArray s;

        for (int i = 0; i < getNumWizards(); ++i)
        {
            ScopedPointer<NewProjectWizard> wiz (createWizardType (i));
            s.add (wiz->getName());
        }

        return s;
    }
};

Component* createNewProjectWizardComponent()
{
    return new NewProjectWizardClasses::StartPageComponent();
}
