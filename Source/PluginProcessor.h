#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/ReverbEngine.h"
#include "Presets.h"

class RyvrbAudioProcessor : public juce::AudioProcessor
{
public:
    RyvrbAudioProcessor();
    ~RyvrbAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "RYVRB"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    // Presets de fabrica expuestos como "programas" del host
    int getNumPrograms() override { return kNumPresets; }
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override { if (index != currentPreset.load()) applyPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void applyPreset (int index);
    void selectSlot (int slot);        // comparacion A/B
    void copyToOtherSlot();

    juce::AudioProcessorValueTreeState apvts;
    ReverbEngine engine;
    std::atomic<int> currentPreset { 0 };
    int currentSlot = 0;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::ValueTree slotState[2];
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RyvrbAudioProcessor)
};
