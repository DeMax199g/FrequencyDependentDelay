/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  =============================c=================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreqencyDependentDelayerAudioProcessorEditor::FreqencyDependentDelayerAudioProcessorEditor(FreqencyDependentDelayerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    low_pass_freq_slider_attachment(audioProcessor.apvts, "Low Pass Freq", low_pass_freq_slider),
    high_pass_freq_slider_attachment(audioProcessor.apvts, "High Pass Freq", high_pass_freq_slider),
    low_pass_slope_slider_attachment(audioProcessor.apvts, "Low Pass Slope", low_pass_slope_slider),
    high_pass_slope_slider_attachment(audioProcessor.apvts, "High Pass Slope", high_pass_slope_slider),
    delay_slider_attachment(audioProcessor.apvts, "Delay", delay_slider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    auto comps = get_comps();
    auto labels = get_comps_labels();
    auto units = get_comps_units();
    auto texts = get_comps_texts();
    for (int i = 0; i < comps.size(); i++) 
    {
        auto& slider = *(comps[i]);
        slider.setTextValueSuffix(units[i]);
        auto& label = *(labels[i]);
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
        label.setText(texts[i], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::Flags::centred);
        label.attachToComponent(&slider, true);
    }

    setSize (600, 400);
}

FreqencyDependentDelayerAudioProcessorEditor::~FreqencyDependentDelayerAudioProcessorEditor()
{
}

//==============================================================================
void FreqencyDependentDelayerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    //    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void FreqencyDependentDelayerAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto response_area = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto high_pass_area = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto high_pass_labels_area = high_pass_area.removeFromTop(high_pass_area.getHeight() * 0.2);
    high_pass_freq_slider.setBounds(high_pass_area.removeFromLeft(high_pass_area.getWidth() * 0.5));
    high_pass_slope_slider.setBounds(high_pass_area);
    high_pass_freq_label.setBounds(high_pass_labels_area.removeFromLeft(high_pass_labels_area.getWidth() * 0.5));
    high_pass_slope_label.setBounds(high_pass_labels_area);

    auto low_pass_area = bounds.removeFromRight(bounds.getWidth()* 0.5);
    auto low_pass_labels_area = low_pass_area.removeFromTop(low_pass_area.getHeight() * 0.2);
    low_pass_freq_slider.setBounds(low_pass_area.removeFromLeft(low_pass_area.getWidth()*0.5));
    low_pass_slope_slider.setBounds(low_pass_area);
    low_pass_freq_label.setBounds(low_pass_labels_area.removeFromLeft(low_pass_labels_area.getWidth() * 0.5));
    low_pass_slope_label.setBounds(low_pass_labels_area);

    auto delay_area = bounds;
    auto delay_label_area = delay_area.removeFromTop(delay_area.getHeight() * 0.2);
    delay_slider.setBounds(delay_area);
    delay_label.setBounds(delay_label_area);
}

std::vector<juce::Slider*> FreqencyDependentDelayerAudioProcessorEditor::get_comps()
{
    return
    {
        &low_pass_freq_slider,
        &high_pass_freq_slider,
        &low_pass_slope_slider,
        &high_pass_slope_slider,
        &delay_slider
    };
}

std::vector<juce::Label*> FreqencyDependentDelayerAudioProcessorEditor::get_comps_labels()
{
    return
    {
        &low_pass_freq_label,
        &high_pass_freq_label,
        &low_pass_slope_label,
        &high_pass_slope_label,
        &delay_label
    };
}

std::vector<std::string> FreqencyDependentDelayerAudioProcessorEditor::get_comps_units()
{
    return
    {
        " Hz",
        " Hz",
        "",
        "",
        " ms"
    };
}

std::vector<std::string> FreqencyDependentDelayerAudioProcessorEditor::get_comps_texts()
{
    return
    {
        "Low Pass Frequency",
        "High Pass Frequency",
        "Low Pass Slope",
        "High Pass Slope",
        "Delay"
    };
}