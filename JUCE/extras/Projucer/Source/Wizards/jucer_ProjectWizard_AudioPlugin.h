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

struct AudioPluginAppWizard   : public NewProjectWizard
{
    AudioPluginAppWizard()  {}

    String getName() const override         { return TRANS("Audio Plug-In"); }
    String getDescription() const override  { return TRANS("Creates a VST/AU/RTAS/AAX audio plug-in. This template features a single window GUI and Audio/MIDI IO functions."); }
    const char* getIcon() const override    { return BinaryData::wizard_AudioPlugin_svg; }

    StringArray getDefaultModules() override
    {
        StringArray s (NewProjectWizard::getDefaultModules());
        s.add ("juce_audio_plugin_client");
        return s;
    }

    bool initialiseProject (Project& project) override
    {
        createSourceFolder();

        String filterClassName = CodeHelpers::makeValidIdentifier (appTitle, true, true, false) + "AudioProcessor";
        filterClassName = filterClassName.substring (0, 1).toUpperCase() + filterClassName.substring (1);
        String editorClassName = filterClassName + "Editor";

        File filterCppFile = getSourceFilesFolder().getChildFile ("PluginProcessor.cpp");
        File filterHFile   = filterCppFile.withFileExtension (".h");
        File editorCppFile = getSourceFilesFolder().getChildFile ("PluginEditor.cpp");
        File editorHFile   = editorCppFile.withFileExtension (".h");

        project.getProjectTypeValue() = ProjectType_AudioPlugin::getTypeName();

        Project::Item sourceGroup (createSourceGroup (project));
        project.getConfigFlag ("JUCE_QUICKTIME") = Project::configFlagDisabled; // disabled because it interferes with RTAS build on PC

        setExecutableNameForAllTargets (project, File::createLegalFileName (appTitle));

        String appHeaders (CodeHelpers::createIncludeStatement (project.getAppIncludeFile(), filterCppFile));

        String filterCpp = project.getFileTemplate ("jucer_AudioPluginFilterTemplate_cpp")
                            .replace ("FILTERHEADERS", CodeHelpers::createIncludeStatement (filterHFile, filterCppFile)
                                                            + newLine + CodeHelpers::createIncludeStatement (editorHFile, filterCppFile), false)
                            .replace ("FILTERCLASSNAME", filterClassName, false)
                            .replace ("EDITORCLASSNAME", editorClassName, false);

        String filterH = project.getFileTemplate ("jucer_AudioPluginFilterTemplate_h")
                            .replace ("APPHEADERS", appHeaders, false)
                            .replace ("FILTERCLASSNAME", filterClassName, false)
                            .replace ("HEADERGUARD", CodeHelpers::makeHeaderGuardName (filterHFile), false);

        String editorCpp = project.getFileTemplate ("jucer_AudioPluginEditorTemplate_cpp")
                            .replace ("EDITORCPPHEADERS", CodeHelpers::createIncludeStatement (filterHFile, filterCppFile)
                                                               + newLine + CodeHelpers::createIncludeStatement (editorHFile, filterCppFile), false)
                            .replace ("FILTERCLASSNAME", filterClassName, false)
                            .replace ("EDITORCLASSNAME", editorClassName, false);

        String editorH = project.getFileTemplate ("jucer_AudioPluginEditorTemplate_h")
                            .replace ("EDITORHEADERS", appHeaders + newLine + CodeHelpers::createIncludeStatement (filterHFile, filterCppFile), false)
                            .replace ("FILTERCLASSNAME", filterClassName, false)
                            .replace ("EDITORCLASSNAME", editorClassName, false)
                            .replace ("HEADERGUARD", CodeHelpers::makeHeaderGuardName (editorHFile), false);

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (filterCppFile, filterCpp))
            failedFiles.add (filterCppFile.getFullPathName());

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (filterHFile, filterH))
            failedFiles.add (filterHFile.getFullPathName());

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (editorCppFile, editorCpp))
            failedFiles.add (editorCppFile.getFullPathName());

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (editorHFile, editorH))
            failedFiles.add (editorHFile.getFullPathName());

        sourceGroup.addFileAtIndex (filterCppFile, -1, true);
        sourceGroup.addFileAtIndex (filterHFile,   -1, false);
        sourceGroup.addFileAtIndex (editorCppFile, -1, true);
        sourceGroup.addFileAtIndex (editorHFile,   -1, false);

        return true;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAppWizard)
};
