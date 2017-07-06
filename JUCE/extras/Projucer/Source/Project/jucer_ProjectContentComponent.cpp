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
#include "jucer_ProjectContentComponent.h"
#include "jucer_Module.h"
#include "../Application/jucer_MainWindow.h"
#include "../Application/jucer_Application.h"
#include "../Application/jucer_DownloadCompileEngineThread.h"
#include "../Code Editor/jucer_SourceCodeEditor.h"
#include "../Utility/jucer_FilePathPropertyComponent.h"

#include "jucer_HeaderComponent.h"
#include "jucer_ProjectTab.h"
#include "jucer_LiveBuildTab.h"

//==============================================================================
struct ContentViewport  : public Component
{
    ContentViewport (Component* content)
    {
        addAndMakeVisible (viewport);
        viewport.setViewedComponent (content, true);
    }

    void resized() override
    {
        viewport.setBounds (getLocalBounds());
    }

    Viewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContentViewport)
};

//==========================================================================
struct ProjectSettingsComp  : public Component,
                              private ChangeListener
{
    ProjectSettingsComp (Project& p)
        : project (p),
          group (project.getProjectFilenameRoot(), Icon (getIcons().settings, Colours::transparentBlack))
    {
        addAndMakeVisible (group);

        updatePropertyList();
        project.addChangeListener (this);
    }

    ~ProjectSettingsComp()
    {
        project.removeChangeListener (this);
    }

    void resized() override
    {
        group.updateSize (12, 0, getWidth() - 24);
        group.setBounds (getLocalBounds().reduced (12, 0));
    }

    void updatePropertyList()
    {
        PropertyListBuilder props;
        project.createPropertyEditors (props);
        group.setProperties (props);
        group.setName ("Project Settings");

        lastProjectType = project.getProjectTypeValue().getValue();
        parentSizeChanged();
    }

    void changeListenerCallback (ChangeBroadcaster*) override
    {
        if (lastProjectType != project.getProjectTypeValue().getValue())
            updatePropertyList();
    }

    void parentSizeChanged() override
    {
        const auto width = jmax (550, getParentWidth());
        auto y = group.updateSize (12, 0, width - 12);

        y = jmax (getParentHeight(), y);

        setSize (width, y);
    }

    Project& project;
    var lastProjectType;
    ConfigTreeItemTypes::PropertyGroupComponent group;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectSettingsComp)
};

//==============================================================================
struct LiveBuildSettingsComp  : public Component
{
    LiveBuildSettingsComp (Project& p)
        : group ("Live Build Settings", Icon (getIcons().settings, Colours::transparentBlack))
    {
        addAndMakeVisible (&group);

        PropertyListBuilder props;
        LiveBuildProjectSettings::getLiveSettings (p, props);

        group.setProperties (props);
        group.setName ("Live Build Settings");
    }

    void resized() override
    {
        group.updateSize (12, 0, getWidth() - 24);
        group.setBounds (getLocalBounds().reduced (12, 0));
    }

    void parentSizeChanged() override
    {
        const auto width = jmax (550, getParentWidth());
        auto y = group.updateSize (12, 0, width - 12);

        y = jmax (getParentHeight(), y);

        setSize (width, y);
    }

    ConfigTreeItemTypes::PropertyGroupComponent group;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveBuildSettingsComp)
};

//==============================================================================
struct LogoComponent  : public Component
{
    LogoComponent()
    {
        ScopedPointer<XmlElement> svg (XmlDocument::parse (BinaryData::background_logo_svg));
        logo = Drawable::createFromSVG (*svg);
    }

    void paint (Graphics& g) override
    {
        g.setColour (findColour (defaultTextColourId));

        Rectangle<int> r (getLocalBounds());

        g.setFont (15.0f);
        g.drawFittedText (getVersionInfo(), r.removeFromBottom (50), Justification::centredBottom, 3);

        logo->drawWithin (g, r.withTrimmedBottom (r.getHeight() / 4).toFloat(),
                          RectanglePlacement (RectanglePlacement::centred), 1.0f);
    }

    static String getVersionInfo()
    {
        return SystemStats::getJUCEVersion()
                + newLine
                + ProjucerApplication::getApp().getVersionDescription();
    }

    ScopedPointer<Drawable> logo;
};

//==============================================================================
ProjectContentComponent::ProjectContentComponent()
    : project (nullptr),
      currentDocument (nullptr),
      sidebarTabs (TabbedButtonBar::TabsAtTop)
{
    setOpaque (true);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (logo = new LogoComponent());
    addAndMakeVisible (header = new HeaderComponent());

    addAndMakeVisible (fileNameLabel = new Label());
    fileNameLabel->setJustificationType (Justification::centred);

    sidebarSizeConstrainer.setMinimumWidth (200);
    sidebarSizeConstrainer.setMaximumWidth (500);

    sidebarTabs.setOutline (0);
    sidebarTabs.getTabbedButtonBar().setMinimumTabScaleFactor (0.5);

    ProjucerApplication::getApp().openDocumentManager.addListener (this);

    Desktop::getInstance().addFocusChangeListener (this);
    startTimer (1600);
}

ProjectContentComponent::~ProjectContentComponent()
{
    Desktop::getInstance().removeFocusChangeListener (this);
    killChildProcess();

    ProjucerApplication::getApp().openDocumentManager.removeListener (this);

    logo = nullptr;
    header = nullptr;
    setProject (nullptr);
    contentView = nullptr;
    fileNameLabel = nullptr;
    removeChildComponent (&bubbleMessage);
    jassert (getNumChildComponents() <= 1);
}

void ProjectContentComponent::paint (Graphics& g)
{
    g.fillAll (findColour (backgroundColourId));
}

void ProjectContentComponent::resized()
{
    auto r = getLocalBounds();

    r.removeFromRight (10);
    r.removeFromLeft (15);
    r.removeFromBottom (40);
    r.removeFromTop (5);

    if (header != nullptr)
        header->setBounds (r.removeFromTop (40));

    r.removeFromTop (10);

    auto sidebarArea = r.removeFromLeft (sidebarTabs.getWidth() != 0 ? sidebarTabs.getWidth()
                                                                     : r.getWidth() / 4);

    if (sidebarTabs.isVisible())
        sidebarTabs.setBounds (sidebarArea);

    if (resizerBar != nullptr)
        resizerBar->setBounds (r.withWidth (4));

    if (auto* h = dynamic_cast<HeaderComponent*> (header.get()))
    {
        h->sidebarTabsWidthChanged (sidebarTabs.getWidth());
        r.removeFromRight (h->getUserButtonWidth());
    }

    if (contentView != nullptr)
    {
        if (fileNameLabel != nullptr && fileNameLabel->isVisible())
            fileNameLabel->setBounds (r.removeFromTop (15));

        contentView->setBounds (r);
    }

    if (logo != nullptr)
        logo->setBounds (r.reduced (r.getWidth() / 6, r.getHeight() / 6));
}

void ProjectContentComponent::lookAndFeelChanged()
{
    repaint();

    if (translationTool != nullptr)
        translationTool->repaint();
}

void ProjectContentComponent::childBoundsChanged (Component* child)
{
    if (child == &sidebarTabs)
        resized();
}

void ProjectContentComponent::setProject (Project* newProject)
{
    if (project != newProject)
    {
        lastCrashMessage = String();
        killChildProcess();

        if (project != nullptr)
            project->removeChangeListener (this);

        contentView = nullptr;
        resizerBar = nullptr;

        deleteProjectTabs();
        project = newProject;
        rebuildProjectTabs();
    }
}

//==============================================================================
LiveBuildTab* findBuildTab (const TabbedComponent& tabs)
{
    return dynamic_cast<LiveBuildTab*> (tabs.getTabContentComponent (1));
}

bool ProjectContentComponent::isBuildTabEnabled() const
{
    auto bt = findBuildTab (sidebarTabs);

    return bt != nullptr && bt->isEnabled;
}

void ProjectContentComponent::createProjectTabs()
{
    jassert (project != nullptr);

    const auto tabColour = Colours::transparentBlack;

    auto* pTab = new ProjectTab (project);
    sidebarTabs.addTab ("Project", tabColour, pTab, true);

    const CompileEngineChildProcess::Ptr childProc (getChildProcess());

    sidebarTabs.addTab ("Build", tabColour, new LiveBuildTab (childProc, lastCrashMessage), true);

    if (childProc != nullptr)
    {
        childProc->crashHandler = [this] (const String& m) { this->handleCrash (m); };

        sidebarTabs.getTabbedButtonBar().getTabButton (1)->setExtraComponent (new BuildStatusTabComp (childProc->errorList,
                                                                                                      childProc->activityList),
                                                                                                      TabBarButton::afterText);
    }
}

void ProjectContentComponent::deleteProjectTabs()
{
    if (project != nullptr && sidebarTabs.getNumTabs() > 0)
    {
        PropertiesFile& settings = project->getStoredProperties();

        if (sidebarTabs.getWidth() > 0)
            settings.setValue ("projectPanelWidth", sidebarTabs.getWidth());

        settings.setValue ("lastViewedTabIndex", sidebarTabs.getCurrentTabIndex());

        for (int i = 0; i < 3; ++i)
            settings.setValue ("projectTabPanelHeight" + String (i),
                               getProjectTab()->getPanelHeightProportion (i));
    }

    sidebarTabs.clearTabs();
}

void ProjectContentComponent::rebuildProjectTabs()
{
    deleteProjectTabs();

    if (project != nullptr)
    {
        addAndMakeVisible (sidebarTabs);
        createProjectTabs();

        //======================================================================
        auto& settings = project->getStoredProperties();

        auto lastTreeWidth = settings.getValue ("projectPanelWidth").getIntValue();
        if (lastTreeWidth < 150)
            lastTreeWidth = 240;

        sidebarTabs.setBounds (0, 0, lastTreeWidth, getHeight());

        sidebarTabs.setCurrentTabIndex (settings.getValue ("lastViewedTabIndex", "0").getIntValue());

        auto* projectTab = getProjectTab();
        for (int i = 2; i >= 0; --i)
            projectTab->setPanelHeightProportion (i, settings.getValue ("projectTabPanelHeight" + String (i), String ("1"))
                                                             .getFloatValue());

        //======================================================================
        addAndMakeVisible (resizerBar = new ResizableEdgeComponent (&sidebarTabs, &sidebarSizeConstrainer,
                                                                    ResizableEdgeComponent::rightEdge));
        resizerBar->setAlwaysOnTop (true);

        project->addChangeListener (this);

        updateMissingFileStatuses();

        if (auto* h = dynamic_cast<HeaderComponent*> (header.get()))
        {
            h->setVisible (true);
            h->setCurrentProject (project);
        }
    }
    else
    {
        sidebarTabs.setVisible (false);
        header->setVisible (false);
    }

    resized();
}

void ProjectContentComponent::saveTreeViewState()
{
}

void ProjectContentComponent::saveOpenDocumentList()
{
    if (project != nullptr)
    {
        ScopedPointer<XmlElement> xml (recentDocumentList.createXML());

        if (xml != nullptr)
            project->getStoredProperties().setValue ("lastDocs", xml);
    }
}

void ProjectContentComponent::reloadLastOpenDocuments()
{
    if (project != nullptr)
    {
        ScopedPointer<XmlElement> xml (project->getStoredProperties().getXmlValue ("lastDocs"));

        if (xml != nullptr)
        {
            recentDocumentList.restoreFromXML (*project, *xml);
            showDocument (recentDocumentList.getCurrentDocument(), true);
        }
    }
}

bool ProjectContentComponent::documentAboutToClose (OpenDocumentManager::Document* document)
{
    hideDocument (document);
    return true;
}

void ProjectContentComponent::changeListenerCallback (ChangeBroadcaster*)
{
    updateMissingFileStatuses();
}

void ProjectContentComponent::updateMissingFileStatuses()
{
    if (auto* pTab = getProjectTab())
        if (auto* tree = pTab->getFileTreePanel())
            tree->updateMissingFileStatuses();
}

bool ProjectContentComponent::showEditorForFile (const File& f, bool grabFocus)
{
    if (getCurrentFile() == f
            || showDocument (ProjucerApplication::getApp().openDocumentManager.openFile (project, f), grabFocus))
    {
        fileNameLabel->setText (f.getFileName(), dontSendNotification);

        return true;
    }

    return false;
}

bool ProjectContentComponent::hasFileInRecentList (const File& f) const
{
    return recentDocumentList.contains (f);
}

File ProjectContentComponent::getCurrentFile() const
{
    return currentDocument != nullptr ? currentDocument->getFile()
                                      : File();
}

bool ProjectContentComponent::showDocument (OpenDocumentManager::Document* doc, bool grabFocus)
{
    if (doc == nullptr)
        return false;

    if (doc->hasFileBeenModifiedExternally())
        doc->reloadFromFile();

    if (doc == getCurrentDocument() && contentView != nullptr)
    {
        if (grabFocus)
            contentView->grabKeyboardFocus();

        return true;
    }

    recentDocumentList.newDocumentOpened (doc);

    bool opened = setEditorComponent (doc->createEditor(), doc);

    if (opened && grabFocus && isShowing())
        contentView->grabKeyboardFocus();

    return opened;
}

void ProjectContentComponent::hideEditor()
{
    currentDocument = nullptr;
    contentView = nullptr;

    if (fileNameLabel != nullptr)
        fileNameLabel->setVisible (false);

    ProjucerApplication::getCommandManager().commandStatusChanged();
    resized();
}

void ProjectContentComponent::hideDocument (OpenDocumentManager::Document* doc)
{
    if (doc == currentDocument)
    {
        if (OpenDocumentManager::Document* replacement = recentDocumentList.getClosestPreviousDocOtherThan (doc))
            showDocument (replacement, true);
        else
            hideEditor();
    }
}

bool ProjectContentComponent::setEditorComponent (Component* editor,
                                                  OpenDocumentManager::Document* doc)
{
    if (editor != nullptr)
    {
        contentView = nullptr;

        if (doc == nullptr)
        {
            auto* viewport = new ContentViewport (editor);

            contentView = viewport;
            currentDocument = nullptr;
            fileNameLabel->setVisible (false);

            addAndMakeVisible (viewport);
        }
        else
        {
            contentView = editor;
            currentDocument = doc;
            fileNameLabel->setText (doc->getFile().getFileName(), dontSendNotification);
            fileNameLabel->setVisible (true);

            addAndMakeVisible (editor);
        }

        resized();

        ProjucerApplication::getCommandManager().commandStatusChanged();
        return true;
    }

    return false;
}

void ProjectContentComponent::closeDocument()
{
    if (currentDocument != nullptr)
        ProjucerApplication::getApp().openDocumentManager.closeDocument (currentDocument, true);
    else if (contentView != nullptr)
        if (! goToPreviousFile())
            hideEditor();
}

static void showSaveWarning (OpenDocumentManager::Document* currentDocument)
{
    AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                 TRANS("Save failed!"),
                                 TRANS("Couldn't save the file:")
                                   + "\n" + currentDocument->getFile().getFullPathName());
}

void ProjectContentComponent::saveDocument()
{
    if (currentDocument != nullptr)
    {
        if (! currentDocument->save())
            showSaveWarning (currentDocument);
    }
    else
        saveProject();
}

void ProjectContentComponent::saveAs()
{
    if (currentDocument != nullptr && ! currentDocument->saveAs())
        showSaveWarning (currentDocument);
}

bool ProjectContentComponent::goToPreviousFile()
{
    OpenDocumentManager::Document* doc = recentDocumentList.getCurrentDocument();

    if (doc == nullptr || doc == getCurrentDocument())
        doc = recentDocumentList.getPrevious();

    return showDocument (doc, true);
}

bool ProjectContentComponent::goToNextFile()
{
    return showDocument (recentDocumentList.getNext(), true);
}

bool ProjectContentComponent::canGoToCounterpart() const
{
    return currentDocument != nullptr
            && currentDocument->getCounterpartFile().exists();
}

bool ProjectContentComponent::goToCounterpart()
{
    if (currentDocument != nullptr)
    {
        const File file (currentDocument->getCounterpartFile());

        if (file.exists())
            return showEditorForFile (file, true);
    }

    return false;
}

bool ProjectContentComponent::saveProject (bool shouldWait)
{
    if (project != nullptr)
    {
        const ScopedValueSetter<bool> valueSetter (project->shouldWaitAfterSaving, shouldWait, false);
        return (project->save (true, true) == FileBasedDocument::savedOk);
    }

    return false;
}

void ProjectContentComponent::closeProject()
{
    if (auto* const mw = findParentComponentOfClass<MainWindow>())
        mw->closeCurrentProject();
}

void ProjectContentComponent::showProjectSettings()
{
    setEditorComponent (new ProjectSettingsComp (*project), nullptr);
}

void ProjectContentComponent::showCurrentExporterSettings()
{
    if (auto* h = dynamic_cast<HeaderComponent*> (header.get()))
        showExporterSettings (h->getSelectedExporterName());
}

void ProjectContentComponent::showExporterSettings (const String& exporterName)
{
    showExportersPanel();

    if (auto* exportersPanel = getProjectTab()->getExportersTreePanel())
    {
        if (auto* exporters = dynamic_cast<ExportersTreeRoot*>(exportersPanel->rootItem.get()))
        {
            for (int i = exporters->getNumSubItems(); i >= 0; --i)
            {
                if (auto* e = dynamic_cast<ConfigTreeItemTypes::ExporterItem*> (exporters->getSubItem (i)))
                {
                    if (e->getDisplayName() == exporterName)
                    {
                        if (e->isSelected())
                            e->setSelected (false, true);

                        e->setSelected (true, true);
                    }
                }
            }
        }
    }
}

void ProjectContentComponent::showModule (const String& moduleID)
{
    showModulesPanel();

    if (auto* modsPanel = getProjectTab()->getModuleTreePanel())
    {
        if (auto* mods = dynamic_cast<ConfigTreeItemTypes::EnabledModulesItem*> (modsPanel->rootItem.get()))
        {
            for (int i = mods->getNumSubItems(); --i >= 0;)
            {
                if (auto* m = dynamic_cast<ConfigTreeItemTypes::ModuleItem*> (mods->getSubItem (i)))
                {
                    if (m->moduleID == moduleID)
                    {
                        if (m->isSelected())
                            m->setSelected (false, true);

                        m->setSelected (true, true);
                    }
                }
            }
        }
    }
}

void ProjectContentComponent::showLiveBuildSettings()
{
    setEditorComponent (new LiveBuildSettingsComp (*project), nullptr);
}

void ProjectContentComponent::showUserSettings()
{
    if (auto* headerComp = dynamic_cast<HeaderComponent*> (header.get()))
        headerComp->showUserSettings();
}

StringArray ProjectContentComponent::getExportersWhichCanLaunch() const
{
    StringArray s;

    if (project != nullptr)
        for (Project::ExporterIterator exporter (*project); exporter.next();)
            if (exporter->canLaunchProject())
                s.add (exporter->getName());

    return s;
}

void ProjectContentComponent::openInSelectedIDE (bool saveFirst)
{
    if (project != nullptr)
    {
        if (auto* headerComp = dynamic_cast<HeaderComponent*> (header.get()))
        {
            auto selectedIDE = headerComp->getSelectedExporterName();

            for (Project::ExporterIterator exporter (*project); exporter.next();)
            {
                if (exporter->canLaunchProject() && exporter->getName() == selectedIDE)
                {
                    if (saveFirst)
                        saveProject (exporter->isXcode());

                    exporter->launchProject();
                    break;
                }
            }
        }
    }
}

static void newExporterMenuCallback (int result, ProjectContentComponent* comp)
{
    if (comp != nullptr && result > 0)
    {
        if (Project* p = comp->getProject())
        {
            String exporterName (ProjectExporter::getExporterNames() [result - 1]);

            if (exporterName.isNotEmpty())
                p->addNewExporter (exporterName);
        }
    }
}

void ProjectContentComponent::showNewExporterMenu()
{
    if (project != nullptr)
    {
        PopupMenu menu;

        menu.addSectionHeader ("Create a new export target:");

        Array<ProjectExporter::ExporterTypeInfo> exporters (ProjectExporter::getExporterTypes());

        for (int i = 0; i < exporters.size(); ++i)
        {
            const ProjectExporter::ExporterTypeInfo& type = exporters.getReference(i);

            menu.addItem (i + 1, type.name, true, false, type.getIcon());
        }

        menu.showMenuAsync (PopupMenu::Options(),
                            ModalCallbackFunction::forComponent (newExporterMenuCallback, this));
    }
}

void ProjectContentComponent::deleteSelectedTreeItems()
{
    if (auto* const tree = getProjectTab()->getTreeWithSelectedItems())
        tree->deleteSelectedItems();
}

void ProjectContentComponent::showBubbleMessage (Rectangle<int> pos, const String& text)
{
    addChildComponent (bubbleMessage);
    bubbleMessage.setColour (BubbleComponent::backgroundColourId, Colours::white.withAlpha (0.7f));
    bubbleMessage.setColour (BubbleComponent::outlineColourId, Colours::black.withAlpha (0.8f));
    bubbleMessage.setAlwaysOnTop (true);

    bubbleMessage.showAt (pos, AttributedString (text), 3000, true, false);
}

//==============================================================================
void ProjectContentComponent::showTranslationTool()
{
    if (translationTool != nullptr)
    {
        translationTool->toFront (true);
    }
    else if (project != nullptr)
    {
        new FloatingToolWindow ("Translation File Builder",
                                "transToolWindowPos",
                                new TranslationToolComponent(),
                                translationTool, true,
                                600, 700,
                                600, 400, 10000, 10000);
    }
}

//==============================================================================
struct AsyncCommandRetrier  : public Timer
{
    AsyncCommandRetrier (const ApplicationCommandTarget::InvocationInfo& i)  : info (i)
    {
        info.originatingComponent = nullptr;
        startTimer (500);
    }

    void timerCallback() override
    {
        stopTimer();
        ProjucerApplication::getCommandManager().invoke (info, true);
        delete this;
    }

    ApplicationCommandTarget::InvocationInfo info;

    JUCE_DECLARE_NON_COPYABLE (AsyncCommandRetrier)
};

bool reinvokeCommandAfterCancellingModalComps (const ApplicationCommandTarget::InvocationInfo& info)
{
    if (ModalComponentManager::getInstance()->cancelAllModalComponents())
    {
        new AsyncCommandRetrier (info);
        return true;
    }

    return false;
}

//==============================================================================
ApplicationCommandTarget* ProjectContentComponent::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void ProjectContentComponent::getAllCommands (Array <CommandID>& commands)
{
    const CommandID ids[] = { CommandIDs::saveProject,
                              CommandIDs::closeProject,
                              CommandIDs::saveDocument,
                              CommandIDs::saveDocumentAs,
                              CommandIDs::closeDocument,
                              CommandIDs::goToPreviousDoc,
                              CommandIDs::goToNextDoc,
                              CommandIDs::goToCounterpart,
                              CommandIDs::showProjectSettings,
                              CommandIDs::showProjectTab,
                              CommandIDs::showBuildTab,
                              CommandIDs::showFileExplorerPanel,
                              CommandIDs::showModulesPanel,
                              CommandIDs::showExportersPanel,
                              CommandIDs::showExporterSettings,
                              CommandIDs::openInIDE,
                              CommandIDs::saveAndOpenInIDE,
                              CommandIDs::createNewExporter,
                              CommandIDs::deleteSelectedItem,
                              CommandIDs::showTranslationTool,
                              CommandIDs::cleanAll,
                              CommandIDs::toggleBuildEnabled,
                              CommandIDs::buildNow,
                              CommandIDs::toggleContinuousBuild,
                              CommandIDs::launchApp,
                              CommandIDs::killApp,
                              CommandIDs::reinstantiateComp,
                              CommandIDs::showWarnings,
                              CommandIDs::nextError,
                              CommandIDs::prevError };

    commands.addArray (ids, numElementsInArray (ids));
}

void ProjectContentComponent::getCommandInfo (const CommandID commandID, ApplicationCommandInfo& result)
{
    String documentName;
    if (currentDocument != nullptr)
        documentName = " '" + currentDocument->getName().substring (0, 32) + "'";

   #if JUCE_MAC
    const ModifierKeys cmdCtrl (ModifierKeys::ctrlModifier | ModifierKeys::commandModifier);
   #else
    const ModifierKeys cmdCtrl (ModifierKeys::ctrlModifier | ModifierKeys::altModifier);
   #endif

    switch (commandID)
    {
    case CommandIDs::saveProject:
        result.setInfo ("Save Project",
                        "Saves the current project",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        break;

    case CommandIDs::closeProject:
        result.setInfo ("Close Project",
                        "Closes the current project",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        break;

    case CommandIDs::saveDocument:
        result.setInfo ("Save" + documentName,
                        "Saves the current document",
                        CommandCategories::general, 0);
        result.setActive (currentDocument != nullptr || project != nullptr);
        result.defaultKeypresses.add (KeyPress ('s', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::saveDocumentAs:
        result.setInfo ("Save As...",
                        "Saves the current document to a new location",
                        CommandCategories::general, 0);
        result.setActive (currentDocument != nullptr);
        result.defaultKeypresses.add (KeyPress ('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        break;

    case CommandIDs::closeDocument:
        result.setInfo ("Close" + documentName,
                        "Closes the current document",
                        CommandCategories::general, 0);
        result.setActive (contentView != nullptr);
        result.defaultKeypresses.add (KeyPress ('w', cmdCtrl, 0));
        break;

    case CommandIDs::goToPreviousDoc:
        result.setInfo ("Previous Document",
                        "Go to previous document",
                        CommandCategories::general, 0);
        result.setActive (recentDocumentList.canGoToPrevious());
        result.defaultKeypresses.add (KeyPress (KeyPress::leftKey, cmdCtrl, 0));
        break;

    case CommandIDs::goToNextDoc:
        result.setInfo ("Next Document",
                        "Go to next document",
                        CommandCategories::general, 0);
        result.setActive (recentDocumentList.canGoToNext());
        result.defaultKeypresses.add (KeyPress (KeyPress::rightKey, cmdCtrl, 0));
        break;

    case CommandIDs::goToCounterpart:
        result.setInfo ("Open Counterpart File",
                        "Open corresponding header or cpp file",
                        CommandCategories::general, 0);
        result.setActive (canGoToCounterpart());
        result.defaultKeypresses.add (KeyPress (KeyPress::upKey, cmdCtrl, 0));
        break;

    case CommandIDs::showProjectSettings:
        result.setInfo ("Show Project Settings",
                        "Shows the main project options page",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.defaultKeypresses.add (KeyPress ('p', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        break;

    case CommandIDs::showProjectTab:
        result.setInfo ("Show Project Tab",
                        "Shows the tab containing the project information",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.defaultKeypresses.add (KeyPress ('p', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::showBuildTab:
        result.setInfo ("Show Build Tab",
                        "Shows the tab containing the build panel",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        break;

    case CommandIDs::showFileExplorerPanel:
        result.setInfo ("Show File Explorer Panel",
                        "Shows the panel containing the tree of files for this project",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.defaultKeypresses.add (KeyPress ('f', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::showModulesPanel:
        result.setInfo ("Show Modules Panel",
                        "Shows the panel containing the project's list of modules",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.defaultKeypresses.add (KeyPress ('m', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::showExportersPanel:
        result.setInfo ("Show Exporters Panel",
                        "Shows the panel containing the project's list of exporters",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.defaultKeypresses.add (KeyPress ('e', ModifierKeys::commandModifier, 0));
        break;

    case CommandIDs::showExporterSettings:
        result.setInfo ("Show Exporter Settings",
                        "Shows the settings page for the currently selected exporter",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.defaultKeypresses.add (KeyPress ('e', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        break;

    case CommandIDs::openInIDE:
        result.setInfo ("Open in IDE...",
                        "Launches the project in an external IDE",
                        CommandCategories::general, 0);
        result.setActive (ProjectExporter::canProjectBeLaunched (project));
        break;

    case CommandIDs::saveAndOpenInIDE:
        result.setInfo ("Save Project and Open in IDE...",
                        "Saves the project and launches it in an external IDE",
                        CommandCategories::general, 0);
        result.setActive (ProjectExporter::canProjectBeLaunched (project));
        result.defaultKeypresses.add (KeyPress ('l', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        break;

    case CommandIDs::createNewExporter:
        result.setInfo ("Create New Exporter...",
                        "Creates a new exporter for a compiler type",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        break;

    case CommandIDs::deleteSelectedItem:
        result.setInfo ("Delete Selected File",
                        String(),
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress (KeyPress::deleteKey, 0, 0));
        result.defaultKeypresses.add (KeyPress (KeyPress::backspaceKey, 0, 0));
        result.setActive (sidebarTabs.getCurrentTabIndex() == 0);
        break;

    case CommandIDs::showTranslationTool:
        result.setInfo ("Translation File Builder",
                        "Shows the translation file helper tool",
                        CommandCategories::general, 0);
        break;

    case CommandIDs::cleanAll:
        result.setInfo ("Clean All",
                        "Cleans all intermediate files",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('k', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        result.setActive (project != nullptr);
        break;

    case CommandIDs::toggleBuildEnabled:
        result.setInfo ("Enable Compilation",
                        "Enables/disables the compiler",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('b', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        result.setActive (project != nullptr);
        result.setTicked (childProcess != nullptr);
        break;

    case CommandIDs::buildNow:
        result.setInfo ("Build Now",
                        "Recompiles any out-of-date files and updates the JIT engine",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('b', ModifierKeys::commandModifier, 0));
        result.setActive (childProcess != nullptr);
        break;

    case CommandIDs::toggleContinuousBuild:
        result.setInfo ("Enable Continuous Recompiling",
                        "Continuously recompiles any changes made in code editors",
                        CommandCategories::general, 0);
        result.setActive (childProcess != nullptr);
        result.setTicked (isContinuousRebuildEnabled());
        break;

    case CommandIDs::launchApp:
        result.setInfo ("Launch Application",
                        "Invokes the app's main() function",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('r', ModifierKeys::commandModifier, 0));
        result.setActive (childProcess != nullptr && childProcess->canLaunchApp());
        break;

    case CommandIDs::killApp:
        result.setInfo ("Stop Application",
                        "Kills the app if it's running",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('.', ModifierKeys::commandModifier, 0));
        result.setActive (childProcess != nullptr && childProcess->canKillApp());
        break;

    case CommandIDs::reinstantiateComp:
        result.setInfo ("Re-instantiate Components",
                        "Re-loads any component editors that are open",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('r', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
        result.setActive (childProcess != nullptr);
        break;

    case CommandIDs::showWarnings:
        result.setInfo ("Show Warnings",
                        "Shows or hides compilation warnings",
                        CommandCategories::general, 0);
        result.setActive (project != nullptr);
        result.setTicked (areWarningsEnabled());
        break;

    case CommandIDs::nextError:
        result.setInfo ("Highlight next error",
                        "Jumps to the next error or warning",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('\'', ModifierKeys::commandModifier, 0));
        result.setActive (childProcess != nullptr && ! childProcess->errorList.isEmpty());
        break;

    case CommandIDs::prevError:
        result.setInfo ("Highlight previous error",
                        "Jumps to the last error or warning",
                        CommandCategories::general, 0);
        result.defaultKeypresses.add (KeyPress ('\"', ModifierKeys::commandModifier, 0));
        result.setActive (childProcess != nullptr && ! childProcess->errorList.isEmpty());
        break;

    default:
        break;
    }
}

bool ProjectContentComponent::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case CommandIDs::saveProject:
        case CommandIDs::closeProject:
        case CommandIDs::saveDocument:
        case CommandIDs::saveDocumentAs:
        case CommandIDs::closeDocument:
        case CommandIDs::goToPreviousDoc:
        case CommandIDs::goToNextDoc:
        case CommandIDs::goToCounterpart:
        case CommandIDs::saveAndOpenInIDE:
            if (reinvokeCommandAfterCancellingModalComps (info))
            {
                grabKeyboardFocus(); // to force any open labels to close their text editors
                return true;
            }

            break;

        default:
            break;
    }

    if (isCurrentlyBlockedByAnotherModalComponent())
        return false;

    switch (info.commandID)
    {
        case CommandIDs::saveProject:               saveProject();      break;
        case CommandIDs::closeProject:              closeProject();     break;
        case CommandIDs::saveDocument:              saveDocument();     break;
        case CommandIDs::saveDocumentAs:            saveAs();           break;
        case CommandIDs::closeDocument:             closeDocument();    break;
        case CommandIDs::goToPreviousDoc:           goToPreviousFile(); break;
        case CommandIDs::goToNextDoc:               goToNextFile();     break;
        case CommandIDs::goToCounterpart:           goToCounterpart();  break;

        case CommandIDs::showProjectSettings:       showProjectSettings();         break;
        case CommandIDs::showProjectTab:            showProjectTab();              break;
        case CommandIDs::showBuildTab:              showBuildTab();                break;
        case CommandIDs::showFileExplorerPanel:     showFilesPanel();              break;
        case CommandIDs::showModulesPanel:          showModulesPanel();            break;
        case CommandIDs::showExportersPanel:        showExportersPanel();          break;
        case CommandIDs::showExporterSettings:      showCurrentExporterSettings(); break;

        case CommandIDs::openInIDE:                 openInSelectedIDE (false); break;
        case CommandIDs::saveAndOpenInIDE:          openInSelectedIDE (true);  break;

        case CommandIDs::createNewExporter:         showNewExporterMenu(); break;

        case CommandIDs::deleteSelectedItem:        deleteSelectedTreeItems(); break;

        case CommandIDs::showTranslationTool:       showTranslationTool(); break;

        case CommandIDs::cleanAll:                  cleanAll();                           break;
        case CommandIDs::toggleBuildEnabled:        setBuildEnabled (! isBuildEnabled()); break;
        case CommandIDs::buildNow:                  rebuildNow();                         break;
        case CommandIDs::toggleContinuousBuild:     setContinuousRebuildEnabled (! isContinuousRebuildEnabled()); break;
        case CommandIDs::launchApp:                 launchApp();                          break;
        case CommandIDs::killApp:                   killApp();                            break;
        case CommandIDs::reinstantiateComp:         reinstantiateLivePreviewWindows();    break;
        case CommandIDs::showWarnings:              toggleWarnings();                     break;
        case CommandIDs::nextError:                 showNextError();                      break;
        case CommandIDs::prevError:                 showPreviousError();                  break;

        default:
            return false;
    }

    return true;
}

void ProjectContentComponent::getSelectedProjectItemsBeingDragged (const DragAndDropTarget::SourceDetails& dragSourceDetails,
                                                                   OwnedArray<Project::Item>& selectedNodes)
{
    FileTreeItemTypes::ProjectTreeItemBase::getSelectedProjectItemsBeingDragged (dragSourceDetails, selectedNodes);
}

//==============================================================================
void ProjectContentComponent::killChildProcess()
{
    if (childProcess != nullptr)
    {
        deleteProjectTabs();
        childProcess = nullptr;
        ProjucerApplication::getApp().childProcessCache->removeOrphans();
    }
}

void ProjectContentComponent::setBuildEnabled (bool b)
{
    if (project != nullptr && b != isBuildEnabled())
    {
        LiveBuildProjectSettings::setBuildDisabled (*project, ! b);
        killChildProcess();
        refreshTabsIfBuildStatusChanged();

        if (auto* h = dynamic_cast<HeaderComponent*> (header.get()))
            h->updateBuildButtons (b, isContinuousRebuildEnabled());
    }
}

void ProjectContentComponent::cleanAll()
{
    lastCrashMessage = String();

    if (childProcess != nullptr)
        childProcess->cleanAll();
    else if (Project* p = getProject())
        CompileEngineChildProcess::cleanAllCachedFilesForProject (*p);
}

void ProjectContentComponent::handleCrash (const String& message)
{
    lastCrashMessage = message.isEmpty() ? TRANS("JIT process stopped responding!")
                                         : (TRANS("JIT process crashed!") + ":\n\n" + message);

    if (project != nullptr)
    {
        setBuildEnabled (false);
        showBuildTab();
    }
}

bool ProjectContentComponent::isBuildEnabled() const
{
    return project != nullptr && ! LiveBuildProjectSettings::isBuildDisabled (*project)
            && CompileEngineDLL::getInstance()->isLoaded();
}

void ProjectContentComponent::refreshTabsIfBuildStatusChanged()
{
    if (project != nullptr
         && (sidebarTabs.getNumTabs() < 2
            || isBuildEnabled() != isBuildTabEnabled()))
        rebuildProjectTabs();
}

bool ProjectContentComponent::areWarningsEnabled() const
{
    return project != nullptr && ! LiveBuildProjectSettings::areWarningsDisabled (*project);
}

void ProjectContentComponent::updateWarningState()
{
    if (childProcess != nullptr)
        childProcess->errorList.setWarningsEnabled (areWarningsEnabled());
}

void ProjectContentComponent::toggleWarnings()
{
    if (project != nullptr)
    {
        LiveBuildProjectSettings::setWarningsDisabled (*project, areWarningsEnabled());
        updateWarningState();
    }
}

static ProjucerAppClasses::ErrorListComp* findErrorListComp (const TabbedComponent& tabs)
{
    if (LiveBuildTab* bt = findBuildTab (tabs))
        return bt->errorListComp;

    return nullptr;
}

void ProjectContentComponent::showNextError()
{
    if (ProjucerAppClasses::ErrorListComp* el = findErrorListComp (sidebarTabs))
    {
        showBuildTab();
        el->showNext();
    }
}

void ProjectContentComponent::showPreviousError()
{
    if (ProjucerAppClasses::ErrorListComp* el = findErrorListComp (sidebarTabs))
    {
        showBuildTab();
        el->showPrevious();
    }
}

void ProjectContentComponent::reinstantiateLivePreviewWindows()
{
    if (childProcess != nullptr)
        childProcess->reinstantiatePreviews();
}

void ProjectContentComponent::launchApp()
{
    if (childProcess != nullptr)
        childProcess->launchApp();
}

void ProjectContentComponent::killApp()
{
    if (childProcess != nullptr)
        childProcess->killApp();
}

void ProjectContentComponent::rebuildNow()
{
    if (childProcess != nullptr)
        childProcess->flushEditorChanges();
}

void ProjectContentComponent::globalFocusChanged (Component* focusedComponent)
{
    const bool nowForeground = (Process::isForegroundProcess()
                                  && (focusedComponent == this || isParentOf (focusedComponent)));

    if (nowForeground != isForeground)
    {
        isForeground = nowForeground;

        if (childProcess != nullptr)
            childProcess->processActivationChanged (isForeground);
    }
}

void ProjectContentComponent::timerCallback()
{
    if (! isBuildEnabled())
        killChildProcess();

    refreshTabsIfBuildStatusChanged();
}

bool ProjectContentComponent::isContinuousRebuildEnabled()
{
    return getAppSettings().getGlobalProperties().getBoolValue ("continuousRebuild", true);
}

void ProjectContentComponent::setContinuousRebuildEnabled (bool b)
{
    if (childProcess != nullptr)
    {
        childProcess->setContinuousRebuild (b);

        if (auto* h = dynamic_cast<HeaderComponent*> (header.get()))
            h->updateBuildButtons (isBuildEnabled(), b);

        getAppSettings().getGlobalProperties().setValue ("continuousRebuild", b);

        ProjucerApplication::getCommandManager().commandStatusChanged();
    }
}

ReferenceCountedObjectPtr<CompileEngineChildProcess> ProjectContentComponent::getChildProcess()
{
    if (childProcess == nullptr && isBuildEnabled())
    {
        childProcess = ProjucerApplication::getApp().childProcessCache->getOrCreate (*project);

        if (childProcess != nullptr)
            childProcess->setContinuousRebuild (isContinuousRebuildEnabled());
    }

    return childProcess;
}

void ProjectContentComponent::handleMissingSystemHeaders()
{
   #if JUCE_MAC
    const String tabMessage = "Compiler not available due to missing system headers\nPlease install a recent version of Xcode";
    const String alertWindowMessage = "Missing system headers\nPlease install a recent version of Xcode";
   #elif JUCE_WINDOWS
    const String tabMessage = "Compiler not available due to missing system headers\nPlease install a recent version of Visual Studio and the Windows Desktop SDK";
    const String alertWindowMessage = "Missing system headers\nPlease install a recent version of Visual Studio and the Windows Desktop SDK";
   #elif JUCE_LINUX
    const String tabMessage = "Compiler not available due to missing system headers\nPlease do a sudo apt-get install ...";
    const String alertWindowMessage = "Missing system headers\nPlease do sudo apt-get install ...";
   #endif

    setBuildEnabled (false);

    deleteProjectTabs();
    createProjectTabs();

    if (auto* bt = getLiveBuildTab())
    {
        bt->isEnabled = false;
        bt->errorMessage = tabMessage;
    }

    showBuildTab();

    AlertWindow::showMessageBox (AlertWindow::AlertIconType::WarningIcon,
                                 "Missing system headers", alertWindowMessage);
}

//==============================================================================
void ProjectContentComponent::showProjectPanel (const int index)
{
    showProjectTab();

    if (auto* pTab = getProjectTab())
        pTab->showPanel (index);
}

ProjectTab* ProjectContentComponent::getProjectTab()
{
    return dynamic_cast<ProjectTab*> (sidebarTabs.getTabContentComponent (0));
}

LiveBuildTab* ProjectContentComponent::getLiveBuildTab()
{
    return dynamic_cast<LiveBuildTab*> (sidebarTabs.getTabContentComponent (1));
}
