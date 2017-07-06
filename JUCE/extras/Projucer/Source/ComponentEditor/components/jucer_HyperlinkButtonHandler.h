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

class HyperlinkButtonHandler  : public ButtonHandler
{
public:
    HyperlinkButtonHandler()
        : ButtonHandler ("Hyperlink Button", "HyperlinkButton", typeid (HyperlinkButton), 150, 24)
    {
        registerColour (HyperlinkButton::textColourId, "text", "textCol");
    }

    Component* createNewComponent (JucerDocument*)
    {
        HyperlinkButton* hb = new HyperlinkButton ("new hyperlink", URL ("http://www.juce.com"));

        setNeedsButtonListener (hb, false);
        return hb;
    }

    void getEditableProperties (Component* component, JucerDocument& document, Array<PropertyComponent*>& props)
    {
        HyperlinkButton* const hb = (HyperlinkButton*) component;
        ButtonHandler::getEditableProperties (component, document, props);
        props.add (new HyperlinkURLProperty (hb, document));
        addColourProperties (component, document, props);
    }

    XmlElement* createXmlFor (Component* comp, const ComponentLayout* layout)
    {
        HyperlinkButton* const hb = (HyperlinkButton*) comp;
        XmlElement* const e = ButtonHandler::createXmlFor (comp, layout);
        e->setAttribute ("url", hb->getURL().toString (false));
        return e;
    }

    bool restoreFromXml (const XmlElement& xml, Component* comp, const ComponentLayout* layout)
    {
        HyperlinkButton* const hb = (HyperlinkButton*) comp;

        if (! ButtonHandler::restoreFromXml (xml, comp, layout))
            return false;

        hb->setURL (URL (xml.getStringAttribute ("url", hb->getURL().toString (false))));

        return true;
    }

    String getCreationParameters (GeneratedCode& code, Component* comp)
    {
        HyperlinkButton* const hb = dynamic_cast<HyperlinkButton*> (comp);

        return quotedString (hb->getButtonText(), code.shouldUseTransMacro())
                + ",\nURL ("
                + quotedString (hb->getURL().toString (false), false)
                + ")";
    }

    void fillInCreationCode (GeneratedCode& code, Component* component, const String& memberVariableName)
    {
        ButtonHandler::fillInCreationCode (code, component, memberVariableName);

        code.constructorCode << getColourIntialisationCode (component, memberVariableName)
                             << '\n';
    }

private:
    //==============================================================================
    class HyperlinkURLProperty  : public ComponentTextProperty <HyperlinkButton>
    {
    public:
        HyperlinkURLProperty (HyperlinkButton* comp, JucerDocument& doc)
            : ComponentTextProperty <HyperlinkButton> ("URL", 512, false, comp, doc)
        {}

        void setText (const String& newText) override
        {
            document.perform (new HyperlinkURLChangeAction (component, *document.getComponentLayout(), URL (newText)),
                              "Change hyperlink URL");
        }

        String getText() const override
        {
            return component->getURL().toString (false);
        }

    private:
        class HyperlinkURLChangeAction  : public ComponentUndoableAction <HyperlinkButton>
        {
        public:
            HyperlinkURLChangeAction (HyperlinkButton* const comp, ComponentLayout& l, const URL& newState_)
                : ComponentUndoableAction <HyperlinkButton> (comp, l),
                  newState (newState_)
            {
                oldState = comp->getURL();
            }

            bool perform()
            {
                showCorrectTab();
                getComponent()->setURL (newState);
                changed();
                return true;
            }

            bool undo()
            {
                showCorrectTab();
                getComponent()->setURL (oldState);
                changed();
                return true;
            }

            URL newState, oldState;
        };
    };
};
