/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Constructor: sets up plugin state, parameters, and initializes pointers
DelayAudioProcessor::DelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
        #if ! JucePlugin_IsMidiEffect
            #if ! JucePlugin_IsSynth
                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            #endif
                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        #endif
        )
#endif
{
    // Initialize circular buffer pointers (delay line storage)
    mCircularBufferLeft = nullptr;
    mCircularBufferRight = nullptr;

    // Initialize buffer state variables
    mCircularBufferLength = 0;
    mCircularBufferWriteHead = 0;

    mDelayTimeInSamples = 0;
    mCircularBufferReadHead = 0;

    // Initialize feedback values (used in delay regeneration)
    mFeedbackLeft = 0;
    mFeedbackRight = 0;

    // Audio parameters (automatable controls in DAW)

    // Dry/Wet mix: controls balance between original and delayed signal
    addParameter(mDryWetParameter = new juce::AudioParameterFloat(
        {"drywet", 1}, "Dry Wet", 0, 1.0, 0.5));

    // Feedback: controls how much delayed signal is fed back into buffer
    addParameter(mFeedbackParameter = new juce::AudioParameterFloat(
        {"feedback", 1}, "Feedback", 0, 0.90, 0.5));

    // Delay time in seconds (converted to samples later)
    addParameter(mDelayTimeParameter = new juce::AudioParameterFloat(
        {"delaytime", 1}, "Delay Time", 0.01, MAX_DELAY_TIME, 0.5));

    // Smoothed delay time value (prevents zipper noise when changing delay)
    mDelayTimeSmoothed = 0;
}

// Destructor: frees dynamically allocated circular buffers
DelayAudioProcessor::~DelayAudioProcessor()
{
    if (mCircularBufferLeft != nullptr)
    {
        delete [] mCircularBufferLeft;
        mCircularBufferLeft = nullptr;
    }

    if (mCircularBufferRight != nullptr)
    {
        delete [] mCircularBufferRight;
        mCircularBufferRight = nullptr;
    }
}

//==============================================================================
// Returns plugin name shown in DAWs
const juce::String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

// MIDI capability flags (compile-time configuration)
bool DelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

// Tail length for reverb-like plugins (0 since this is a delay)
double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

// Program management (not used, but required by JUCE)
int DelayAudioProcessor::getNumPrograms()
{
    return 1;
}

int DelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
// Called before audio starts: allocate buffers and initialize delay system
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mCircularBufferWriteHead = 0;

    // Buffer length = max delay time in samples
    mCircularBufferLength = sampleRate * MAX_DELAY_TIME;

    // Default delay time = 0.5 seconds in samples
    mDelayTimeInSamples = sampleRate * *mDelayTimeParameter;

    // Allocate circular buffer memory (only once)
    if (mCircularBufferLeft == nullptr)
    {
        mCircularBufferLeft = new float [mCircularBufferLength]();
    }

    if (mCircularBufferRight == nullptr)
    {
        mCircularBufferRight = new float [mCircularBufferLength]();
    }

    // Initialize smoothed delay parameter to current value
    mDelayTimeSmoothed = *mDelayTimeParameter;
}

// Called when audio stops: free or reset resources if needed
void DelayAudioProcessor::releaseResources()
{
}

//==============================================================================
// Ensures plugin supports mono/stereo layouts correctly
#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else

    // Only allow mono or stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Ensure input/output channel layout matches
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
// Main audio processing block (runs per buffer)
void DelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels (safety for mono/stereo mismatches)
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        // Smooth delay time changes to avoid artifacts (zipper noise reduction)
        mDelayTimeSmoothed = mDelayTimeSmoothed
            - 0.001 * (mDelayTimeSmoothed - *mDelayTimeParameter);
        
        // Calculate read head position (where delayed audio is read from)
        mCircularBufferReadHead = mCircularBufferWriteHead - mDelayTimeInSamples;

        // Convert delay time parameter into samples
        mDelayTimeInSamples = getSampleRate() * mDelayTimeSmoothed;

        // Wrap read head if negative (circular buffer behavior)
        if (mCircularBufferReadHead < 0)
            mCircularBufferReadHead += mCircularBufferLength;

        int readHead_x = (int)mCircularBufferReadHead;
        int readHead_x1 = readHead_x + 1;

        float readHeadFloat = mCircularBufferReadHead - readHead_x;

        // Wrap interpolation index if it exceeds buffer size
        if (readHead_x1 >= mCircularBufferLength)
            readHead_x1 -= mCircularBufferLength;

        // Linear interpolation between buffer samples for smooth delay output
        float delay_sample_left = lin_interp(
            mCircularBufferLeft[readHead_x],
            mCircularBufferLeft[readHead_x1],
            readHeadFloat);

        float delay_sample_right = lin_interp(
            mCircularBufferRight[readHead_x],
            mCircularBufferRight[readHead_x1],
            readHeadFloat);

        // Apply feedback (re-inject delayed signal into buffer)
        mFeedbackLeft = delay_sample_left * *mFeedbackParameter;
        mFeedbackRight = delay_sample_right * *mFeedbackParameter;

        // Mix dry + wet signal (Dry/Wet control)
        buffer.setSample(0, i,
            buffer.getSample(0, i) * (1 - *mDryWetParameter)
            + delay_sample_left * *mDryWetParameter);

        buffer.setSample(1, i,
            buffer.getSample(1, i) * (1 - *mDryWetParameter)
            + delay_sample_right * *mDryWetParameter);

        // Write current sample + feedback into circular buffer
        mCircularBufferLeft[mCircularBufferWriteHead] = leftChannel[i] + mFeedbackLeft;
        mCircularBufferRight[mCircularBufferWriteHead] = rightChannel[i] + mFeedbackRight;

        // Advance write head (wrap around circular buffer)
        mCircularBufferWriteHead++;

        if (mCircularBufferWriteHead >= mCircularBufferLength)
            mCircularBufferWriteHead = 0;
    }
}

//==============================================================================
// Editor creation (GUI)
bool DelayAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor (*this);
}

//==============================================================================
// Save plugin state (not implemented here)
void DelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

// Restore plugin state (not implemented here)
void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

//==============================================================================
// Plugin entry point (JUCE required)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}

// Linear interpolation function used for smoothing delay reads
float DelayAudioProcessor::lin_interp(float sample_x, float sample_x1, float inPhase)
{
    return (1 - inPhase) * sample_x + inPhase * sample_x1;
}
