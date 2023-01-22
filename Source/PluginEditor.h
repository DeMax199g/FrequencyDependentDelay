/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct Custom_Rotary_Slider : juce::Slider {
    Custom_Rotary_Slider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::TextBoxBelow)
    {

    }
};

//==============================================================================
/**
*/
class FreqencyDependentDelayerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FreqencyDependentDelayerAudioProcessorEditor (FreqencyDependentDelayerAudioProcessor&);
    ~FreqencyDependentDelayerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FreqencyDependentDelayerAudioProcessor& audioProcessor;

    Custom_Rotary_Slider low_pass_freq_slider,
        low_pass_slope_slider,
        high_pass_freq_slider,
        high_pass_slope_slider,
        delay_slider;

    juce::Label low_pass_freq_label,
        high_pass_freq_label,
        low_pass_slope_label,
        high_pass_slope_label,
        delay_label;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment low_pass_freq_slider_attachment,
        low_pass_slope_slider_attachment,
        high_pass_freq_slider_attachment,
        high_pass_slope_slider_attachment,
        delay_slider_attachment;

    std::vector<juce::Slider*> get_comps();
    std::vector<juce::Label*> get_comps_labels();
    std::vector<std::string> get_comps_units();
    std::vector<std::string> get_comps_texts();

    Pass_Chain pass_chain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqencyDependentDelayerAudioProcessorEditor)
};
