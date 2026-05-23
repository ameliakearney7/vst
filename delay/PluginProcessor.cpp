/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
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
    mCircularBufferLeft = nullptr;
    mCircularBufferRight = nullptr;
    
    mCircularBufferLength = 0;
    mCircularBufferWriteHead = 0;
    
    mDelayTimeInSamples = 0;
    mCircularBufferReadHead = 0;
    
    mFeedbackLeft = 0;
    mFeedbackRight = 0;
    
    mDryWet = 0.5;
    
    addParameter(mDryWetParameter = new juce::AudioParameterFloat({"drywet", 1}, "Dry Wet", 0, 1.0, 0.5));
    addParameter(mFeedbackParameter = new juce::AudioParameterFloat({"feedback", 1}, "Feedback", 0, 0.98, 0.5));
    addParameter(mDelayTimeParameter = new juce::AudioParameterFloat({"delaytime", 1}, "Delay Time", 0.01, MAX_DELAY_TIME, 0.5));
    
    mDelayTimeSmoothed = 0;
}

DelayAudioProcessor::~DelayAudioProcessor()
{
    if (mCircularBufferLeft != nullptr) {
        delete [] mCircularBufferLeft;
        mCircularBufferLeft = nullptr;
    }
    
    if (mCircularBufferRight != nullptr) {
        delete [] mCircularBufferRight;
        mCircularBufferRight = nullptr;
    }
}

//==============================================================================
const juce::String DelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

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

double DelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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
void DelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
    mCircularBufferWriteHead = 0;
    mCircularBufferLength = sampleRate * MAX_DELAY_TIME;
    mDelayTimeInSamples = sampleRate * 0.5;
    
    if (mCircularBufferLeft == nullptr) {
        mCircularBufferLeft = new float [mCircularBufferLength]();
    }
    
    if (mCircularBufferRight == nullptr) {
        mCircularBufferRight = new float [mCircularBufferLength]();
    }
    
    mDelayTimeSmoothed = *mDelayTimeParameter;
    
}


void DelayAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);
    
    mDelayTimeInSamples = getSampleRate() * 0.5;
    mDelayTimeSmoothed = mDelayTimeSmoothed - 0.001 * (mDelayTimeSmoothed - *mDelayTimeParameter);
    mDelayTimeInSamples = getSampleRate() * *mDelayTimeParameter;

    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        // read head calculation
        mCircularBufferReadHead = mCircularBufferWriteHead - mDelayTimeInSamples;

        if (mCircularBufferReadHead < 0)
            mCircularBufferReadHead += mCircularBufferLength;
        
        int readHead_x = (int)mCircularBufferReadHead;
        int readHead_x1 = readHead_x + 1;
        float readHeadFloat = mCircularBufferReadHead - readHead_x;

        // add this in case readHead_x + 1 needs to wrap around the circular buffer to the start
        if (readHead_x1 >= mCircularBufferLength) {
        readHead_x1 -= mCircularBufferLength;
        }
        
        float delay_sample_left = lin_interp(mCircularBufferLeft[readHead_x], mCircularBufferLeft[readHead_x1], readHeadFloat);
        float delay_sample_right = lin_interp(mCircularBufferRight[readHead_x], mCircularBufferRight[readHead_x1], readHeadFloat);
        
        mFeedbackLeft = delay_sample_left * *mFeedbackParameter;
        mFeedbackRight = delay_sample_right * *mFeedbackParameter;

        buffer.setSample(0, i, buffer.getSample(0, i) * (1 - mDryWet) + delay_sample_left * mDryWet);
        buffer.setSample(1, i, buffer.getSample(1, i) * (1 - mDryWet) + delay_sample_right * mDryWet);
        // write into delay buffer WITH feedback control
        
        mCircularBufferLeft[mCircularBufferWriteHead] = leftChannel[i] + mFeedbackLeft;
        mCircularBufferRight[mCircularBufferWriteHead] = rightChannel[i] + mFeedbackRight;

        // advance write head
        mCircularBufferWriteHead++;

        if (mCircularBufferWriteHead >= mCircularBufferLength)
            mCircularBufferWriteHead = 0;
    }

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
}

//==============================================================================
bool DelayAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return new DelayAudioProcessorEditor (*this);
}

//==============================================================================
void DelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}

float DelayAudioProcessor::lin_interp(float sample_x, float sample_x1, float inPhase)
{
    return (1 - inPhase) * sample_x + inPhase * sample_x1; // maybe looks familiar from dry/wet math...
}
