/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "C74_GENPLUGIN.h"
#include "UIComponent.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public AudioAppComponent, Slider::Listener
{
public:
    //==============================================================================
    MainContentComponent()
	:m_CurrentBufferSize(0)
	{
		// use a default samplerate and vector size here, reset it later
		m_C74PluginState = (CommonState *)C74_GENPLUGIN::create(44100, 64);
		C74_GENPLUGIN::reset(m_C74PluginState);

		m_InputBuffers = new t_sample *[C74_GENPLUGIN::num_inputs()];
		m_OutputBuffers = new t_sample *[C74_GENPLUGIN::num_outputs()];
		
		for (int i = 0; i < C74_GENPLUGIN::num_inputs(); i++) {
			m_InputBuffers[i] = NULL;
		}
		for (int i = 0; i < C74_GENPLUGIN::num_outputs(); i++) {
			m_OutputBuffers[i] = NULL;
		}
		
        // specify the number of input and output channels that we want to open
        setAudioChannels (getNumInputChannels(), getNumOutputChannels());
		
		m_uiComponent = new UIComponent();
		addAndMakeVisible(m_uiComponent);
		addSliders();
		
		Rectangle<int> r = Desktop::getInstance().getDisplays().getMainDisplay().userArea;
		setSize (r.getWidth(), r.getHeight());
    }

    ~MainContentComponent()
    {
        shutdownAudio();
		C74_GENPLUGIN::destroy(m_C74PluginState);
    }

    //=======================================================================
	
	long getNumInputChannels() const { return 2; }
	long getNumOutputChannels() const { return 2; }
	
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        // This function will be called when the audio device is started, or when
        // its settings (i.e. sample rate, block size, etc) are changed.

        // You can use this function to initialise any resources you might need,
        // but be careful - it will be called on the audio thread, not the GUI thread.

        // For more details, see the help for AudioProcessor::prepareToPlay()
		
		// initialize samplerate and vectorsize with the correct values
		m_C74PluginState->sr = sampleRate;
		m_C74PluginState->vs = samplesPerBlockExpected;
		
		assureBufferSize(samplesPerBlockExpected);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // Your audio-processing code goes here!

        // For more details, see the help for AudioProcessor::getNextAudioBlock()
		
		AudioSampleBuffer *buffer = bufferToFill.buffer;
		
		assureBufferSize(buffer->getNumSamples());
		
		// fill input buffers
		for (int i = 0; i < C74_GENPLUGIN::num_inputs(); i++) {
			if (i < getNumInputChannels()) {
				for (int j = 0; j < buffer->getNumSamples(); j++) {
					m_InputBuffers[i][j] = buffer->getReadPointer(i)[j];
				}
			} else {
				memset(m_InputBuffers[i], 0, m_CurrentBufferSize *  sizeof(double));
			}
		}
		
		// process audio
		C74_GENPLUGIN::perform(m_C74PluginState,
								  m_InputBuffers,
								  C74_GENPLUGIN::num_inputs(),
								  m_OutputBuffers,
								  C74_GENPLUGIN::num_outputs(),
								  buffer->getNumSamples());
		
		// fill output buffers
		for (int i = 0; i < getNumOutputChannels(); i++) {
			if (i < C74_GENPLUGIN::num_outputs()) {
				for (int j = 0; j < buffer->getNumSamples(); j++) {
					buffer->getWritePointer(i)[j] = m_OutputBuffers[i][j];
				}
			} else {
				buffer->clear (i, 0, buffer->getNumSamples());
			}
		}
    }

    void releaseResources() override
    {
        // This will be called when the audio device stops, or when it is being
        // restarted due to a setting change.

        // For more details, see the help for AudioProcessor::releaseResources()
    }

    //=======================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (Colours::black);


        // You can add your drawing code here!
    }

    void resized() override
    {
        // This is called when the MainContentComponent is resized.
        // If you add any child components, this is where you should
        // update their positions.

		m_uiComponent->setBounds(getBounds());
    }

	void addSliders()
	{
		Component *sliderHolder = m_uiComponent->getSliderHolder();
		
		if (sliderHolder) {
			for (long i = 0; i < C74_GENPLUGIN::num_params(); i++) {
				Slider *slider = new Slider();
				double min = 0;
				double max = 1;
				
				if (C74_GENPLUGIN::getparameterhasminmax(m_C74PluginState, i)) {
					min = C74_GENPLUGIN::getparametermin(m_C74PluginState, i);
					max = C74_GENPLUGIN::getparametermax(m_C74PluginState, i);
				}

				slider->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
				slider->setRange(min, max);
				slider->setName(String((int)i));
				slider->addListener(this);
				
				sliderHolder->addAndMakeVisible(slider);
			}
		}
		
		Component *sliderLabelHolder = m_uiComponent->getSliderLabelHolder();
		
		if (sliderLabelHolder) {
			for (long i = 0; i < C74_GENPLUGIN::num_params(); i++) {
				Label *sliderLabel = new Label();
				sliderLabel->setText(String(C74_GENPLUGIN::getparametername(m_C74PluginState, i)), NotificationType::dontSendNotification);
				sliderLabelHolder->addAndMakeVisible(sliderLabel);
			}
		}
		
		resized();
	}
	
	void sliderValueChanged (Slider* slider)
	{
		long index = atoi(slider->getName().getCharPointer());
		C74_GENPLUGIN::setparameter(m_C74PluginState, index, slider->getValue(), NULL);
	}
	

protected:
	// c74: since Juce does float sample processing and Gen offers double sample
	// processing, we need to go through input and output buffers
	void assureBufferSize(long bufferSize)
	{
		if (bufferSize > m_CurrentBufferSize) {
			for (int i = 0; i < C74_GENPLUGIN::num_inputs(); i++) {
				if (m_InputBuffers[i]) delete m_InputBuffers[i];
				m_InputBuffers[i] = new t_sample[bufferSize];
			}
			for (int i = 0; i < C74_GENPLUGIN::num_outputs(); i++) {
				if (m_OutputBuffers[i]) delete m_OutputBuffers[i];
				m_OutputBuffers[i] = new t_sample[bufferSize];
			}
			
			m_CurrentBufferSize = bufferSize;
		}
	}
private:
    //==============================================================================

    // Your private member variables go here...

	UIComponent				*m_uiComponent;
	
	CommonState				*m_C74PluginState;
	
	long					m_CurrentBufferSize;
	t_sample				**m_InputBuffers;
	t_sample				**m_OutputBuffers;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }


#endif  // MAINCOMPONENT_H_INCLUDED
