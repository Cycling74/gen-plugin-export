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

#include "../JuceLibraryCode/JuceHeader.h"
#include "LightpadComponent.h"

//==============================================================================
/**
    A struct that handles the setup and layout of the DrumPadGridProgram
*/
struct ColourGrid
{
    ColourGrid (int cols, int rows)
        : numColumns (cols),
          numRows (rows)
    {
        constructGridFillArray();
    }

    /** Creates a GridFill object for each pad in the grid and sets its colour
        and fill before adding it to an array of GridFill objects
    */
    void constructGridFillArray()
    {
        gridFillArray.clear();

        int counter = 0;

        for (int i = 0; i < numColumns; ++i)
        {
            for (int j = 0; j < numRows; ++j)
            {
                DrumPadGridProgram::GridFill fill;
                Colour colourToUse = colourArray.getUnchecked (counter);

                fill.colour = colourToUse.withBrightness (colourToUse == currentColour ? 1.0f : 0.1f);

                if (colourToUse == Colours::black)
                    fill.fillType = DrumPadGridProgram::GridFill::FillType::hollow;
                else
                    fill.fillType = DrumPadGridProgram::GridFill::FillType::filled;

                gridFillArray.add (fill);

                if (++counter == colourArray.size())
                    counter = 0;
            }
        }
    }

    /** Sets which colour should be active for a given touch co-ordinate. Returns
        true if the colour has changed
    */
    bool setActiveColourForTouch (int x, int y)
    {
        bool colourHasChanged = false;

        int xindex = x / 5;
        int yindex = y / 5;

        Colour newColour = colourArray.getUnchecked ((yindex * 3) + xindex);
        if (currentColour != newColour)
        {
            currentColour = newColour;
            constructGridFillArray();
            colourHasChanged = true;
        }

        return colourHasChanged;
    }

    //==============================================================================
    int numColumns, numRows;

    Array<DrumPadGridProgram::GridFill> gridFillArray;

    Array<Colour> colourArray = { Colours::white, Colours::red, Colours::green,
                                  Colours::blue, Colours::hotpink, Colours::orange,
                                  Colours::magenta, Colours::cyan, Colours::black };

    Colour currentColour = Colours::hotpink;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourGrid)
};

//==============================================================================
/**
    The main component
*/
class MainComponent   : public Component,
                        public TopologySource::Listener,
                        private TouchSurface::Listener,
                        private ControlButton::Listener,
                        private LightpadComponent::Listener,
                        private Button::Listener,
                        private Slider::Listener,
                        private Timer
{
public:
    MainComponent()
    {
        activeLeds.clear();

        // Register MainContentComponent as a listener to the PhysicalTopologySource object
        topologySource.addListener (this);

        infoLabel.setText ("Connect a Lightpad Block to draw.", dontSendNotification);
        infoLabel.setJustificationType (Justification::centred);
        addAndMakeVisible (infoLabel);

        addAndMakeVisible (lightpadComponent);
        lightpadComponent.setVisible (false);
        lightpadComponent.addListener (this);

        clearButton.setButtonText ("Clear");
        clearButton.addListener (this);
        clearButton.setAlwaysOnTop (true);
        addAndMakeVisible (clearButton);

        brightnessSlider.setRange (0.0, 1.0);
        brightnessSlider.setValue (1.0);
        brightnessSlider.setAlwaysOnTop (true);
        brightnessSlider.setTextBoxStyle (Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
        brightnessSlider.addListener (this);
        addAndMakeVisible (brightnessSlider);

        brightnessLED.setAlwaysOnTop (true);
        brightnessLED.setColour (layout.currentColour.withBrightness (static_cast<float> (brightnessSlider.getValue())));
        addAndMakeVisible (brightnessLED);

       #if JUCE_IOS
        connectButton.setButtonText ("Connect");
        connectButton.addListener (this);
        connectButton.setAlwaysOnTop (true);
        addAndMakeVisible (connectButton);
       #endif

        setSize (600, 600);
    }

    ~MainComponent()
    {
        if (activeBlock != nullptr)
            detachActiveBlock();

        lightpadComponent.removeListener (this);
    }

    void paint (Graphics& g) override
    {
    }

    void resized() override
    {
        infoLabel.centreWithSize (getWidth(), 100);

        Rectangle<int> bounds = getLocalBounds().reduced (20);

        // top buttons
        Rectangle<int> topButtonArea = bounds.removeFromTop (getHeight() / 20);

        topButtonArea.removeFromLeft (20);
        clearButton.setBounds (topButtonArea.removeFromLeft (80));

       #if JUCE_IOS
        topButtonArea.removeFromRight (20);
        connectButton.setBounds (topButtonArea.removeFromRight (80));
       #endif

        bounds.removeFromTop (20);

        // brightness controls
        Rectangle<int> brightnessControlBounds;

        Desktop::DisplayOrientation orientation = Desktop::getInstance().getCurrentOrientation();

        if (orientation == Desktop::DisplayOrientation::upright || orientation == Desktop::DisplayOrientation::upsideDown)
        {
            brightnessControlBounds = bounds.removeFromBottom (getHeight() / 10);

            brightnessSlider.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
            brightnessLED.setBounds (brightnessControlBounds.removeFromLeft (getHeight() / 10));
            brightnessSlider.setBounds (brightnessControlBounds);
        }
        else
        {
            brightnessControlBounds = bounds.removeFromRight (getWidth() / 10);

            brightnessSlider.setSliderStyle (Slider::SliderStyle::LinearVertical);
            brightnessLED.setBounds (brightnessControlBounds.removeFromTop (getWidth() / 10));
            brightnessSlider.setBounds (brightnessControlBounds);
        }

        // lightpad component
        int sideLength = jmin (bounds.getWidth() - 40, bounds.getHeight() - 40);
        lightpadComponent.centreWithSize (sideLength, sideLength);
    }

    /** Overridden from TopologySource::Listener. Called when the topology changes */
    void topologyChanged() override
    {
        lightpadComponent.setVisible (false);
        infoLabel.setVisible (true);

        // Reset the activeBlock object
        if (activeBlock != nullptr)
            detachActiveBlock();

        // Get the array of currently connected Block objects from the PhysicalTopologySource
        Block::Array blocks = topologySource.getCurrentTopology().blocks;

        // Iterate over the array of Block objects
        for (auto b : blocks)
        {
            // Find the first Lightpad
            if (b->getType() == Block::Type::lightPadBlock)
            {
                activeBlock = b;

                // Register MainContentComponent as a listener to the touch surface
                if (auto surface = activeBlock->getTouchSurface())
                    surface->addListener (this);

                // Register MainContentComponent as a listener to any buttons
                for (auto button : activeBlock->getButtons())
                    button->addListener (this);

                // Get the LEDGrid object from the Lightpad and set its program to the program for the current mode
                if (auto grid = activeBlock->getLEDGrid())
                {
                    // Work out scale factors to translate X and Y touches to LED indexes
                    scaleX = (float) (grid->getNumColumns()) / activeBlock->getWidth();
                    scaleY = (float) (grid->getNumRows())    / activeBlock->getHeight();

                    setLEDProgram (*activeBlock);
                }

                // Make the on screen Lighpad component visible
                lightpadComponent.setVisible (true);
                infoLabel.setVisible (false);

                break;
            }
        }
    }

private:
    /** Overridden from TouchSurface::Listener. Called when a Touch is received on the Lightpad */
    void touchChanged (TouchSurface&, const TouchSurface::Touch& touch) override
    {
        // Translate X and Y touch events to LED indexes
        int xLed = roundToInt (touch.x * scaleX);
        int yLed = roundToInt (touch.y * scaleY);

        if (currentMode == colourPalette)
        {
            if (layout.setActiveColourForTouch (xLed, yLed))
            {
                colourPaletteProgram->setGridFills (layout.numColumns, layout.numRows, layout.gridFillArray);
                brightnessLED.setColour (layout.currentColour.withBrightness (layout.currentColour == Colours::black ? 0.0f
                                                                                                                     : static_cast<float> (brightnessSlider.getValue())));
            }
        }
        else if (currentMode == canvas)
        {
            drawLED ((uint32) xLed, (uint32) yLed, touch.z, layout.currentColour);
        }
    }

    /** Overridden from ControlButton::Listener. Called when a button on the Lightpad is pressed */
    void buttonPressed (ControlButton&, Block::Timestamp) override { }

    /** Overridden from ControlButton::Listener. Called when a button on the Lightpad is released */
    void buttonReleased (ControlButton&, Block::Timestamp) override
    {
        if (currentMode == canvas)
        {
            // Wait 500ms to see if there is a second press
            if (! isTimerRunning())
                startTimer (500);
            else
                doublePress = true;
        }
        else if (currentMode == colourPalette)
        {
            // Switch to canvas mode and set the LEDGrid program
            currentMode = canvas;
            setLEDProgram (*activeBlock);
        }
    }

    void buttonClicked (Button* b) override
    {
       #if JUCE_IOS
        if (b == &connectButton)
        {
            BluetoothMidiDevicePairingDialogue::open();
            return;
        }
       #else
        ignoreUnused (b);
       #endif

        clearLEDs();
    }

    void sliderValueChanged (Slider* s) override
    {
        if (s == &brightnessSlider)
            brightnessLED.setColour (layout.currentColour.withBrightness (layout.currentColour == Colours::black ? 0.0f
                                                                                                                 : static_cast<float> (brightnessSlider.getValue())));
    }

    void timerCallback() override
    {
        if (doublePress)
        {
            clearLEDs();

            // Reset the doublePress flag
            doublePress = false;
        }
        else
        {
            // Switch to colour palette mode and set the LEDGrid program
            currentMode = colourPalette;
            setLEDProgram (*activeBlock);
        }

        stopTimer();
    }

    void ledClicked (int x, int y, float z) override
    {
        drawLED ((uint32) x, (uint32) y, z == 0.0f ?     static_cast<float> (brightnessSlider.getValue())
                                                   : z * static_cast<float> (brightnessSlider.getValue()), layout.currentColour);
    }

    /** Removes TouchSurface and ControlButton listeners and sets activeBlock to nullptr */
    void detachActiveBlock()
    {
        if (auto surface = activeBlock->getTouchSurface())
            surface->removeListener (this);

        for (auto button : activeBlock->getButtons())
            button->removeListener (this);

        activeBlock = nullptr;
    }

    /** Sets the LEDGrid Program for the selected mode */
    void setLEDProgram (Block& block)
    {
        canvasProgram = nullptr;
        colourPaletteProgram = nullptr;

        if (currentMode == canvas)
        {
            // Create a new BitmapLEDProgram for the LEDGrid
            canvasProgram = new BitmapLEDProgram (block);

            // Set the LEDGrid program
            block.setProgram (canvasProgram);

            // Redraw any previously drawn LEDs
            redrawLEDs();
        }
        else if (currentMode == colourPalette)
        {
            // Create a new DrumPadGridProgram for the LEDGrid
            colourPaletteProgram = new DrumPadGridProgram (block);

            // Set the LEDGrid program
            block.setProgram (colourPaletteProgram);

            // Setup the grid layout
            colourPaletteProgram->setGridFills (layout.numColumns,
                                                layout.numRows,
                                                layout.gridFillArray);
        }
    }

    void clearLEDs()
    {
        // Clear the LED grid
        for (uint32 x = 0; x < 15; ++x)
        {
            for (uint32 y = 0; y < 15; ++ y)
            {
                if (canvasProgram != nullptr)
                    canvasProgram->setLED (x, y, Colours::black);

                lightpadComponent.setLEDColour (x, y, Colours::black);
            }
        }

        // Clear the ActiveLED array
        activeLeds.clear();
    }

    /** Sets an LED on the Lightpad for a given touch co-ordinate and pressure */
    void drawLED (uint32 x0, uint32 y0, float z, Colour drawColour)
    {
        // Check if the activeLeds array already contains an ActiveLED object for this LED
        auto index = getLEDAt (x0, y0);

        // If the colour is black then just set the LED to black and return
        if (drawColour == Colours::black)
        {
            if (index >= 0)
            {
                if (canvasProgram != nullptr)
                    canvasProgram->setLED (x0, y0, Colours::black);

                lightpadComponent.setLEDColour (x0, y0, Colours::black);

                activeLeds.remove (index);
            }

            return;
        }

        // If there is no ActiveLED obejct for this LED then create one,
        // add it to the array, set the LED on the Block and return
        if (index < 0)
        {
            ActiveLED led;
            led.x = x0;
            led.y = y0;
            led.colour = drawColour;
            led.brightness = z;

            activeLeds.add (led);

            if (canvasProgram != nullptr)
                canvasProgram->setLED (led.x, led.y, led.colour.withBrightness (led.brightness));

            lightpadComponent.setLEDColour (led.x, led.y, led.colour.withBrightness (led.brightness));

            return;
        }

        // Get the ActiveLED object for this LED
        ActiveLED currentLed = activeLeds.getReference (index);

        // If the LED colour is the same as the draw colour, add the brightnesses together.
        // If it is different, blend the colours
        if (currentLed.colour == drawColour)
            currentLed.brightness = jmin (currentLed.brightness + z, 1.0f);
        else
            currentLed.colour = currentLed.colour.interpolatedWith (drawColour, z);


        // Set the LED on the Block and change the ActiveLED object in the activeLeds array
        if (canvasProgram != nullptr)
            canvasProgram->setLED (currentLed.x, currentLed.y, currentLed.colour.withBrightness (currentLed.brightness));

        lightpadComponent.setLEDColour (currentLed.x, currentLed.y, currentLed.colour.withBrightness (currentLed.brightness));

        activeLeds.set (index, currentLed);
    }

    /** Redraws the LEDs on the Lightpad from the activeLeds array */
    void redrawLEDs()
    {
        // Iterate over the activeLeds array and set the LEDs on the Block
        for (auto led : activeLeds)
        {
            canvasProgram->setLED (led.x, led.y, led.colour.withBrightness (led.brightness));
            lightpadComponent.setLEDColour (led.x, led.y, led.colour.withBrightness (led.brightness));
        }
    }

    /**
        A struct that represents an active LED on the Lightpad.
        Has a position, colour and brightness.
    */
    struct ActiveLED
    {
        uint32 x, y;
        Colour colour;
        float brightness;

        /** Returns true if this LED occupies the given co-ordinates */
        bool occupies (uint32 xPos, uint32 yPos) const
        {
            return xPos == x && yPos == y;
        }
    };
    Array<ActiveLED> activeLeds;

    int getLEDAt (uint32 x, uint32 y) const
    {
        for (int i = 0; i < activeLeds.size(); ++i)
            if (activeLeds.getReference(i).occupies (x, y))
                return i;

        return -1;
    }

    //==============================================================================
    enum DisplayMode
    {
        colourPalette = 0,
        canvas
    };
    DisplayMode currentMode = colourPalette;

    //==============================================================================
    BitmapLEDProgram* canvasProgram = nullptr;
    DrumPadGridProgram* colourPaletteProgram = nullptr;

    ColourGrid layout { 3, 3 };
    PhysicalTopologySource topologySource;
    Block::Ptr activeBlock;

    float scaleX = 0.0;
    float scaleY = 0.0;

    bool doublePress = false;

    //==============================================================================
    Label infoLabel;
    LightpadComponent lightpadComponent;
    TextButton clearButton;
    LEDComponent brightnessLED;
    Slider brightnessSlider;

   #if JUCE_IOS
    TextButton connectButton;
   #endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
