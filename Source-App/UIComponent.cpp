/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.2.0

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "UIComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
UIComponent::UIComponent ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    setName ("UIComponent");
    addAndMakeVisible (appTitle = new Label ("appTitle",
                                             TRANS("C74 Gen Application")));
    appTitle->setFont (Font (20.00f, Font::bold));
    appTitle->setJustificationType (Justification::centred);
    appTitle->setEditable (false, false, false);
    appTitle->setColour (TextEditor::textColourId, Colours::black);
    appTitle->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (sliderHolder = new Component());
    sliderHolder->setName ("sliderHolder");

    addAndMakeVisible (sliderLabelHolder = new Component());
    sliderLabelHolder->setName ("sliderLabelHolder");


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

UIComponent::~UIComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    appTitle = nullptr;
    sliderHolder = nullptr;
    sliderLabelHolder = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void UIComponent::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::white);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void UIComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    appTitle->setBounds (8, 8, proportionOfWidth (0.9495f), proportionOfHeight (0.0707f));
    sliderHolder->setBounds (proportionOfWidth (0.6024f) - (proportionOfWidth (0.6995f) / 2), proportionOfHeight (0.0897f), proportionOfWidth (0.6995f), proportionOfHeight (0.9000f));
    sliderLabelHolder->setBounds (proportionOfWidth (0.1503f) - (proportionOfWidth (0.1995f) / 2), proportionOfHeight (0.0897f), proportionOfWidth (0.1995f), proportionOfHeight (0.9000f));
    //[UserResized] Add your own custom resize handling here..

	if (sliderHolder->getNumChildComponents()) {
		double sliderHeight = sliderHolder->getHeight() / sliderHolder->getNumChildComponents();

		for (long i = 0; i < sliderHolder->getNumChildComponents(); i++) {
			Component *slider = sliderHolder->getChildComponent(i);
			slider->setBounds(0, i * sliderHeight, sliderHolder->getWidth(), sliderHeight);
		}
	}

	if (sliderLabelHolder->getNumChildComponents()) {
		double sliderHeight = sliderLabelHolder->getHeight() / sliderLabelHolder->getNumChildComponents();

		for (long i = 0; i < sliderLabelHolder->getNumChildComponents(); i++) {
			Component *sliderLabel = sliderLabelHolder->getChildComponent(i);
			sliderLabel->setBounds(0, i * sliderHeight, sliderLabelHolder->getWidth(), sliderHeight);
		}
	}
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
	Component *UIComponent::getSliderHolder()
	{
		return sliderHolder;
	}
	Component *UIComponent::getSliderLabelHolder()
	{
		return sliderLabelHolder;
	}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="UIComponent" componentName="UIComponent"
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ffffffff"/>
  <LABEL name="appTitle" id="964815051e58ddb8" memberName="appTitle" virtualName=""
         explicitFocusOrder="0" pos="8 8 94.961% 7.014%" edTextCol="ff000000"
         edBkgCol="0" labelText="C74 Gen Application" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Default font"
         fontsize="20" bold="1" italic="0" justification="36"/>
  <GENERICCOMPONENT name="sliderHolder" id="2dc3fe12f5f2ce27" memberName="sliderHolder"
                    virtualName="" explicitFocusOrder="0" pos="60.19%c 8.937% 69.989% 90.045%"
                    class="Component" params=""/>
  <GENERICCOMPONENT name="sliderLabelHolder" id="7ba511cc4e3a05b3" memberName="sliderLabelHolder"
                    virtualName="" explicitFocusOrder="0" pos="15.006%c 8.937% 19.933% 90.045%"
                    class="Component" params=""/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]
