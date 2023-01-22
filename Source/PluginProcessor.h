/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <iostream>
#include <vector>

enum Slope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct Chain_Settings {
    float low_pass_freq{ 20000.f }, high_pass_freq{ 20.f }, delay_ms{ 0.f };
    Slope low_pass_slope{ Slope::Slope_12 }, high_pass_slope{ Slope::Slope_12 };
};
Chain_Settings get_chain_settings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;
using Pass_Filter = juce::dsp::ProcessorChain< Filter, Filter, Filter, Filter >;
using Pass_Chain = juce::dsp::ProcessorChain< Pass_Filter, Pass_Filter >;

enum Pass_Chain_Positions {
    High_Pass,
    Low_Pass
};


//==============================================================================
/**
*/
class FreqencyDependentDelayerAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    FreqencyDependentDelayerAudioProcessor();
    ~FreqencyDependentDelayerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
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
    //=== MY PARAMETERS



    static juce::AudioProcessorValueTreeState::ParameterLayout create_parameter_layout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", create_parameter_layout() };


private:
    Pass_Chain pass_chain_mono;
    using Coefficients = Filter::CoefficientsPtr;
    static void update_coefficients(Coefficients& old, const Coefficients& replacements);
    
    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain_part, const CoefficientType& pass_coefficients) {
        update_coefficients(chain_part.template get<Index>().coefficients, pass_coefficients[Index]);
        chain_part.template setBypassed<Index>(false);
    };

    template<typename ChainType, typename CoefficientType>
    void update_pass_filter(ChainType& chain_part,
            const CoefficientType& pass_coefficients,
            Slope& slope) {
        chain_part.template setBypassed<0>(true);
        chain_part.template setBypassed<1>(true);
        chain_part.template setBypassed<2>(true);
        chain_part.template setBypassed<3>(true);

        switch (slope) {
        case Slope_48:
            update<3>(chain_part, pass_coefficients);
        case Slope_36:
            update<2>(chain_part, pass_coefficients);
        case Slope_24:
            update<1>(chain_part, pass_coefficients);
        case Slope_12:
            update<0>(chain_part, pass_coefficients);
        };
    };

    //juce::dsp::AudioBlock<float> shift_block, shift_block_copy;
    juce::AudioBuffer<float> cut_buffer;
    std::vector < std::vector<float> > shift_buffers;
    void delay_signal(float* buffer, int buffer_size, std::vector<float>& shift_buffer);
    void update_shift_buffer(int num_samples_new, std::vector<float>& shift_buffer);
    
    void update_processing();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreqencyDependentDelayerAudioProcessor)
};