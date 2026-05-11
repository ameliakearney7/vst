/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GainAudioProcessorEditor::GainAudioProcessorEditor (GainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    // (Our component is opaque, so we must completely fill the background with a solid colour)

}

GainAudioProcessorEditor::~GainAudioProcessorEditor()
{
}

//==============================================================================
void GainAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const int xDest = 200;
    const int yDest = 100;
    auto transform = juce::AffineTransform::scale (0.05f, 0.05f)
        .translated (xDest, yDest)
        .rotated (
                  juce::MathConstants<float>::pi / 2.0f,
                  xDest,
                  yDest);
    g.drawImageTransformed(myImage, transform);

    mGainControlSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mGainControlSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
  
    mGainControlSlider.onValueChange = [this]
    {
        processor.getParameters().getUnchecked(0)->setValueNotifyingHost(
            mGainControlSlider.getValue());
    };
    
    auto& params = processor.getParameters();

    auto* gainParameter =
        dynamic_cast<juce::AudioParameterFloat*>(
            params.getUnchecked(0));
    
    mGainControlSlider.onDragStart = [gainParameter] {
            gainParameter->beginChangeGesture();
        };
        
        mGainControlSlider.onValueChange = [this, gainParameter] {
        *gainParameter = mGainControlSlider.getValue();
        };
        
        mGainControlSlider.onDragEnd = [gainParameter] {
            gainParameter->endChangeGesture();
        };


    addAndMakeVisible(mGainControlSlider);
    
    mGainControlSlider.setRange(gainParameter->range.start, gainParameter->range.end);
    
    mGainControlSlider.setBounds(0, 0, 100, 100);
    
    //g.setColour (juce::Colours::white);
    //g.setFont (juce::FontOptions (15.0f));
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void GainAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
