/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreqencyDependentDelayerAudioProcessor::FreqencyDependentDelayerAudioProcessor()
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
}

FreqencyDependentDelayerAudioProcessor::~FreqencyDependentDelayerAudioProcessor()
{
}

//==============================================================================
const juce::String FreqencyDependentDelayerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FreqencyDependentDelayerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FreqencyDependentDelayerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FreqencyDependentDelayerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FreqencyDependentDelayerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FreqencyDependentDelayerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FreqencyDependentDelayerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FreqencyDependentDelayerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FreqencyDependentDelayerAudioProcessor::getProgramName (int index)
{
    return {};
}

void FreqencyDependentDelayerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FreqencyDependentDelayerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    int n_channels = 2;

    cut_buffer.setSize(n_channels, samplesPerBlock);
    for (int ch = 0; ch < n_channels; ch++)
    {
        shift_buffers.push_back({});
    }

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    pass_chain_mono.prepare(spec);

    auto chain_settings = get_chain_settings(apvts);

    update_processing();
}

void FreqencyDependentDelayerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FreqencyDependentDelayerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void FreqencyDependentDelayerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    auto chain_settings = get_chain_settings(apvts);
    DBG("High Pass Freq: ");
    DBG(chain_settings.high_pass_freq);
    DBG("Low Pass Freq: ");
    DBG(chain_settings.low_pass_freq);
    DBG("High Pass Slope: ");
    DBG(chain_settings.high_pass_slope);
    DBG("Low Pass Slope: ");
    DBG(chain_settings.low_pass_slope);
    DBG("Delay ms: ");
    DBG(chain_settings.delay_ms);

    update_processing();
    juce::dsp::AudioBlock<float> block(buffer);

    int num_requested_samples = buffer.getNumSamples();
    for (int ch = 0; ch < buffer.getNumChannels(); ch++) // for soome reason people use ++ch?
    {
        cut_buffer.copyFrom(ch, 0, buffer.getReadPointer(ch), num_requested_samples);
        juce::dsp::ProcessContextReplacing<float> mono_context(block.getSingleChannelBlock(ch));
        pass_chain_mono.process(mono_context);
        auto* data_channel = buffer.getWritePointer(ch);
        auto* cut_channel = cut_buffer.getWritePointer(ch);
        for (int i = 0; i < num_requested_samples; i++)
        {
            cut_channel[i] -= data_channel[i]; // calculate rest of cut signal (opposite filtering)
        }

        auto& shift_buffer = shift_buffers[ch];
        if (chain_settings.delay_ms < 0)
        {
            delay_signal(data_channel, num_requested_samples, shift_buffer);
        }
        else if (chain_settings.delay_ms > 0)
        {
            delay_signal(cut_channel, num_requested_samples, shift_buffer);
        }
        else { ; }

        for (int i = 0; i < num_requested_samples; i++)
        {
            data_channel[i] += cut_channel[i];
        }
    }

    /*
    //
    // LR not arbitrary many channels
    // 
    
    //buffer.addFrom(0, 0, buffer, 1, 0, buffer.getNumSamples()); // mix in channel 0
    //buffer.copyFrom(1, 0, buffer.getReadPointer(0), buffer.getNumSamples()); // copy mix to channel 1
    cut_buffer.copyFrom(0, 0, buffer.getReadPointer(0), buffer.getNumSamples()); // 
    cut_buffer.copyFrom(1, 0, buffer.getReadPointer(1), buffer.getNumSamples()); // 


    juce::dsp::ProcessContextReplacing<float> mono_context_left(block.getSingleChannelBlock(0));
    juce::dsp::ProcessContextReplacing<float> mono_context_right(block.getSingleChannelBlock(1));
    pass_chain_mono.process(mono_context_left);
    pass_chain_mono.process(mono_context_right);

    //juce::AudioBuffer<float> output_buffer_copy(output_block.getNumChannels(), output_block.getNumSamples());     // Allocate storage
    //juce::dsp::AudioBlock<float> output_block_copy(output_buffer_copy);
    //output_block_copy.copy(output_buffer_copy);

    // subtract the filtered signal from the original to obtain the opposite filtering (to add them later)

    auto* data_channel_left = buffer.getWritePointer(0);
    auto* data_channel_right = buffer.getWritePointer(1);
    auto* cut_channel_left = cut_buffer.getWritePointer(0);
    auto* cut_channel_right = cut_buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        //data_channel[i] = -0.5 + float(i) / float(buffer.getNumSamples());
        //cut_channel[i] = data_channel[i] / 5.f;
        cut_channel_left[i] -= data_channel_left[i]; // calculate rest of cut signal (opposite filtering)
        cut_channel_right[i] -= data_channel_right[i]; // calculate rest of cut signal (opposite filtering)
    }
    DBG("delaying");
    if (chain_settings.delay_ms < 0)
    {
        delay_signal(data_channel_left, data_channel_right, buffer.getNumSamples(), shift_block_left, shift_block_right); // delay buffer (overwrite)
    }
    else if (chain_settings.delay_ms > 0)
    {
        delay_signal(cut_channel_left, cut_channel_right, buffer.getNumSamples(), shift_block_left, shift_block_right); // delay buffer (overwrite)
    }
    else
    {
        ;
    }
    DBG("delayed");
    for (int i = 0; i < buffer.getNumSamples(); i++)
    {
        data_channel_right[i] += cut_channel_right[i]; // add delayed_buffer to sig
        data_channel_left[i] += cut_channel_left[i]; // add delayed_buffer to sig
    }
    //buffer.copyFrom(1, 0, buffer.getReadPointer(0), buffer.getNumSamples());
    */
}

//==============================================================================
bool FreqencyDependentDelayerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FreqencyDependentDelayerAudioProcessor::createEditor()
{
    return new FreqencyDependentDelayerAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FreqencyDependentDelayerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void FreqencyDependentDelayerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
        update_processing();
    }
}

Chain_Settings get_chain_settings(juce::AudioProcessorValueTreeState& apvts)
{
    Chain_Settings settings;
    settings.low_pass_freq = apvts.getRawParameterValue("Low Pass Freq")->load();
    settings.high_pass_freq = apvts.getRawParameterValue("High Pass Freq")->load();
    settings.low_pass_slope = static_cast<Slope>(apvts.getRawParameterValue("Low Pass Slope")->load());
    settings.high_pass_slope = static_cast<Slope>(apvts.getRawParameterValue("High Pass Slope")->load());
    settings.delay_ms = apvts.getRawParameterValue("Delay")->load();
    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout FreqencyDependentDelayerAudioProcessor::create_parameter_layout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            "Low Pass Freq",
            "Low Pass Freq",
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            20000.f
            )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            "High Pass Freq",
            "High Pass Freq",
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            20.f
            )
    );
    layout.add(
        std::make_unique<juce::AudioParameterFloat>(
            "Delay",
            "Delay",
            juce::NormalisableRange<float>(-200.f, 200.f, 2.f, 1.f), 
            0.f
        )
    );
    juce::StringArray string_array;
    for (int order = 2; order <= 8; order += 2) {
        juce::String str;
        str << order * 6;
        str << " dB/Oct";
        string_array.add(str); 
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>("Low Pass Slope", "Low Pass Slope", string_array, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("High Pass Slope", "High Pass Slope", string_array, 0));
    return layout;
}

void FreqencyDependentDelayerAudioProcessor::update_shift_buffer(int num_samples_new, std::vector<float>& shift_buffer)
{
    int num_samples = shift_buffer.size();
    
    int diff = num_samples_new - num_samples;
    if (diff < 0)
    {
        shift_buffer.erase(shift_buffer.begin(), shift_buffer.begin() + std::abs(diff));
    }
    else if(diff > 0)
    { 
        std::vector<float> new_vec(diff, 0); // fill with 0 means 2 clicks
        // Implement fade in/out
        shift_buffer.insert(shift_buffer.begin(), new_vec.begin(), new_vec.end());
     }
}

void FreqencyDependentDelayerAudioProcessor::update_processing()
{
    auto chain_settings = get_chain_settings(apvts);

    auto low_pass_coefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chain_settings.low_pass_freq, getSampleRate(), 2 * (chain_settings.low_pass_slope + 1));
    update_pass_filter(pass_chain_mono.get<Pass_Chain_Positions::Low_Pass>(), low_pass_coefficients, chain_settings.low_pass_slope);

    auto high_pass_coefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chain_settings.high_pass_freq, getSampleRate(), 2 * (chain_settings.high_pass_slope + 1));
    update_pass_filter(pass_chain_mono.get<Pass_Chain_Positions::High_Pass>(), high_pass_coefficients, chain_settings.high_pass_slope);

    int num_samples_new = std::round(getSampleRate() * std::abs(chain_settings.delay_ms) / 1000);
    for (int ch = 0; ch < shift_buffers.size(); ch++)
    { 
        update_shift_buffer(num_samples_new, shift_buffers[ch]);
    }
}

void FreqencyDependentDelayerAudioProcessor::delay_signal(float* buffer, int buffer_size, std::vector<float>& shift_buffer)
{
    int shift_buffer_size = shift_buffer.size();
    if (shift_buffer_size > 0)
    {
        //std::vector<float> data_vec(buffer_left, buffer_left+buffer_size); // params pointers to (begin, end) of array
        shift_buffer.insert(shift_buffer.end(), buffer, buffer + buffer_size);
        //shift_block.insert(shift_block.end(), data_vec.begin(), data_vec.end());
        for (int i = 0; i < buffer_size; i++)
        {
            buffer[i] = shift_buffer[i];
        }
        shift_buffer.erase(shift_buffer.begin(), shift_buffer.begin() + buffer_size);
    }
    DBG("shift_buffer before: " << shift_buffer_size << "shift_buffer after" << shift_buffer.size());
}

void FreqencyDependentDelayerAudioProcessor::update_coefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreqencyDependentDelayerAudioProcessor();
}
