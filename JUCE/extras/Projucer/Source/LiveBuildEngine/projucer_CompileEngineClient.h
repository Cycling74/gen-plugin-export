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

#include "projucer_ActivityList.h"
#include "projucer_ErrorList.h"
class Project;

//==============================================================================
class CompileEngineChildProcess  : public ReferenceCountedObject,
                                   private OpenDocumentManager::DocumentCloseListener
{
public:
    CompileEngineChildProcess (Project&);
    ~CompileEngineChildProcess();

    bool openedOk() const       { return process != nullptr; }

    void editorOpened (const File& file, CodeDocument& document);
    bool documentAboutToClose (OpenDocumentManager::Document*) override;

    void cleanAll();
    void openPreview (const ClassDatabase::Class&);
    void reinstantiatePreviews();
    void processActivationChanged (bool isForeground);

    bool canLaunchApp() const;
    void launchApp();
    bool canKillApp() const;
    void killApp();

    const ClassDatabase::ClassList& getComponentList() const        { return lastComponentList; }

    void setContinuousRebuild (bool continuousBuild);
    void flushEditorChanges();

    static void cleanAllCachedFilesForProject (Project&);

    Project& project;
    ActivityList activityList;
    ErrorList errorList;

    std::function<void (const String&)> crashHandler;

    //==============================================================================
    // from server..
    void handleNewDiagnosticList (const ValueTree& newList);
    void handleClearErrors();
    void handleActivityListChanged (const StringArray&);
    void handleClassListChanged (const ValueTree& newList);
    void handleBuildFailed();
    void handleChangeCode (const SourceCodeRange& location, const String& newText);
    void handleAppLaunched();
    void handleAppQuit();
    void handleHighlightCode (const SourceCodeRange& location);
    void handlePing();
    void handleCrash (const String& message);
    void handleCloseIDE();
    void handleKeyPress (const String& className, const KeyPress& key);
    void handleUndoInEditor (const String& className);
    void handleRedoInEditor (const String& className);
    void handleMissingSystemHeaders();

    typedef ReferenceCountedObjectPtr<CompileEngineChildProcess> Ptr;

private:
    //==============================================================================
    class ChildProcess;
    ScopedPointer<ChildProcess> process, runningAppProcess;
    ClassDatabase::ClassList lastComponentList;
    bool continuousRebuild;

    struct Editor;
    OwnedArray<Editor> editors;
    void updateAllEditors();

    void createProcess();
    Editor* getOrOpenEditorFor (const File&);
    ProjectContentComponent* findProjectContentComponent() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompileEngineChildProcess)
};

//==============================================================================
struct ChildProcessCache
{
    ChildProcessCache() {}

    CompileEngineChildProcess::Ptr getExisting (Project& project) const noexcept
    {
        for (CompileEngineChildProcess* p : processes)
            if (&(p->project) == &project)
                return p;

        return nullptr;
    }

    CompileEngineChildProcess::Ptr getOrCreate (Project& project)
    {
        CompileEngineChildProcess::Ptr p (getExisting (project));

        if (p == nullptr)
        {
            p = new CompileEngineChildProcess (project);
            tellNewProcessAboutExistingEditors (p);
            processes.add (p);
        }

        return p;
    }

    static void tellNewProcessAboutExistingEditors (CompileEngineChildProcess* process)
    {
        OpenDocumentManager& odm = ProjucerApplication::getApp().openDocumentManager;

        for (int i = odm.getNumOpenDocuments(); --i >= 0;)
            if (SourceCodeDocument* d = dynamic_cast<SourceCodeDocument*> (odm.getOpenDocument (i)))
                process->editorOpened (d->getFile(), d->getCodeDocument());
    }

    void removeOrphans()
    {
        for (int i = processes.size(); --i >= 0;)
            if (processes.getObjectPointerUnchecked (i)->getReferenceCount() <= 1)
                processes.remove (i);
    }

private:
    ReferenceCountedArray<CompileEngineChildProcess> processes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChildProcessCache)
};

//==============================================================================
struct LiveBuildProjectSettings
{
    static void getLiveSettings (Project&, PropertyListBuilder&);
    static void updateNewlyOpenedProject (Project& p);

    static bool isBuildDisabled (Project&);
    static void setBuildDisabled (Project&, bool);

    static bool areWarningsDisabled (Project&);
    static void setWarningsDisabled (Project&, bool);
};
