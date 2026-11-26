#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Paleta: azul fuerte + dorado (el dorado y el azul del escudo de referencia)
inline const juce::Colour kBlue      { 0xff2a5cff };
inline const juce::Colour kBlueLight { 0xff86abff };
inline const juce::Colour kBg0       { 0xff050a17 };
inline const juce::Colour kBg1       { 0xff0b1631 };
inline const juce::Colour kGold      { 0xffd59f0f };
inline const juce::Colour kGoldLight { 0xfff2c94c };
inline const juce::Colour kText      { 0xffd3defc };

class RyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RyLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
};

// Medidor de nivel con segmentos LED
class LevelMeter : public juce::Component
{
public:
    void setLevelDb (float db) { levelDb = db; repaint(); }
    void paint (juce::Graphics&) override;
private:
    float levelDb = -90.f;
};

// Zona clicable transparente
struct ClickArea : public juce::Component
{
    std::function<void()> onClick;
    ClickArea() { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
};

class RyvrbAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RyvrbAudioProcessorEditor (RyvrbAudioProcessor&);
    ~RyvrbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAtt> att;
    };

    void timerCallback() override;
    void addKnob (Knob& k, const juce::String& text, const juce::String& paramId, bool big, bool visible = true);
    void showTab (int tab);
    void stepPreset (int delta);
    void stepCategory (int delta);
    void showPresetMenu (juce::Component& target);
    void afterPresetChange();

    RyvrbAudioProcessor& proc;
    RyLookAndFeel laf;
    std::unique_ptr<juce::Drawable> logo;

    Knob preK, decayK, mixK, outK;
    Knob tabK[4][4];
    juce::TextButton tabBtn[4], slotA { "A" }, slotB { "B" }, copyAB { "" }, bypassBtn { "BYPASS" };
    juce::ArrowButton topPrev { "tp", 0.5f, kGold }, topNext { "tn", 0.0f, kGold },
                      catPrev { "cp", 0.5f, kGold }, catNext { "cn", 0.0f, kGold },
                      prePrev { "pp", 0.5f, kGold }, preNext { "pn", 0.0f, kGold };
    ClickArea topMenuArea, rowMenuArea;
    LevelMeter inMeter, outMeter;
    std::unique_ptr<ButtonAtt> bypassAtt;

    int currentTab = 0, shownPreset = -1;
    float shownIn = -90.f, shownOut = -90.f;
    bool duckWasActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RyvrbAudioProcessorEditor)
};
