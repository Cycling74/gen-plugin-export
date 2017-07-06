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
#include "jucer_SlidingPanelComponent.h"


struct SlidingPanelComponent::DotButton  : public Button
{
    DotButton (SlidingPanelComponent& sp, int pageIndex)
        : Button (String()), owner (sp), index (pageIndex) {}

    void paintButton (Graphics& g, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        g.setColour (findColour (defaultButtonBackgroundColourId));
        const auto r = getLocalBounds().reduced (getWidth() / 4).toFloat();

        if (index == owner.getCurrentTabIndex())
            g.fillEllipse (r);
        else
            g.drawEllipse (r, 1.0f);
    }

    void clicked() override
    {
        owner.goToTab (index);
    }

    SlidingPanelComponent& owner;
    int index;
};

//==============================================================================
SlidingPanelComponent::SlidingPanelComponent()
    : currentIndex (0), dotSize (20)
{
    addAndMakeVisible (pageHolder);
}

SlidingPanelComponent::~SlidingPanelComponent()
{
}

SlidingPanelComponent::PageInfo::~PageInfo()
{
    if (shouldDelete)
        content.deleteAndZero();
}

void SlidingPanelComponent::addTab (const String& tabName,
                                    Component* const contentComponent,
                                    const bool deleteComponentWhenNotNeeded,
                                    const int insertIndex)
{
    PageInfo* page = new PageInfo();
    pages.insert (insertIndex, page);
    page->content = contentComponent;
    addAndMakeVisible (page->dotButton = new DotButton (*this, pages.indexOf (page)));
    page->name = tabName;
    page->shouldDelete = deleteComponentWhenNotNeeded;

    pageHolder.addAndMakeVisible (contentComponent);
    resized();
}

void SlidingPanelComponent::goToTab (int targetTabIndex)
{
    currentIndex = targetTabIndex;

    Desktop::getInstance().getAnimator()
        .animateComponent (&pageHolder, pageHolder.getBounds().withX (-targetTabIndex * getWidth()),
                           1.0f, 600, false, 0.0, 0.0);

    repaint();
}

void SlidingPanelComponent::resized()
{
    pageHolder.setBounds (-currentIndex * getWidth(), pageHolder.getPosition().y,
                          getNumTabs() * getWidth(), getHeight());

    Rectangle<int> content (getLocalBounds());

    Rectangle<int> dotHolder = content.removeFromBottom (20 + dotSize)
                                 .reduced ((content.getWidth() - dotSize * getNumTabs()) / 2, 10);

    for (int i = 0; i < getNumTabs(); ++i)
        pages.getUnchecked(i)->dotButton->setBounds (dotHolder.removeFromLeft (dotSize));

    for (int i = pages.size(); --i >= 0;)
        if (Component* c = pages.getUnchecked(i)->content)
            c->setBounds (content.translated (i * content.getWidth(), 0));
}
