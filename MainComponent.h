#pragma once

#include <JuceHeader.h>


class MainComponent : public juce::AudioAppComponent
{
public:
    MainComponent()
    {
        sizeOfLoopBuff = 480000;

        levelSlider.setRange(0.0, 0.25);
        levelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
        levelLabel.setText("Noise Level", juce::dontSendNotification);
        addAndMakeVisible(levelSlider);
        addAndMakeVisible(levelLabel);

        addAndMakeVisible(loopSlider);
        loopSlider.setRange(100, sizeOfLoopBuff, 1);
        loopSlider.onValueChange = [this] {
            numberOfSamplesToLoop = (int)loopSlider.getValue();
            bufferPosition = sizeOfLoopBuff - numberOfSamplesToLoop;
            tableDelta = (float)samplesWhenLoopStarted / (float)numberOfSamplesToLoop;
        };

        loopSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
        addAndMakeVisible(loopLabel);
        loopLabel.setText("Loop length", juce::dontSendNotification);

        addAndMakeVisible(loopButton);
        loopButton.setButtonText("Looooooop");
        loopButton.onClick = [this] {
            looping = !looping; 
            samplesWhenLoopStarted = (int)loopSlider.getValue();
            currentIndex = sizeOfLoopBuff - samplesWhenLoopStarted;
            tableDelta = (float)samplesWhenLoopStarted / (float)numberOfSamplesToLoop;
        };

        setSize(600, 100);
        setAudioChannels(2, 2);

        loopBuffer = new juce::AudioBuffer<float>(2, sizeOfLoopBuff);
        bufferPosition = 0;
        samplesWhenLoopStarted = 1;
        tableDelta = 1;
        currentIndex = 1;
        looping = false;
        tape = true;
        loopOutBuffer = loopBuffer->getWritePointer(0, bufferPosition);
    }

    ~MainComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay(int, double) override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeInputChannels = device->getActiveInputChannels();
        auto activeOutputChannels = device->getActiveOutputChannels();
        auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
        auto maxOutputChannels = activeOutputChannels.getHighestBit() + 1;

        auto level = (float)levelSlider.getValue();

        for (auto channel = 0; channel < maxOutputChannels; ++channel)
        {
            if (!looping) {
                normalPlaying(activeOutputChannels, activeInputChannels, channel, maxInputChannels, bufferToFill, level); 
            }
            else {
                tape ? loopedTape(bufferToFill, channel): loopedNoTape(bufferToFill, channel);
            }
        }
    }

    void normalPlaying(juce::BigInteger activeOutputChannels, juce::BigInteger activeInputChannels, int channel, int maxInputChannels, const juce::AudioSourceChannelInfo& bufferToFill, float level) {
        if ((!activeOutputChannels[channel]) || maxInputChannels == 0)
        {
            bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
        }
        else
        {
            auto actualInputChannel = channel % maxInputChannels;

            if (!activeInputChannels[channel])
            {
                bufferToFill.buffer->clear(channel, bufferToFill.startSample, bufferToFill.numSamples);
            }
            else
            {
                auto* inBuffer = bufferToFill.buffer->getReadPointer(actualInputChannel,
                    bufferToFill.startSample);
                auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);

                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample) {
                    outBuffer[sample] = inBuffer[sample]  * level;
                    bufferPosition = bufferPosition % sizeOfLoopBuff;
                    loopOutBuffer[bufferPosition] = outBuffer[sample];
                    bufferPosition++;
                }
            }
        }
    }

    void loopedNoTape(const juce::AudioSourceChannelInfo& bufferToFill, int channel) {
        auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample) {
            if (bufferPosition >= sizeOfLoopBuff) {
                bufferPosition = sizeOfLoopBuff - numberOfSamplesToLoop;
            }
            outBuffer[sample] = loopOutBuffer[bufferPosition];
            ++bufferPosition;
        }
    }

    void loopedTape(const juce::AudioSourceChannelInfo& bufferToFill, int channel) {
        auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
        for(auto sample = 0; sample < bufferToFill.numSamples; ++sample) {
            auto index0 = (unsigned int)currentIndex;
            auto index1 = index0 + 1;

            auto frac = currentIndex - (float)index0;
            auto value0 = loopOutBuffer[index0];
            auto value1 = loopOutBuffer[index1];
            auto currentSample = value0 + frac * (value1 - value0);
            outBuffer[sample] = currentSample;
            currentIndex += tableDelta;
            if (currentIndex >= sizeOfLoopBuff) {
                currentIndex = sizeOfLoopBuff - numberOfSamplesToLoop;
            }
        }
    }

    void releaseResources() override {
        loopBuffer = nullptr;
    }

    void resized() override
    {
        levelLabel.setBounds(10, 10, 90, 20);
        levelSlider.setBounds(100, 10, getWidth() - 110, 20);

        loopLabel.setBounds(10, 30, 90, 20);
        loopSlider.setBounds(100, 30, getWidth() - 110, 20);
        loopButton.setBounds(10, 50, 90, 20);
    }

private:
    juce::Slider levelSlider;
    juce::Label levelLabel;
    juce::Slider loopSlider;
    juce::Label loopLabel;
    juce::TextButton loopButton;

    juce::AudioBuffer<float>* loopBuffer;
    float* loopOutBuffer;
    float tableDelta, currentIndex;

    int bufferPosition, numberOfSamplesToLoop, sizeOfLoopBuff, samplesWhenLoopStarted;
    bool looping, tape;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
