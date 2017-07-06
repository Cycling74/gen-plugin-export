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

#include "../JuceDemoHeader.h"


//==============================================================================
class BouncingBallComp  : public Component
{
public:
    BouncingBallComp()
    {
        x = Random::getSystemRandom().nextFloat() * 200.0f;
        y = Random::getSystemRandom().nextFloat() * 200.0f;
        parentWidth = 50;
        parentHeight = 50;
        innerX = 0;
        innerY = 0;
        threadId = 0;

        const float speed = 5.0f; // give each ball a fixed speed so we can
                                  // see the effects of thread priority on how fast
                                  // they actually go.
        const float angle = Random::getSystemRandom().nextFloat() * float_Pi * 2.0f;

        dx = std::sin (angle) * speed;
        dy = std::cos (angle) * speed;

        size = Random::getSystemRandom().nextFloat() * 30.0f + 30.0f;

        colour = Colour ((juce::uint32) Random::getSystemRandom().nextInt())
                    .withAlpha (0.5f)
                    .withBrightness (0.7f);
    }

    void paint (Graphics& g) override
    {
        g.setColour (colour);
        g.fillEllipse (innerX, innerY, size, size);

        g.setColour (Colours::black);
        g.setFont (10.0f);
        g.drawText (String::toHexString ((int64) threadId), getLocalBounds(), Justification::centred, false);
    }

    void parentSizeChanged() override
    {
        parentWidth = getParentWidth() - size;
        parentHeight = getParentHeight() - size;
    }

    void moveBall()
    {
        threadId = Thread::getCurrentThreadId(); // this is so the component can print the thread ID inside the ball

        x += dx;
        y += dy;

        if (x < 0)
            dx = std::abs (dx);

        if (x > parentWidth)
            dx = -std::abs (dx);

        if (y < 0)
            dy = std::abs (dy);

        if (y > parentHeight)
            dy = -std::abs (dy);

        setBounds (((int) x) - 2,
                   ((int) y) - 2,
                   ((int) size) + 4,
                   ((int) size) + 4);

        innerX = x - getX();
        innerY = y - getY();

        repaint();
    }

private:
    float x, y, size, dx, dy, parentWidth, parentHeight;
    float innerX, innerY;
    Colour colour;
    Thread::ThreadID threadId;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BouncingBallComp)
};


//==============================================================================
class DemoThread    : public BouncingBallComp,
                      public Thread
{
public:
    DemoThread()
        : Thread ("Juce Demo Thread")
    {
        interval = Random::getSystemRandom().nextInt (50) + 6;

        // give the threads a random priority, so some will move more
        // smoothly than others..
        startThread (Random::getSystemRandom().nextInt (3) + 3);
    }

    ~DemoThread()
    {
        // allow the thread 2 seconds to stop cleanly - should be plenty of time.
        stopThread (2000);
    }

    void run() override
    {
        // this is the code that runs this thread - we'll loop continuously,
        // updating the coordinates of our blob.

        // threadShouldExit() returns true when the stopThread() method has been
        // called, so we should check it often, and exit as soon as it gets flagged.
        while (! threadShouldExit())
        {
            // sleep a bit so the threads don't all grind the CPU to a halt..
            wait (interval);

            // because this is a background thread, we mustn't do any UI work without
            // first grabbing a MessageManagerLock..
            const MessageManagerLock mml (Thread::getCurrentThread());

            if (! mml.lockWasGained())  // if something is trying to kill this job, the lock
                return;                 // will fail, in which case we'd better return..

            // now we've got the UI thread locked, we can mess about with the components
            moveBall();
        }
    }

private:
    int interval;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoThread)
};


//==============================================================================
class DemoThreadPoolJob  : public BouncingBallComp,
                           public ThreadPoolJob
{
public:
    DemoThreadPoolJob()
        : ThreadPoolJob ("Demo Threadpool Job")
    {
    }

    JobStatus runJob() override
    {
        // this is the code that runs this job. It'll be repeatedly called until we return
        // jobHasFinished instead of jobNeedsRunningAgain.
        Thread::sleep (30);

        // because this is a background thread, we mustn't do any UI work without
        // first grabbing a MessageManagerLock..
        const MessageManagerLock mml (this);

        // before moving the ball, we need to check whether the lock was actually gained, because
        // if something is trying to stop this job, it will have failed..
        if (mml.lockWasGained())
            moveBall();

        return jobNeedsRunningAgain;
    }

    void removedFromQueue()
    {
        // This is called to tell us that our job has been removed from the pool.
        // In this case there's no need to do anything here.
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoThreadPoolJob)
};

//==============================================================================
class MultithreadingDemo   : public Component,
                             private Timer,
                             private Button::Listener
{
public:
    MultithreadingDemo()
        : pool (3),
          controlButton ("Thread type"),
          isUsingPool (false)
    {
        setOpaque (true);

        addAndMakeVisible (controlButton);
        controlButton.changeWidthToFitText (24);
        controlButton.setTopLeftPosition (20, 20);
        controlButton.setTriggeredOnMouseDown (true);
        controlButton.setAlwaysOnTop (true);
        controlButton.addListener (this);
    }

    ~MultithreadingDemo()
    {
        pool.removeAllJobs (true, 2000);
    }

    void resetAllBalls()
    {
        stopTimer();

        pool.removeAllJobs (true, 4000);
        balls.clear();

        if (isShowing())
        {
            while (balls.size() < 5)
                addABall();

            startTimer (300);
        }
    }

    void paint (Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::windowBackground));
    }

private:
    ThreadPool pool;
    TextButton controlButton;
    bool isUsingPool;

    OwnedArray<Component> balls;

    void setUsingPool (bool usePool)
    {
        isUsingPool = usePool;
        resetAllBalls();
    }

    void addABall()
    {
        if (isUsingPool)
        {
            DemoThreadPoolJob* newBall = new DemoThreadPoolJob();
            balls.add (newBall);
            addAndMakeVisible (newBall);
            newBall->parentSizeChanged();

            pool.addJob (newBall, false);
        }
        else
        {
            DemoThread* newBall = new DemoThread();
            balls.add (newBall);
            addAndMakeVisible (newBall);
            newBall->parentSizeChanged();
        }
    }

    void removeABall()
    {
        if (balls.size() > 0)
        {
            int indexToRemove = Random::getSystemRandom().nextInt (balls.size());

            if (isUsingPool)
                pool.removeJob (dynamic_cast<DemoThreadPoolJob*> (balls [indexToRemove]), true, 4000);

            balls.remove (indexToRemove);
        }
    }


    // this gets called when a component is added or removed from a parent component.
    void parentHierarchyChanged() override
    {
        // we'll use this as an opportunity to start and stop the threads, so that
        // we don't leave them going when the component's not actually visible.
        resetAllBalls();
    }

    void timerCallback() override
    {
        if (Random::getSystemRandom().nextBool())
        {
            if (balls.size() <= 10)
                addABall();
        }
        else
        {
            if (balls.size() > 3)
                removeABall();
        }
    }

    void buttonClicked (Button*) override
    {
        PopupMenu m;
        m.addItem (1, "Use one thread per ball", true, ! isUsingPool);
        m.addItem (2, "Use a thread pool", true, isUsingPool);

        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&controlButton),
                         ModalCallbackFunction::forComponent (menuItemChosenCallback, this));
    }

    static void menuItemChosenCallback (int result, MultithreadingDemo* demoComponent)
    {
        if (result != 0 && demoComponent != nullptr)
            demoComponent->setUsingPool (result == 2);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultithreadingDemo)
};


// This static object will register this demo type in a global list of demos..
static JuceDemoType<MultithreadingDemo> demo ("40 Multi-threading");
