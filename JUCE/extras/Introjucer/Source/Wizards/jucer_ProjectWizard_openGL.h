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

struct OpenGLAppWizard   : public NewProjectWizard
{
    OpenGLAppWizard()  {}

    String getName() const override         { return TRANS("OpenGL Application"); }
    String getDescription() const override  { return TRANS("Creates a blank JUCE application with a single window component. This component supports openGL drawing features including 3D model import and GLSL shaders."); }
    const char* getIcon() const override    { return BinaryData::wizard_OpenGL_svg; }

    bool initialiseProject (Project& project) override
    {
        createSourceFolder();

        File mainCppFile    = getSourceFilesFolder().getChildFile ("Main.cpp");
        File contentCompCpp = getSourceFilesFolder().getChildFile ("MainComponent.cpp");
        File contentCompH   = contentCompCpp.withFileExtension (".h");
        String contentCompName = "MainContentComponent";

        project.getProjectTypeValue() = ProjectType::getGUIAppTypeName();

        Project::Item sourceGroup (createSourceGroup (project));

        setExecutableNameForAllTargets (project, File::createLegalFileName (appTitle));

        String appHeaders (CodeHelpers::createIncludeStatement (project.getAppIncludeFile(), mainCppFile));

        // create main window
        String windowCpp = project.getFileTemplate ("jucer_OpenGLComponentTemplate_cpp")
                            .replace ("INCLUDE_JUCE", CodeHelpers::createIncludeStatement (project.getAppIncludeFile(), contentCompCpp), false);

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (contentCompCpp, windowCpp))
            failedFiles.add (contentCompCpp.getFullPathName());

        sourceGroup.addFileAtIndex (contentCompCpp, -1, true);


        // create main cpp
        String mainCpp = project.getFileTemplate ("jucer_MainTemplate_SimpleWindow_cpp")
                            .replace ("APPHEADERS", appHeaders, false)
                            .replace ("APPCLASSNAME", CodeHelpers::makeValidIdentifier (appTitle + "Application", false, true, false), false)
                            .replace ("APPNAME", CppTokeniserFunctions::addEscapeChars (appTitle), false)
                            .replace ("CONTENTCOMPCLASS", contentCompName, false)
                            .replace ("ALLOWMORETHANONEINSTANCE", "true", false);

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (mainCppFile, mainCpp))
            failedFiles.add (mainCppFile.getFullPathName());

        sourceGroup.addFileAtIndex (mainCppFile, -1, true);

        return true;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLAppWizard)
};
