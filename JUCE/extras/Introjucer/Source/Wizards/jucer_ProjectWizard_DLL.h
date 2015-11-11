/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

struct DynamicLibraryWizard   : public NewProjectWizard
{
    DynamicLibraryWizard()  {}

    String getName() const override         { return TRANS("Dynamic Library"); }
    String getDescription() const override  { return TRANS("Creates a Dynamic Library template with support for all JUCE features."); }
    const char* getIcon() const override    { return BinaryData::wizard_DLL_svg; }

    bool initialiseProject (Project& project) override
    {
        createSourceFolder();
        project.getProjectTypeValue() = ProjectType::getDynamicLibTypeName();
        createSourceGroup (project);
        setExecutableNameForAllTargets (project, File::createLegalFileName (appTitle));

        return true;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicLibraryWizard)
};
