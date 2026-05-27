/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// Maximum delay time in seconds (used to size circular buffer)
#define MAX_DELAY_TIME 2

//==============================================================================
class DelayAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    DelayAudioProcessor();
    ~DelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    // Total length of circular buffer in samples (depends on sample rate)
    int mCircularBufferLength;

    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Linear interpolation helper used for smooth delay reading
    float lin_interp(float sample_x, float sample_x1, float inPhase);

private:
    // Circular buffer (delay line storage)
    float* mCircularBufferLeft;
    float* mCircularBufferRight;

    // Write head index into circular buffer
    int mCircularBufferWriteHead;

    // Delay time in samples (computed from parameter + sample rate)
    float mDelayTimeInSamples;

    // Read head position in circular buffer (fractional for interpolation)
    float mCircularBufferReadHead;

    // Feedback signals (left/right channels)
    float mFeedbackLeft;
    float mFeedbackRight;

    // Smoothed delay time to prevent parameter jitter artifacts
    float mDelayTimeSmoothed;

    // Plugin parameters (automatable in DAW)
    juce::AudioParameterFloat* mDryWetParameter;
    juce::AudioParameterFloat* mFeedbackParameter;
    juce::AudioParameterFloat* mDelayTimeParameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DelayAudioProcessor)
};
