/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayAudioProcessorEditor::DelayAudioProcessorEditor (DelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Set initial window size of plugin UI
    setSize (400, 300);

    // Load embedded image from BinaryData (compiled resource inside JUCE project)
    myImage = juce::ImageCache::getFromMemory(
        BinaryData::Neville_jpeg,
        BinaryData::Neville_jpegSize
    );

    auto& params = processor.getParameters();

    //========================
    // Dry/Wet slider
    //========================
    juce::AudioParameterFloat* dryWetParameter =
        (juce::AudioParameterFloat*)params.getUnchecked(0);

    mDryWetSlider.setBounds(0, 0, 100, 100);
    mDryWetSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);

    // No text box because UI is minimal/clean design
    mDryWetSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);

    // Match slider range to plugin parameter range
    mDryWetSlider.setRange(dryWetParameter->range.start, dryWetParameter->range.end);

    // Initialize slider to current parameter value
    mDryWetSlider.setValue(*dryWetParameter);

    addAndMakeVisible(mDryWetSlider);

    // Connect UI interaction to parameter (manual automation binding)
    mDryWetSlider.onDragStart = [dryWetParameter]
    {
        dryWetParameter->beginChangeGesture();
    };

    mDryWetSlider.onValueChange = [this, dryWetParameter]
    {
        *dryWetParameter = mDryWetSlider.getValue();
    };

    mDryWetSlider.onDragEnd = [dryWetParameter]
    {
        dryWetParameter->endChangeGesture();
    };

    //========================
    // Feedback slider
    //========================
    juce::AudioParameterFloat* feedbackParameter =
        (juce::AudioParameterFloat*)params.getUnchecked(1);

    mFeedbackSlider.setBounds(100, 0, 100, 100);
    mFeedbackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mFeedbackSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mFeedbackSlider.setRange(feedbackParameter->range.start, feedbackParameter->range.end);
    mFeedbackSlider.setValue(*feedbackParameter);

    addAndMakeVisible(mFeedbackSlider);

    mFeedbackSlider.onDragStart = [feedbackParameter]
    {
        feedbackParameter->beginChangeGesture();
    };

    mFeedbackSlider.onValueChange = [this, feedbackParameter]
    {
        *feedbackParameter = mFeedbackSlider.getValue();
    };

    mFeedbackSlider.onDragEnd = [feedbackParameter]
    {
        feedbackParameter->endChangeGesture();
    };

    //========================
    // Delay Time slider
    //========================
    juce::AudioParameterFloat* delayTimeParameter =
        (juce::AudioParameterFloat*)params.getUnchecked(2);

    mDelayTimeSlider.setBounds(200, 0, 100, 100);
    mDelayTimeSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mDelayTimeSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 0, 0);
    mDelayTimeSlider.setRange(delayTimeParameter->range.start, delayTimeParameter->range.end);
    mDelayTimeSlider.setValue(*delayTimeParameter);

    addAndMakeVisible(mDelayTimeSlider);

    mDelayTimeSlider.onDragStart = [delayTimeParameter]
    {
        delayTimeParameter->beginChangeGesture();
    };

    mDelayTimeSlider.onValueChange = [this, delayTimeParameter]
    {
        *delayTimeParameter = mDelayTimeSlider.getValue();
    };

    mDelayTimeSlider.onDragEnd = [delayTimeParameter]
    {
        delayTimeParameter->endChangeGesture();
    };
}

DelayAudioProcessorEditor::~DelayAudioProcessorEditor()
{
}

//==============================================================================
void DelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fill background (plugin UI base color)
    g.fillAll (juce::Colour (255, 182, 193));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));

    // Labels under sliders
    g.drawFittedText ("Dry/Wet",    0,   100, 100, 20, juce::Justification::centred, 1);
    g.drawFittedText ("Feedback",   100, 100, 100, 20, juce::Justification::centred, 1);
    g.drawFittedText ("Delay Time", 200, 100, 100, 20, juce::Justification::centred, 1);

    g.setColour (juce::Colours::white);

    // Title font using modern JUCE FontOptions API
    g.setFont (juce::Font (juce::FontOptions (30.0f)
                            .withStyle ("bold")));

    g.drawText (
        "Amelia's Plug-In!",
        130, 180, 240, 60,
        juce::Justification::centredLeft,
        true
    );

    // Image transform setup (scaling + positioning)
    const int xDest = 20;
    const int yDest = 150;

    auto transform = juce::AffineTransform::scale (0.03f, 0.03f)
        .translated(xDest, yDest)
        .rotated (0.0f, xDest, yDest);

    g.drawImageTransformed(myImage, transform);
}

void DelayAudioProcessorEditor::resized()
{
    // Layout is handled manually via setBounds in constructor,
    // so this function is intentionally unused.
}
