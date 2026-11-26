#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout RyvrbAudioProcessor::createLayout()
{
    using namespace juce;
    using Fmt = std::function<String (float)>;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto fl = [] (const char* id, const char* name, NormalisableRange<float> r, float def, const char* unit, Fmt fmt)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
            AudioParameterFloatAttributes()
                .withLabel (unit)
                .withStringFromValueFunction ([fmt] (float v, int) { return fmt (v); })
                .withValueFromStringFunction ([] (const String& t)
                {
                    float f = t.getFloatValue();
                    if (t.trim().endsWithIgnoreCase ("k")) f *= 1000.f;
                    return f;
                }));
    };
    const Fmt f0 = [] (float v) { return String ((int) std::lround (v)); };
    const Fmt f1 = [] (float v) { return String (v, 1); };
    const Fmt f2 = [] (float v) { return String (v, 2); };
    const Fmt fk = [] (float v) { return v >= 10000.f ? String (v / 1000.f, 1) + "k" : String ((int) std::lround (v)); };

    p.push_back (fl ("predelay",  "Pre Delay",    { 0.f, 250.f, 0.1f, 0.5f },    20.f,   "ms", f1));
    p.push_back (fl ("decay",     "Decay",        { 0.1f, 20.f, 0.01f, 0.4f },   2.5f,   "s",  f2));
    p.push_back (fl ("lowratio",  "Low Ratio",    { 0.1f, 3.f, 0.01f },          1.15f,  "x",  f2));
    p.push_back (fl ("lowfreq",   "Low Freq",     { 50.f, 2000.f, 1.f, 0.5f },   400.f,  "Hz", f0));
    p.push_back (fl ("highratio", "High Ratio",   { 0.1f, 2.f, 0.01f },          0.85f,  "x",  f2));
    p.push_back (fl ("highfreq",  "High Freq",    { 1000.f, 16000.f, 1.f, 0.5f }, 4000.f, "Hz", fk));
    p.push_back (fl ("size",      "Size",         { 25.f, 200.f, 1.f },          100.f,  "%",  f0));
    p.push_back (fl ("diffusion", "Diffusion",    { 0.f, 100.f, 1.f },           70.f,   "%",  f0));
    p.push_back (fl ("modulation","Modulation",   { 0.f, 100.f, 1.f },           25.f,   "%",  f0));
    p.push_back (fl ("width",     "Width",        { 0.f, 200.f, 1.f },           100.f,  "%",  f0));
    p.push_back (fl ("duckamt",   "Ducker Amount",    { 0.f, 100.f, 1.f },       0.f,    "%",  f0));
    p.push_back (fl ("duckthr",   "Ducker Threshold", { -60.f, 0.f, 0.1f },      -24.f,  "dB", f1));
    p.push_back (fl ("duckrel",   "Ducker Release",   { 20.f, 1000.f, 1.f, 0.5f }, 250.f, "ms", f0));
    p.push_back (fl ("duckatk",   "Ducker Attack",    { 1.f, 100.f, 0.1f, 0.5f }, 8.f,    "ms", f1));
    p.push_back (fl ("lowcut",    "Low Cut",      { 20.f, 1000.f, 1.f, 0.4f },   20.f,   "Hz", f0));
    p.push_back (fl ("highcut",   "High Cut",     { 1000.f, 20000.f, 1.f, 0.4f }, 16000.f, "Hz", fk));
    p.push_back (fl ("lowgain",   "Low Gain",     { -12.f, 12.f, 0.1f },         0.f,    "dB", f1));
    p.push_back (fl ("highgain",  "High Gain",    { -12.f, 12.f, 0.1f },         0.f,    "dB", f1));
    p.push_back (fl ("mix",       "Mix",          { 0.f, 100.f, 1.f },           30.f,   "%",  f0));
    p.push_back (fl ("output",    "Output",       { -24.f, 12.f, 0.1f },         0.f,    "dB", f1));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));

    return { p.begin(), p.end() };
}

RyvrbAudioProcessor::RyvrbAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "RYVRB", createLayout())
{
}

bool RyvrbAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == l.getMainInputChannelSet();
}

void RyvrbAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

void RyvrbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    const int n = buffer.getNumSamples();
    for (int c = numIn; c < numOut; ++c) buffer.clear (c, 0, n);

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    ReverbParams r;
    r.predelayMs = get ("predelay");   r.decay = get ("decay");
    r.lowRatio = get ("lowratio");     r.lowFreq = get ("lowfreq");
    r.highRatio = get ("highratio");   r.highFreq = get ("highfreq");
    r.size = get ("size");             r.diffusion = get ("diffusion");
    r.modulation = get ("modulation"); r.width = get ("width");
    r.duckAmount = get ("duckamt");    r.duckThresholdDb = get ("duckthr");
    r.duckReleaseMs = get ("duckrel"); r.duckAttackMs = get ("duckatk");
    r.lowCutHz = get ("lowcut");       r.highCutHz = get ("highcut");
    r.lowGainDb = get ("lowgain");     r.highGainDb = get ("highgain");
    r.mix = get ("mix");               r.outputDb = get ("output");
    r.bypass = get ("bypass") > 0.5f;
    engine.setParams (r);

    engine.process (buffer.getArrayOfWritePointers(), juce::jmin (numIn, buffer.getNumChannels()), n);
}

const juce::String RyvrbAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= kNumPresets) return {};
    return juce::String (kPresets[index].category) + " - " + kPresets[index].name;
}

void RyvrbAudioProcessor::applyPreset (int index)
{
    if (index < 0 || index >= kNumPresets) return;
    const auto& pr = kPresets[index];
    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    };
    for (int k = 0; k < 18; ++k) set (kPresetParamIds[k], pr.v[k]);
    set ("duckatk", 8.f);
    currentPreset.store (index);
}

void RyvrbAudioProcessor::selectSlot (int slot)
{
    if (slot == currentSlot) return;
    apvts.state.setProperty ("preset", currentPreset.load(), nullptr);
    slotState[currentSlot] = apvts.copyState();
    currentSlot = slot;
    if (slotState[slot].isValid())
    {
        apvts.replaceState (slotState[slot].createCopy());
        currentPreset.store (juce::jlimit (0, kNumPresets - 1, (int) apvts.state.getProperty ("preset", 0)));
    }
}

void RyvrbAudioProcessor::copyToOtherSlot()
{
    apvts.state.setProperty ("preset", currentPreset.load(), nullptr);
    slotState[1 - currentSlot] = apvts.copyState();
}

juce::AudioProcessorEditor* RyvrbAudioProcessor::createEditor()
{
    return new RyvrbAudioProcessorEditor (*this);
}

void RyvrbAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    apvts.state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void RyvrbAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPreset.store (juce::jlimit (0, kNumPresets - 1, (int) apvts.state.getProperty ("preset", 0)));
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RyvrbAudioProcessor();
}
