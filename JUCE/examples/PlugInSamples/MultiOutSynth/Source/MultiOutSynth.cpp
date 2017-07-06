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

#include "../JuceLibraryCode/JuceHeader.h"
#include "../../GenericEditor.h"

//==============================================================================
/**
 */
class MultiOutSynth  : public AudioProcessor
{
public:
    enum
    {
        maxMidiChannel = 16,
        maxNumberOfVoices = 5
    };

    //==============================================================================
    MultiOutSynth()
        : AudioProcessor (BusesProperties()
                          .withOutput ("Output #1",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output #2",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #3",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #4",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #5",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #6",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #7",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #8",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #9",  AudioChannelSet::stereo(), false)
                          .withOutput ("Output #10", AudioChannelSet::stereo(), false)
                          .withOutput ("Output #11", AudioChannelSet::stereo(), false)
                          .withOutput ("Output #12", AudioChannelSet::stereo(), false)
                          .withOutput ("Output #13", AudioChannelSet::stereo(), false)
                          .withOutput ("Output #14", AudioChannelSet::stereo(), false)
                          .withOutput ("Output #15", AudioChannelSet::stereo(), false)
                          .withOutput ("Output #16", AudioChannelSet::stereo(), false))
    {
        // initialize other stuff (not related to buses)
        formatManager.registerBasicFormats();

        for (int midiChannel = 0; midiChannel < maxMidiChannel; ++midiChannel)
        {
            synth.add (new Synthesiser());

            for (int i = 0; i < maxNumberOfVoices; ++i)
                synth[midiChannel]->addVoice (new SamplerVoice());
        }

        loadNewSample (BinaryData::singing_ogg, BinaryData::singing_oggSize);
    }

    ~MultiOutSynth() {}

    //==============================================================================
    bool canAddBus    (bool isInput) const override   { return (! isInput && getBusCount (false) < maxMidiChannel); }
    bool canRemoveBus (bool isInput) const override   { return (! isInput && getBusCount (false) > 1); }

    //==============================================================================
    void prepareToPlay (double newSampleRate, int samplesPerBlock) override
    {
        ignoreUnused (samplesPerBlock);

        for (int midiChannel = 0; midiChannel < maxMidiChannel; ++midiChannel)
            synth[midiChannel]->setCurrentPlaybackSampleRate (newSampleRate);
    }

    void releaseResources() override {}

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiBuffer) override
    {
        const int busCount = getBusCount (false);
        for (int busNr = 0; busNr < busCount; ++busNr)
        {
            MidiBuffer midiChannelBuffer = filterMidiMessagesForChannel (midiBuffer, busNr + 1);
            AudioSampleBuffer audioBusBuffer = getBusBuffer (buffer, false, busNr);

            synth [busNr]->renderNextBlock (audioBusBuffer, midiChannelBuffer, 0, audioBusBuffer.getNumSamples());
        }
    }

    //==============================================================================
    AudioProcessorEditor* createEditor() override { return new GenericEditor (*this); }
    bool hasEditor() const override               { return true;   }

    //==============================================================================
    const String getName() const override               { return "Gain PlugIn"; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0; }
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const String getProgramName (int) override             { return String(); }
    void changeProgramName (int , const String& ) override { }

    //==============================================================================
    void getStateInformation (MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

private:
    //==============================================================================
    static MidiBuffer filterMidiMessagesForChannel (const MidiBuffer& input, int channel)
    {
        MidiMessage msg;
        int samplePosition;
        MidiBuffer output;

        for (MidiBuffer::Iterator it (input); it.getNextEvent (msg, samplePosition);)
            if (msg.getChannel() == channel) output.addEvent (msg, samplePosition);

        return output;
    }

    void loadNewSample (const void* data, int dataSize)
    {
        MemoryInputStream* soundBuffer = new MemoryInputStream (data, static_cast<std::size_t> (dataSize), false);
        ScopedPointer<AudioFormatReader> formatReader (formatManager.findFormatForFileExtension ("ogg")->createReaderFor (soundBuffer, true));

        BigInteger midiNotes;
        midiNotes.setRange (0, 126, true);
        SynthesiserSound::Ptr newSound = new SamplerSound ("Voice", *formatReader, midiNotes, 0x40, 0.0, 0.0, 10.0);

        for (int channel = 0; channel < maxMidiChannel; ++channel)
            synth[channel]->removeSound (0);

        sound = newSound;

        for (int channel = 0; channel < maxMidiChannel; ++channel)
            synth[channel]->addSound (sound);
    }

    //==============================================================================
    AudioFormatManager formatManager;
    OwnedArray<Synthesiser> synth;
    SynthesiserSound::Ptr sound;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiOutSynth)
};

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultiOutSynth();
}
