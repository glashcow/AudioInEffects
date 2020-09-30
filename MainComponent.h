#pragma once

#include <JuceHeader.h>


class MainComponent : public juce::AudioAppComponent
{
public:
    //==============================================================================
    MainComponent()
    {
        levelSlider.setRange(0.0, 0.25);
        levelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
        levelLabel.setText("Noise Level", juce::dontSendNotification);

        addAndMakeVisible(levelSlider);
        addAndMakeVisible(levelLabel);

        addAndMakeVisible(loopButton);
        loopButton.setButtonText("Looooooop");
        loopButton.onClick = [this] {looping = !looping; };

        setSize(600, 100);
        setAudioChannels(2, 2);

        loopBuffer = new juce::AudioBuffer<float>(2, 48000);
        bufferPosition = 0;
        looping = false;
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
                            outBuffer[sample] = inBuffer[sample] * random.nextFloat() * level;
                            if (bufferPosition >= 48000) {
                                bufferPosition = 0;
                            }
                            loopOutBuffer[bufferPosition] = outBuffer[sample];
                            bufferPosition++;
                        }
                    }
                }
            }
            else {
                auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
                for (auto sample = 0; sample < bufferToFill.numSamples; ++sample) {
                    if (bufferPosition >= 48000) {
                        bufferPosition = 0;
                    }
                    outBuffer[sample] = loopOutBuffer[bufferPosition];
                    ++bufferPosition;
                }
            }
        }
    }

    void releaseResources() override {}

    void resized() override
    {
        levelLabel.setBounds(10, 10, 90, 20);
        levelSlider.setBounds(100, 10, getWidth() - 110, 20);
        loopButton.setBounds(10, 30, 90, 20);
    }

private:
    juce::Random random;
    juce::Slider levelSlider;
    juce::Label levelLabel;
    juce::TextButton loopButton;

    juce::AudioBuffer<float>* loopBuffer;
    float* loopOutBuffer;

    int bufferPosition;
    bool looping;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
