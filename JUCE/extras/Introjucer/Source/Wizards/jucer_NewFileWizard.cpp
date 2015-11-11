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

#include "jucer_NewFileWizard.h"

NewFileWizard::Type* createGUIComponentWizard();

//==============================================================================
namespace
{
    static String fillInBasicTemplateFields (const File& file, const Project::Item& item, const char* templateName)
    {
        return item.project.getFileTemplate (templateName)
                      .replace ("FILENAME", file.getFileName(), false)
                      .replace ("DATE", Time::getCurrentTime().toString (true, true, true), false)
                      .replace ("AUTHOR", SystemStats::getFullUserName(), false)
                      .replace ("HEADERGUARD", CodeHelpers::makeHeaderGuardName (file), false)
                      .replace ("INCLUDE_CORRESPONDING_HEADER", CodeHelpers::createIncludeStatement (file.withFileExtension (".h"), file));
    }

    static bool fillInNewCppFileTemplate (const File& file, const Project::Item& item, const char* templateName)
    {
        return FileHelpers::overwriteFileWithNewDataIfDifferent (file, fillInBasicTemplateFields (file, item, templateName));
    }

    const int menuBaseID = 0x12d83f0;
}

//==============================================================================
class NewCppFileWizard  : public NewFileWizard::Type
{
public:
    NewCppFileWizard() {}

    String getName()  { return "CPP File"; }

    void createNewFile (Project::Item parent)
    {
        const File newFile (askUserToChooseNewFile ("SourceCode.cpp", "*.cpp", parent));

        if (newFile != File::nonexistent)
            create (parent, newFile, "jucer_NewCppFileTemplate_cpp");
    }

    static bool create (Project::Item parent, const File& newFile, const char* templateName)
    {
        if (fillInNewCppFileTemplate (newFile, parent, templateName))
        {
            parent.addFileRetainingSortOrder (newFile, true);
            return true;
        }

        showFailedToWriteMessage (newFile);
        return false;
    }
};

//==============================================================================
class NewHeaderFileWizard  : public NewFileWizard::Type
{
public:
    NewHeaderFileWizard() {}

    String getName()  { return "Header File"; }

    void createNewFile (Project::Item parent)
    {
        const File newFile (askUserToChooseNewFile ("SourceCode.h", "*.h", parent));

        if (newFile != File::nonexistent)
            create (parent, newFile, "jucer_NewCppFileTemplate_h");
    }

    static bool create (Project::Item parent, const File& newFile, const char* templateName)
    {
        if (fillInNewCppFileTemplate (newFile, parent, templateName))
        {
            parent.addFileRetainingSortOrder (newFile, true);
            return true;
        }

        showFailedToWriteMessage (newFile);
        return false;
    }
};

//==============================================================================
class NewCppAndHeaderFileWizard  : public NewFileWizard::Type
{
public:
    NewCppAndHeaderFileWizard() {}

    String getName()  { return "CPP & Header File"; }

    void createNewFile (Project::Item parent)
    {
        const File newFile (askUserToChooseNewFile ("SourceCode.h", "*.h;*.cpp", parent));

        if (newFile != File::nonexistent)
        {
            if (NewCppFileWizard::create (parent, newFile.withFileExtension ("h"),   "jucer_NewCppFileTemplate_h"))
                NewCppFileWizard::create (parent, newFile.withFileExtension ("cpp"), "jucer_NewCppFileTemplate_cpp");
        }
    }
};

//==============================================================================
class NewComponentFileWizard  : public NewFileWizard::Type
{
public:
    NewComponentFileWizard() {}

    String getName()  { return "Component class (split between a CPP & header)"; }

    void createNewFile (Project::Item parent)
    {
        for (;;)
        {
            AlertWindow aw (TRANS ("Create new Component class"),
                            TRANS ("Please enter the name for the new class"),
                            AlertWindow::NoIcon, nullptr);

            aw.addTextEditor (getClassNameFieldName(), String::empty, String::empty, false);
            aw.addButton (TRANS ("Create Files"),  1, KeyPress (KeyPress::returnKey));
            aw.addButton (TRANS ("Cancel"),        0, KeyPress (KeyPress::escapeKey));

            if (aw.runModalLoop() == 0)
                break;

            const String className (aw.getTextEditorContents (getClassNameFieldName()).trim());

            if (className == CodeHelpers::makeValidIdentifier (className, false, true, false))
            {
                const File newFile (askUserToChooseNewFile (className + ".h", "*.h;*.cpp", parent));

                if (newFile != File::nonexistent)
                    createFiles (parent, className, newFile);

                break;
            }
        }
    }

    static bool create (const String& className, Project::Item parent,
                        const File& newFile, const char* templateName)
    {
        String content = fillInBasicTemplateFields (newFile, parent, templateName)
                            .replace ("COMPONENTCLASS", className)
                            .replace ("INCLUDE_JUCE", CodeHelpers::createIncludeStatement (parent.project.getAppIncludeFile(), newFile));

        if (FileHelpers::overwriteFileWithNewDataIfDifferent (newFile, content))
        {
            parent.addFileRetainingSortOrder (newFile, true);
            return true;
        }

        showFailedToWriteMessage (newFile);
        return false;
    }

private:
    virtual void createFiles (Project::Item parent, const String& className, const File& newFile)
    {
        if (create (className, parent, newFile.withFileExtension ("h"),   "jucer_NewComponentTemplate_h"))
            create (className, parent, newFile.withFileExtension ("cpp"), "jucer_NewComponentTemplate_cpp");
    }

    static String getClassNameFieldName()  { return "Class Name"; }
};

//==============================================================================
class NewSingleFileComponentFileWizard  : public NewComponentFileWizard
{
public:
    NewSingleFileComponentFileWizard() {}

    String getName()  { return "Component class (in a single source file)"; }

    void createFiles (Project::Item parent, const String& className, const File& newFile)
    {
        create (className, parent, newFile.withFileExtension ("h"), "jucer_NewInlineComponentTemplate_h");
    }
};


//==============================================================================
void NewFileWizard::Type::showFailedToWriteMessage (const File& file)
{
    AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                 "Failed to Create File!",
                                 "Couldn't write to the file: " + file.getFullPathName());
}

File NewFileWizard::Type::askUserToChooseNewFile (const String& suggestedFilename, const String& wildcard,
                                                  const Project::Item& projectGroupToAddTo)
{
    FileChooser fc ("Select File to Create",
                    projectGroupToAddTo.determineGroupFolder()
                                       .getChildFile (suggestedFilename)
                                       .getNonexistentSibling(),
                    wildcard);

    if (fc.browseForFileToSave (true))
        return fc.getResult();

    return File::nonexistent;
}

//==============================================================================
NewFileWizard::NewFileWizard()
{
    registerWizard (new NewCppFileWizard());
    registerWizard (new NewHeaderFileWizard());
    registerWizard (new NewCppAndHeaderFileWizard());
    registerWizard (new NewComponentFileWizard());
    registerWizard (new NewSingleFileComponentFileWizard());
    registerWizard (createGUIComponentWizard());
}

NewFileWizard::~NewFileWizard()
{
}

void NewFileWizard::addWizardsToMenu (PopupMenu& m) const
{
    for (int i = 0; i < wizards.size(); ++i)
        m.addItem (menuBaseID + i, "Add New " + wizards.getUnchecked(i)->getName() + "...");
}

bool NewFileWizard::runWizardFromMenu (int chosenMenuItemID, const Project::Item& projectGroupToAddTo) const
{
    if (Type* wiz = wizards [chosenMenuItemID - menuBaseID])
    {
        wiz->createNewFile (projectGroupToAddTo);
        return true;
    }

    return false;
}

void NewFileWizard::registerWizard (Type* newWizard)
{
    wizards.add (newWizard);
}
