/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <mutex>

//==============================================================================
class MainContentComponent   : public AudioAppComponent,
                               private Timer
{
public:
    //==============================================================================
    MainContentComponent()
    {
        setSize (400, 400);
        setAudioChannels (0, 2);

        initGui();
        Desktop::getInstance().setScreenSaverEnabled (false);
        startTimer (1000);
    }

    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int bufferSize, double sampleRate) override
    {
        currentSampleRate = sampleRate;
        allocateBuffers (static_cast<size_t> (bufferSize));
        printHeader();
    }

    void releaseResources() override
    {
        a.clear();
        b.clear();
        c.clear();
        currentSampleRate = 0.0;
    }

    //==============================================================================
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        const double startTimeMs = getPreciseTimeMs();

        AudioBuffer<float>& outputAudio = *bufferToFill.buffer;
        std::size_t bufferSize = (std::size_t) outputAudio.getNumSamples();
        initialiseBuffers (bufferToFill, bufferSize);

        for (int ch = 0; ch < outputAudio.getNumChannels(); ++ch)
            crunchSomeNumbers (outputAudio.getWritePointer (ch), bufferSize, numLoopIterationsPerCallback);

        std::lock_guard<std::mutex> lock (metricMutex);

        double endTimeMs = getPreciseTimeMs();
        addCallbackMetrics (startTimeMs, endTimeMs);
    }

    //==============================================================================
    void addCallbackMetrics (double startTimeMs, double endTimeMs)
    {
        double runtimeMs = endTimeMs - startTimeMs;
        audioCallbackRuntimeMs.addValue (runtimeMs);

        if (runtimeMs > getPhysicalTimeLimitMs())
            numCallbacksOverPhysicalTimeLimit++;

        if (lastCallbackStartTimeMs > 0.0)
        {
            double gapMs = startTimeMs - lastCallbackStartTimeMs;
            audioCallbackGapMs.addValue (gapMs);

            if (gapMs > 1.5 * getPhysicalTimeLimitMs())
                numLateCallbacks++;
        }

        lastCallbackStartTimeMs = startTimeMs;
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
        g.setFont (Font (16.0f));
        g.setColour (Colours::white);
        g.drawText ("loop iterations / audio callback",
                    getLocalBounds().withY (loopIterationsSlider.getHeight()), Justification::centred, true);
    }

    //==============================================================================
    void resized() override
    {
        loopIterationsSlider.setBounds (getLocalBounds().withSizeKeepingCentre (proportionOfWidth (0.9f), 50));
    }

private:
    //==============================================================================
    void initGui()
    {
        loopIterationsSlider.setSliderStyle (Slider::LinearBar);
        loopIterationsSlider.setRange (0, 30000, 250);
        loopIterationsSlider.setValue (15000);
        loopIterationsSlider.setColour (Slider::thumbColourId, Colours::white);
        loopIterationsSlider.setColour (Slider::textBoxTextColourId, Colours::grey);
        updateNumLoopIterationsPerCallback();
        addAndMakeVisible (loopIterationsSlider);
    }

    //==============================================================================
    void allocateBuffers (std::size_t bufferSize)
    {
        a.resize (bufferSize);
        b.resize (bufferSize);
        c.resize (bufferSize);
    }

    //==============================================================================
    void initialiseBuffers (const AudioSourceChannelInfo& bufferToFill, std::size_t bufferSize)
    {
        if (bufferSize != a.size())
        {
            jassertfalse;
            Logger::writeToLog ("WARNING: Unexpected buffer size received."
                                "expected: " + String (a.size()) +
                                ", actual: " + String (bufferSize));

            if (bufferSize > a.size())
                Logger::writeToLog ("WARNING: Need to allocate larger buffers on audio thread!");

            allocateBuffers (bufferSize);
        }

        bufferToFill.clearActiveBufferRegion();
        std::fill (a.begin(), a.end(), 0.09f);
        std::fill (b.begin(), b.end(), 0.1f );
        std::fill (c.begin(), c.end(), 0.11f);
    }

    //==============================================================================
    void crunchSomeNumbers (float* outBuffer, std::size_t bufferSize, int numIterations) noexcept
    {
        jassert (a.size() == bufferSize && b.size() == bufferSize && c.size() == bufferSize);

        for (int i = 0; i < numIterations; ++i)
        {
            FloatVectorOperations::multiply (c.data(), a.data(), b.data(), (int) bufferSize);
            FloatVectorOperations::addWithMultiply (outBuffer, b.data(), c.data(), (int) bufferSize);
        }
    }

    //==============================================================================
    void timerCallback() override
    {
        printAndResetPerformanceMetrics();
    }

    //==============================================================================
    void printHeader() const
    {
        Logger::writeToLog ("buffer size = " + String (a.size()) + " samples");
        Logger::writeToLog ("sample rate = " + String (currentSampleRate) + " Hz");
        Logger::writeToLog ("physical time limit / callback = " + String (getPhysicalTimeLimitMs() )+ " ms");
        Logger::writeToLog ("");
        Logger::writeToLog ("         | callback exec time / physLimit   | callback time gap / physLimit    | callback counters        ");
        Logger::writeToLog ("numLoops | avg     min     max     stddev   | avg     min     max     stddev   | called  late    >limit   ");
        Logger::writeToLog ("-----    | -----   -----   -----   -----    | -----   -----   -----   -----    | ---     ---     ---      ");
    }

    //==============================================================================
    void printAndResetPerformanceMetrics()
    {
        std::unique_lock<std::mutex> lock (metricMutex);

        auto runtimeMetric = audioCallbackRuntimeMs;
        auto gapMetric = audioCallbackGapMs;
        auto late = numLateCallbacks;
        auto overLimit = numCallbacksOverPhysicalTimeLimit;

        resetPerformanceMetrics();
        updateNumLoopIterationsPerCallback();

        lock.unlock();

        Logger::writeToLog (String (numLoopIterationsPerCallback).paddedRight (' ', 8) + " | "
                            + getPercentFormattedMetricString (runtimeMetric) + " | "
                            + getPercentFormattedMetricString (gapMetric) + " | "
                            + String (runtimeMetric.getCount()).paddedRight (' ', 8)
                            + String (late).paddedRight (' ', 8)
                            + String (overLimit).paddedRight (' ', 8) + " | ");
    }

    //==============================================================================
    String getPercentFormattedMetricString (const StatisticsAccumulator<double> metric) const
    {
        auto physTimeLimit = getPhysicalTimeLimitMs();

        return (String (100.0 * metric.getAverage()  / physTimeLimit, 1) + "%").paddedRight (' ', 8)
             + (String (100.0 * metric.getMinValue() / physTimeLimit, 1) + "%").paddedRight (' ', 8)
             + (String (100.0 * metric.getMaxValue() / physTimeLimit, 1) + "%").paddedRight (' ', 8)
             + String (metric.getStandardDeviation(), 3).paddedRight (' ', 8);
    }

    //==============================================================================
    void resetPerformanceMetrics()
    {
        audioCallbackRuntimeMs.reset();
        audioCallbackGapMs.reset();
        numLateCallbacks = 0;
        numCallbacksOverPhysicalTimeLimit = 0;
    }

    //==============================================================================
    void updateNumLoopIterationsPerCallback()
    {
        numLoopIterationsPerCallback = (int) loopIterationsSlider.getValue();
    }

    //==============================================================================
    static double getPreciseTimeMs() noexcept
    {
        return 1000.0 * (double) Time::getHighResolutionTicks() / (double) Time::getHighResolutionTicksPerSecond();
    }

    //==============================================================================
    double getPhysicalTimeLimitMs() const noexcept
    {
        return 1000.0 * (double) a.size() / currentSampleRate;
    }

    //==============================================================================
    std::vector<float> a, b, c; // must always be of size == current bufferSize
    double currentSampleRate = 0.0;

    StatisticsAccumulator<double> audioCallbackRuntimeMs;
    StatisticsAccumulator<double> audioCallbackGapMs;
    double lastCallbackStartTimeMs = 0.0;
    int numLateCallbacks = 0;
    int numCallbacksOverPhysicalTimeLimit = 0;
    int numLoopIterationsPerCallback;

    Slider loopIterationsSlider;
    std::mutex metricMutex;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }
