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

class ExporterItem   : public ConfigTreeItemBase
{
public:
    ExporterItem (Project& p, ProjectExporter* e, int index)
        : project (p), exporter (e), configListTree (exporter->getConfigurations()),
          exporterIndex (index)
    {
        configListTree.addListener (this);
        jassert (exporter != nullptr);
    }

    bool canBeSelected() const override       { return true; }
    bool mightContainSubItems() override      { return exporter->getNumConfigurations() > 0; }
    String getUniqueName() const override     { return "exporter_" + String (exporterIndex); }
    String getRenamingName() const override   { return getDisplayName(); }
    String getDisplayName() const override    { return exporter->getName(); }
    void setName (const String&) override     {}
    bool isMissing() override                 { return false; }
    Icon getIcon() const override             { return Icon (getIcons().exporter, getContrastingColour (0.5f)); }
    void showDocument() override              { showSettingsPage (new SettingsComp (exporter)); }

    void deleteItem() override
    {
        if (AlertWindow::showOkCancelBox (AlertWindow::WarningIcon, "Delete Exporter",
                                          "Are you sure you want to delete this export target?"))
        {
            closeSettingsPage();
            ValueTree parent (exporter->settings.getParent());
            parent.removeChild (exporter->settings, project.getUndoManagerFor (parent));
        }
    }

    void addSubItems() override
    {
        for (ProjectExporter::ConfigIterator config (*exporter); config.next();)
            addSubItem (new ConfigItem (config.config, exporter->getName()));
    }

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Add a new configuration");
        menu.addSeparator();
        menu.addItem (2, "Delete this exporter");

        launchPopupMenu (menu);
    }

    void handlePopupMenuResult (int resultCode) override
    {
        if (resultCode == 2)
            deleteAllSelectedItems();
        else if (resultCode == 1)
            exporter->addNewConfiguration (nullptr);
    }

    var getDragSourceDescription() override
    {
        return getParentItem()->getUniqueName() + "/" + String (exporterIndex);
    }

    bool isInterestedInDragSource (const DragAndDropTarget::SourceDetails& dragSourceDetails) override
    {
        return dragSourceDetails.description.toString().startsWith (getUniqueName());
    }

    void itemDropped (const DragAndDropTarget::SourceDetails& dragSourceDetails, int insertIndex) override
    {
        const int oldIndex = indexOfConfig (dragSourceDetails.description.toString().fromLastOccurrenceOf ("||", false, false));

        if (oldIndex >= 0)
            configListTree.moveChild (oldIndex, insertIndex, project.getUndoManagerFor (configListTree));
    }

    int indexOfConfig (const String& configName)
    {
        int i = 0;
        for (ProjectExporter::ConfigIterator config (*exporter); config.next(); ++i)
            if (config->getName() == configName)
                return i;

        return -1;
    }

    //==============================================================================
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree&) override         { refreshIfNeeded (parentTree); }
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree&, int) override  { refreshIfNeeded (parentTree); }
    void valueTreeChildOrderChanged (ValueTree& parentTree, int, int) override    { refreshIfNeeded (parentTree); }

    void refreshIfNeeded (ValueTree& changedTree)
    {
        if (changedTree == configListTree)
            refreshSubItems();
    }

private:
    Project& project;
    ScopedPointer<ProjectExporter> exporter;
    ValueTree configListTree;
    int exporterIndex;

    //==============================================================================
    class SettingsComp  : public Component
    {
    public:
        SettingsComp (ProjectExporter* exp)
        {
            addAndMakeVisible (group);

            PropertyListBuilder props;
            exp->createPropertyEditors (props);
            group.setProperties (props);
            group.setName ("Export target: " + exp->getName());
            parentSizeChanged();
        }

        void parentSizeChanged() override  { updateSize (*this, group); }

    private:
        PropertyGroupComponent group;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComp)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExporterItem)
};


//==============================================================================
class ConfigItem   : public ConfigTreeItemBase
{
public:
    ConfigItem (const ProjectExporter::BuildConfiguration::Ptr& conf, const String& expName)
        : config (conf), exporterName (expName), configTree (config->config)
    {
        jassert (config != nullptr);
        configTree.addListener (this);
    }

    bool isMissing() override                       { return false; }
    bool canBeSelected() const override             { return true; }
    bool mightContainSubItems() override            { return false; }
    String getUniqueName() const override           { return "config_" + config->getName(); }
    String getRenamingName() const override         { return getDisplayName(); }
    String getDisplayName() const override          { return config->getName(); }
    void setName (const String&) override           {}
    Icon getIcon() const override                   { return Icon (getIcons().config, getContrastingColour (Colours::green, 0.5f)); }

    void showDocument() override                    { showSettingsPage (new SettingsComp (config, exporterName)); }
    void itemOpennessChanged (bool) override        {}

    void deleteItem() override
    {
        if (AlertWindow::showOkCancelBox (AlertWindow::WarningIcon, "Delete Configuration",
                                          "Are you sure you want to delete this configuration?"))
        {
            closeSettingsPage();
            config->removeFromExporter();
        }
    }

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Create a copy of this configuration");
        menu.addSeparator();
        menu.addItem (2, "Delete this configuration");

        launchPopupMenu (menu);
    }

    void handlePopupMenuResult (int resultCode) override
    {
        if (resultCode == 2)
        {
            deleteAllSelectedItems();
        }
        else if (resultCode == 1)
        {
            for (Project::ExporterIterator exporter (config->project); exporter.next();)
            {
                if (config->config.isAChildOf (exporter->settings))
                {
                    exporter->addNewConfiguration (config);
                    break;
                }
            }
        }
    }

    var getDragSourceDescription() override
    {
        return getParentItem()->getUniqueName() + "||" + config->getName();
    }

    void valueTreePropertyChanged (ValueTree&, const Identifier&) override  { repaintItem(); }

private:
    ProjectExporter::BuildConfiguration::Ptr config;
    String exporterName;
    ValueTree configTree;

    //==============================================================================
    class SettingsComp  : public Component
    {
    public:
        SettingsComp (ProjectExporter::BuildConfiguration* conf, const String& expName)
        {
            addAndMakeVisible (group);

            PropertyListBuilder props;
            conf->createPropertyEditors (props);
            group.setProperties (props);
            group.setName (expName + " / " + conf->getName());
            parentSizeChanged();
        }

        void parentSizeChanged() override  { updateSize (*this, group); }

    private:
        PropertyGroupComponent group;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComp)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConfigItem)
};
