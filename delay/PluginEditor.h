/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class DelayAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DelayAudioProcessorEditor (DelayAudioProcessor&);
    ~DelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Reference to the processor so the editor can read/write parameters
    // This is required for UI ↔ audio engine communication
    DelayAudioProcessor& audioProcessor;

    // UI sliders controlling plugin parameters
    juce::Slider mDryWetSlider;
    juce::Slider mFeedbackSlider;
    juce::Slider mDelayTimeSlider;

    // Image displayed in the plugin UI (loaded from BinaryData in .cpp)
    juce::Image myImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessorEditor)
};
