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

#include "../../jucer_Headers.h"
#include "../../Application/jucer_Application.h"
#include "../ui/jucer_JucerCommandIDs.h"
#include "jucer_PaintRoutineEditor.h"
#include "../jucer_ObjectTypes.h"
#include "jucer_JucerDocumentEditor.h"


//==============================================================================
PaintRoutineEditor::PaintRoutineEditor (PaintRoutine& pr, JucerDocument& doc,
                                        JucerDocumentEditor* docHolder)
    : graphics (pr),
      document (doc),
      documentHolder (docHolder),
      componentOverlay (nullptr),
      componentOverlayOpacity (0.0f)
{
    refreshAllElements();

    setSize (document.getInitialWidth(),
             document.getInitialHeight());
}

PaintRoutineEditor::~PaintRoutineEditor()
{
    document.removeChangeListener (this);
    removeAllElementComps();
    removeChildComponent (&lassoComp);
    deleteAllChildren();
}

void PaintRoutineEditor::removeAllElementComps()
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (PaintElement* const e = dynamic_cast <PaintElement*> (getChildComponent (i)))
            removeChildComponent (e);
}

Rectangle<int> PaintRoutineEditor::getComponentArea() const
{
    if (document.isFixedSize())
        return Rectangle<int> ((getWidth() - document.getInitialWidth()) / 2,
                               (getHeight() - document.getInitialHeight()) / 2,
                               document.getInitialWidth(),
                               document.getInitialHeight());

    return getLocalBounds().reduced (4);
}

//==============================================================================
void PaintRoutineEditor::paint (Graphics& g)
{
    const Rectangle<int> clip (getComponentArea());

    g.reduceClipRegion (clip);
    g.setOrigin (clip.getPosition());

    graphics.fillWithBackground (g, true);
    grid.draw (g, &graphics);
}

void PaintRoutineEditor::paintOverChildren (Graphics& g)
{
    if (componentOverlay.isNull() && document.getComponentOverlayOpacity() > 0.0f)
        updateComponentOverlay();

    if (componentOverlay.isValid())
    {
        const Rectangle<int> clip (getComponentArea());
        g.drawImageAt (componentOverlay, clip.getX(), clip.getY());
    }
}

void PaintRoutineEditor::resized()
{
    if (getWidth() > 0 && getHeight() > 0)
    {
        componentOverlay = Image();
        refreshAllElements();
    }
}

void PaintRoutineEditor::updateChildBounds()
{
    const Rectangle<int> clip (getComponentArea());

    for (int i = 0; i < getNumChildComponents(); ++i)
        if (PaintElement* const e = dynamic_cast <PaintElement*> (getChildComponent (i)))
            e->updateBounds (clip);
}

void PaintRoutineEditor::updateComponentOverlay()
{
    if (componentOverlay.isValid())
        repaint();

    componentOverlay = Image();
    componentOverlayOpacity = document.getComponentOverlayOpacity();

    if (componentOverlayOpacity > 0.0f)
    {
        if (documentHolder != nullptr)
            componentOverlay = documentHolder->createComponentLayerSnapshot();

        if (componentOverlay.isValid())
        {
            componentOverlay.multiplyAllAlphas (componentOverlayOpacity);
            repaint();
        }
    }
}

void PaintRoutineEditor::visibilityChanged()
{
    document.beginTransaction();

    if (isVisible())
    {
        refreshAllElements();
        document.addChangeListener (this);
    }
    else
    {
        document.removeChangeListener (this);
        componentOverlay = Image();
    }
}

void PaintRoutineEditor::refreshAllElements()
{
    for (int i = getNumChildComponents(); --i >= 0;)
        if (PaintElement* const e = dynamic_cast <PaintElement*> (getChildComponent (i)))
            if (! graphics.containsElement (e))
                removeChildComponent (e);

    Component* last = nullptr;

    for (int i = graphics.getNumElements(); --i >= 0;)
    {
        PaintElement* const e = graphics.getElement (i);

        addAndMakeVisible (e);

        if (last != nullptr)
            e->toBehind (last);
        else
            e->toFront (false);

        last = e;
    }

    updateChildBounds();

    if (grid.updateFromDesign (document))
        repaint();

    if (currentBackgroundColour != graphics.getBackgroundColour())
    {
        currentBackgroundColour = graphics.getBackgroundColour();
        repaint();
    }

    if (componentOverlayOpacity != document.getComponentOverlayOpacity())
    {
        componentOverlay = Image();
        componentOverlayOpacity = document.getComponentOverlayOpacity();
        repaint();
    }
}

void PaintRoutineEditor::changeListenerCallback (ChangeBroadcaster*)
{
    refreshAllElements();
}

void PaintRoutineEditor::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        ApplicationCommandManager* commandManager = &IntrojucerApp::getCommandManager();

        PopupMenu m;

        m.addCommandItem (commandManager, JucerCommandIDs::editCompLayout);
        m.addCommandItem (commandManager, JucerCommandIDs::editCompGraphics);
        m.addSeparator();

        for (int i = 0; i < ObjectTypes::numElementTypes; ++i)
            m.addCommandItem (commandManager, JucerCommandIDs::newElementBase + i);

        m.show();
    }
    else
    {
        addChildComponent (lassoComp);
        lassoComp.beginLasso (e, this);
    }
}

void PaintRoutineEditor::mouseDrag (const MouseEvent& e)
{
    lassoComp.toFront (false);
    lassoComp.dragLasso (e);
}

void PaintRoutineEditor::mouseUp (const MouseEvent& e)
{
    lassoComp.endLasso();

    if (e.mouseWasClicked() && ! e.mods.isAnyModifierKeyDown())
    {
        graphics.getSelectedElements().deselectAll();
        graphics.getSelectedPoints().deselectAll();
    }
}

void PaintRoutineEditor::findLassoItemsInArea (Array <PaintElement*>& results, const Rectangle<int>& lasso)
{
    for (int i = 0; i < getNumChildComponents(); ++i)
        if (PaintElement* const e = dynamic_cast <PaintElement*> (getChildComponent (i)))
            if (e->getBounds().expanded (-e->borderThickness).intersects (lasso))
                results.add (e);
}

SelectedItemSet <PaintElement*>& PaintRoutineEditor::getLassoSelection()
{
    return graphics.getSelectedElements();
}

bool PaintRoutineEditor::isInterestedInFileDrag (const StringArray& files)
{
    return File::createFileWithoutCheckingPath (files[0])
             .hasFileExtension ("jpg;jpeg;png;gif;svg");
}

void PaintRoutineEditor::filesDropped (const StringArray& filenames, int x, int y)
{
    const File f (filenames [0]);

    if (f.existsAsFile())
    {
        ScopedPointer<Drawable> d (Drawable::createFromImageFile (f));

        if (d != nullptr)
        {
            d = nullptr;

            document.beginTransaction();

            graphics.dropImageAt (f,
                                  jlimit (10, getWidth() - 10, x),
                                  jlimit (10, getHeight() - 10, y));

            document.beginTransaction();
        }
    }
}
