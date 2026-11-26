#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    juce::Font monoBold (float h)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::bold));
    }
    juce::String two (int n) { return juce::String (n).paddedLeft ('0', 2); }
}

//==============================================================================
RyLookAndFeel::RyLookAndFeel()
{
    using namespace juce;
    setColour (PopupMenu::backgroundColourId, kBg0);
    setColour (PopupMenu::textColourId, kText);
    setColour (PopupMenu::highlightedBackgroundColourId, kGold);
    setColour (PopupMenu::highlightedTextColourId, kBg0);
    setColour (Label::textColourId, kText);
    setColour (TextButton::textColourOffId, kText);
    setColour (TextButton::textColourOnId, kGold);
}

void RyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startA, float endA, juce::Slider& s)
{
    using namespace juce;
    const bool big = (bool) s.getProperties().getWithDefault ("big", false);
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startA + pos * (endA - startA);

    // Anillo de LEDs
    const int N = big ? 41 : 23;
    const float ringR = R * 0.93f, dot = big ? 2.7f : 1.9f;
    const int head = (int) std::lround (pos * (float) (N - 1));
    for (int k = 0; k < N; ++k)
    {
        const float t = (float) k / (float) (N - 1);
        const float a = startA + t * (endA - startA);
        const float px = c.x + std::sin (a) * ringR, py = c.y - std::cos (a) * ringR;
        if (k <= head)
        {
            const bool isHead = (k == head);
            const auto col = isHead ? kGoldLight : kBlue.interpolatedWith (kBlueLight, t);
            g.setColour ((isHead ? kGold : kBlue).withAlpha (0.25f));
            g.fillEllipse (px - dot * 2.3f, py - dot * 2.3f, dot * 4.6f, dot * 4.6f);
            g.setColour (col);
            const float d = isHead ? dot * 1.35f : dot;
            g.fillEllipse (px - d, py - d, d * 2.f, d * 2.f);
        }
        else
        {
            g.setColour (Colours::white.withAlpha (0.13f));
            g.fillEllipse (px - dot * 0.7f, py - dot * 0.7f, dot * 1.4f, dot * 1.4f);
        }
    }

    // Cuerpo metalico
    const float bodyR = R * 0.74f;
    g.setGradientFill (ColourGradient (Colour (0xff303a5a), c.x - bodyR * 0.45f, c.y - bodyR * 0.65f,
                                       Colour (0xff060912), c.x + bodyR * 0.55f, c.y + bodyR * 0.85f, true));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f);
    g.setColour (kGold.withAlpha (0.65f));
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f, big ? 2.f : 1.4f);

    // Cara interior
    const float faceR = bodyR * 0.80f;
    g.setColour (Colour (0xff050912));
    g.fillEllipse (c.x - faceR, c.y - faceR, faceR * 2.f, faceR * 2.f);
    g.setColour (kBlue.withAlpha (0.20f));
    g.drawEllipse (c.x - faceR, c.y - faceR, faceR * 2.f, faceR * 2.f, 1.f);

    // Marca de posicion
    Path p;
    p.addRoundedRectangle (-1.8f, -bodyR * 0.97f, 3.6f, bodyR * 0.20f, 1.6f);
    p.applyTransform (AffineTransform::rotation (angle).translated (c.x, c.y));
    g.setColour (kGoldLight);
    g.fillPath (p);

    // Lectura digital
    const String text = s.getTextFromValue (s.getValue());
    const float fh = big ? bodyR * 0.46f : bodyR * 0.40f;
    g.setFont (monoBold (fh));
    auto tr = Rectangle<float> (c.x - faceR, c.y - fh, faceR * 2.f, fh * 2.f);
    g.setColour (kBlue.withAlpha (0.20f));
    for (auto o : { Point<float> (1.5f, 0.f), Point<float> (-1.5f, 0.f), Point<float> (0.f, 1.5f), Point<float> (0.f, -1.5f) })
        g.drawText (text, tr.translated (o.x, o.y), Justification::centred, false);
    g.setColour (kBlueLight);
    g.drawText (text, tr, Justification::centred, false);
}

void RyLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& bt, const juce::Colour&,
                                          bool highlighted, bool down)
{
    using namespace juce;
    auto r = bt.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = bt.getToggleState();
    if (bt.getProperties().contains ("tab"))
    {
        g.setColour (on ? kBlue.withAlpha (0.28f) : kBg0.withAlpha (0.9f));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (on ? kGold : Colours::white.withAlpha (highlighted ? 0.45f : 0.2f));
        g.drawRoundedRectangle (r, 4.f, 1.3f);
    }
    else
    {
        g.setColour (on ? kGold : (down ? kBlue.withAlpha (0.45f) : (highlighted ? kBg1.brighter (0.15f) : kBg1)));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (kGold.withAlpha (on ? 0.f : 0.6f));
        g.drawRoundedRectangle (r, 4.f, 1.f);
    }
}

void LevelMeter::paint (juce::Graphics& g)
{
    const int N = 26;
    auto b = getLocalBounds().toFloat();
    const float segH = b.getHeight() / (float) N;
    for (int k = 0; k < N; ++k)
    {
        const float db = -60.f + (float) k * (60.f / (float) N);
        auto r = juce::Rectangle<float> (b.getX(), b.getBottom() - (float) (k + 1) * segH + 1.f, b.getWidth(), segH - 2.f);
        if (levelDb >= db)
            g.setColour (k >= N - 3 ? kGoldLight : kBlue.interpolatedWith (kBlueLight, (float) k / (float) N));
        else
            g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (r, 1.f);
    }
}

//==============================================================================
RyvrbAudioProcessorEditor::RyvrbAudioProcessorEditor (RyvrbAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    logo = juce::Drawable::createFromImageData (BinaryData::logo_svg, (size_t) BinaryData::logo_svgSize);

    addKnob (preK,   "Pre Delay", "predelay", false);
    addKnob (decayK, "Decay",     "decay",    true);
    addKnob (mixK,   "Mix",       "mix",      false);
    addKnob (outK,   "Output",    "output",   false);

    static const char* ids[4][4] = { { "lowratio", "lowfreq", "highratio", "highfreq" },
                                     { "size", "diffusion", "modulation", "width" },
                                     { "duckamt", "duckthr", "duckrel", "duckatk" },
                                     { "lowcut", "highcut", "lowgain", "highgain" } };
    static const char* names[4][4] = { { "Low Ratio", "Low Freq", "High Ratio", "High Freq" },
                                       { "Size", "Diffusion", "Modulation", "Width" },
                                       { "Amount", "Threshold", "Release", "Attack" },
                                       { "Low Cut", "High Cut", "Low Gain", "High Gain" } };
    for (int t = 0; t < 4; ++t)
        for (int k = 0; k < 4; ++k)
            addKnob (tabK[t][k], names[t][k], ids[t][k], false, t == 0);

    static const char* tabNames[4] = { "Damp", "Space", "Ducker", "EQ" };
    for (int t = 0; t < 4; ++t)
    {
        auto& b = tabBtn[t];
        b.setButtonText (tabNames[t]);
        b.getProperties().set ("tab", true);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1);
        b.setColour (juce::TextButton::textColourOnId, kGold);
        b.onClick = [this, t] { showTab (t); };
        addAndMakeVisible (b);
    }
    tabBtn[0].setToggleState (true, juce::dontSendNotification);

    for (auto* b : { &slotA, &slotB })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (2);
        b->setColour (juce::TextButton::textColourOnId, kBg0);
        addAndMakeVisible (*b);
    }
    slotA.setToggleState (proc.currentSlot == 0, juce::dontSendNotification);
    slotB.setToggleState (proc.currentSlot == 1, juce::dontSendNotification);
    slotA.onClick = [this] { proc.selectSlot (0); };
    slotB.onClick = [this] { proc.selectSlot (1); };
    copyAB.setButtonText (juce::String::fromUTF8 ("A \xe2\x86\x92 B"));
    copyAB.onClick = [this] { proc.copyToOtherSlot(); };
    addAndMakeVisible (copyAB);

    bypassBtn.setClickingTogglesState (true);
    bypassBtn.setColour (juce::TextButton::textColourOnId, kBg0);
    addAndMakeVisible (bypassBtn);
    bypassAtt = std::make_unique<ButtonAtt> (proc.apvts, "bypass", bypassBtn);

    topPrev.onClick = [this] { stepPreset (-1); };   topNext.onClick = [this] { stepPreset (1); };
    prePrev.onClick = [this] { stepPreset (-1); };   preNext.onClick = [this] { stepPreset (1); };
    catPrev.onClick = [this] { stepCategory (-1); }; catNext.onClick = [this] { stepCategory (1); };
    for (auto* a : { &topPrev, &topNext, &catPrev, &catNext, &prePrev, &preNext }) addAndMakeVisible (*a);

    topMenuArea.onClick = [this] { showPresetMenu (topMenuArea); };
    rowMenuArea.onClick = [this] { showPresetMenu (rowMenuArea); };
    addAndMakeVisible (topMenuArea);
    addAndMakeVisible (rowMenuArea);
    addAndMakeVisible (inMeter);
    addAndMakeVisible (outMeter);

    setSize (900, 360);
    showTab (0);
    startTimerHz (30);
}

RyvrbAudioProcessorEditor::~RyvrbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void RyvrbAudioProcessorEditor::addKnob (Knob& k, const juce::String& text, const juce::String& id, bool big, bool visible)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.setMouseDragSensitivity (220);
    k.slider.getProperties().set ("big", big);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::Font (juce::FontOptions (big ? 14.f : 12.f, juce::Font::bold)));
    k.label.setColour (juce::Label::textColourId, kText);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
    k.slider.setVisible (visible);
    k.label.setVisible (visible);
    k.att = std::make_unique<SliderAtt> (proc.apvts, id, k.slider);
}

void RyvrbAudioProcessorEditor::showTab (int tab)
{
    currentTab = tab;
    for (int t = 0; t < 4; ++t)
        for (int k = 0; k < 4; ++k)
        {
            tabK[t][k].slider.setVisible (t == tab);
            tabK[t][k].label.setVisible (t == tab);
        }
    repaint();
}

void RyvrbAudioProcessorEditor::resized()
{
    topPrev.setBounds (300, 7, 14, 16);
    topNext.setBounds (586, 7, 14, 16);
    topMenuArea.setBounds (320, 3, 260, 24);
    slotA.setBounds (640, 5, 28, 20);
    slotB.setBounds (672, 5, 28, 20);
    copyAB.setBounds (706, 5, 56, 20);
    bypassBtn.setBounds (790, 5, 84, 20);

    inMeter.setBounds (12, 56, 12, 274);
    outMeter.setBounds (876, 56, 12, 274);

    preK.slider.setBounds (46, 168, 106, 106);   preK.label.setBounds (46, 276, 106, 18);
    decayK.slider.setBounds (190, 78, 224, 224); decayK.label.setBounds (190, 304, 224, 20);

    catPrev.setBounds (448, 62, 14, 16);  catNext.setBounds (718, 62, 14, 16);
    prePrev.setBounds (448, 88, 14, 16);  preNext.setBounds (718, 88, 14, 16);
    rowMenuArea.setBounds (468, 84, 244, 24);

    for (int t = 0; t < 4; ++t) tabBtn[t].setBounds (452 + t * 72, 130, 66, 22);
    for (int t = 0; t < 4; ++t)
        for (int k = 0; k < 4; ++k)
        {
            const int x0 = 446 + k * 72;
            tabK[t][k].slider.setBounds (x0 + 6, 178, 60, 60);
            tabK[t][k].label.setBounds (x0, 240, 72, 18);
        }

    mixK.slider.setBounds (762, 52, 100, 100); mixK.label.setBounds (762, 154, 100, 18);
    outK.slider.setBounds (762, 196, 100, 100); outK.label.setBounds (762, 298, 100, 18);
}

//==============================================================================
void RyvrbAudioProcessorEditor::stepPreset (int d)
{
    const int cur = proc.currentPreset.load();
    const int cat = ryCategoryOf (cur), cnt = ryCountInCategory (cat);
    const int idx = (cur - ryFirstOfCategory (cat) + d + cnt) % cnt;
    proc.applyPreset (ryFirstOfCategory (cat) + idx);
    afterPresetChange();
}

void RyvrbAudioProcessorEditor::stepCategory (int d)
{
    const int n = ryNumCategories();
    const int cat = (ryCategoryOf (proc.currentPreset.load()) + d + n) % n;
    proc.applyPreset (ryFirstOfCategory (cat));
    afterPresetChange();
}

void RyvrbAudioProcessorEditor::showPresetMenu (juce::Component& target)
{
    juce::PopupMenu menu;
    const int cur = proc.currentPreset.load();
    for (int c = 0; c < ryNumCategories(); ++c)
    {
        juce::PopupMenu sub;
        const int first = ryFirstOfCategory (c);
        for (int i = 0; i < ryCountInCategory (c); ++i)
            sub.addItem (first + i + 1, kPresets[first + i].name, true, first + i == cur);
        menu.addSubMenu (two (c + 1) + " " + kPresets[first].category, sub);
    }
    juce::Component::SafePointer<RyvrbAudioProcessorEditor> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                        [safe] (int result)
                        {
                            if (safe != nullptr && result > 0)
                            {
                                safe->proc.applyPreset (result - 1);
                                safe->afterPresetChange();
                            }
                        });
}

void RyvrbAudioProcessorEditor::afterPresetChange()
{
    shownPreset = -1;   // fuerza refresco en el siguiente tick
    repaint();
}

void RyvrbAudioProcessorEditor::timerCallback()
{
    // Medidores con caida suave
    auto toDb = [] (float lin) { return 20.f * std::log10 (lin + 1e-6f); };
    shownIn  = juce::jmax (toDb (proc.engine.inPeak.exchange (0.f)),  shownIn  - 2.4f);
    shownOut = juce::jmax (toDb (proc.engine.outPeak.exchange (0.f)), shownOut - 2.4f);
    inMeter.setLevelDb (shownIn);
    outMeter.setLevelDb (shownOut);

    const int cur = proc.currentPreset.load();
    if (cur != shownPreset)
    {
        shownPreset = cur;
        repaint();
    }

    const bool duckActive = proc.apvts.getRawParameterValue ("duckamt")->load() > 0.5f;
    if (duckActive != duckWasActive)
    {
        duckWasActive = duckActive;
        tabBtn[2].setButtonText (duckActive ? juce::String ("Ducker ") + juce::String::fromUTF8 ("\xe2\x80\xa2")
                                            : juce::String ("Ducker"));
    }
    if (proc.currentSlot == 0 && !slotA.getToggleState()) slotA.setToggleState (true, juce::dontSendNotification);
    if (proc.currentSlot == 1 && !slotB.getToggleState()) slotB.setToggleState (true, juce::dontSendNotification);
}

//==============================================================================
void RyvrbAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    g.setGradientFill (ColourGradient (kBg1, 0.f, 30.f, kBg0, 0.f, (float) getHeight(), false));
    g.fillAll();

    // Barra superior
    g.setColour (kBg0);
    g.fillRect (0, 0, getWidth(), 30);
    g.setColour (kGold.withAlpha (0.8f));
    g.fillRect (0, 30, getWidth(), 1);

    const int cur = proc.currentPreset.load();
    const int cat = ryCategoryOf (cur), first = ryFirstOfCategory (cat);
    const auto& pr = kPresets[cur];

    g.setColour (Colours::white);
    g.setFont (Font (FontOptions (14.f, Font::bold)));
    g.drawText (String (pr.category) + "  -  " + pr.name, 320, 3, 260, 24, Justification::centred);
    g.setColour (kGold.withAlpha (0.7f));
    g.setFont (Font (FontOptions (11.f).withKerningFactor (0.2f)));
    g.drawText ("RYVRB", 16, 3, 100, 24, Justification::centredLeft);

    // Logo, nombre y frase
    if (logo != nullptr)
        logo->drawWithin (g, Rectangle<float> (40.f, 42.f, 38.f, 56.f), RectanglePlacement::centred, 1.f);
    g.setColour (kGold);
    g.setFont (Font (FontOptions (38.f, Font::bold).withKerningFactor (0.08f)));
    g.drawText ("RYVRB", 88, 40, 220, 42, Justification::centredLeft);
    g.setColour (kText.withAlpha (0.8f));
    g.setFont (Font (FontOptions (11.f).withKerningFactor (0.18f)));
    g.drawText ("DEEP BLUE SPATIAL REVERB", 90, 82, 260, 16, Justification::centredLeft);

    g.setColour (kText.withAlpha (0.8f));
    g.setFont (Font (FontOptions (10.f, Font::bold)));
    g.drawText ("In", 6, 38, 24, 14, Justification::centred);
    g.setColour (kGold);
    g.drawText ("Out", 870, 38, 24, 14, Justification::centred);

    // Pantalla de presets
    auto scr = Rectangle<float> (440.f, 50.f, 300.f, 66.f);
    g.setColour (Colour (0xff030712));
    g.fillRoundedRectangle (scr, 8.f);
    g.setColour (kBlue.withAlpha (0.55f));
    g.drawRoundedRectangle (scr, 8.f, 1.4f);
    g.setFont (monoBold (17.f));
    auto row = [&] (const String& text, int y)
    {
        g.setColour (kBlue.withAlpha (0.25f));
        for (int dx = -1; dx <= 1; dx += 2) g.drawText (text, 468 + dx, y, 244, 24, Justification::centred, false);
        g.setColour (kBlueLight);
        g.drawText (text, 468, y, 244, 24, Justification::centred, false);
    };
    row (two (cat + 1) + " " + pr.category, 58);
    row (two (cur - first + 1) + " " + pr.name, 84);

    // Panel de pestanas
    auto panel = Rectangle<float> (440.f, 122.f, 300.f, 208.f);
    g.setColour (kBg0.withAlpha (0.75f));
    g.fillRoundedRectangle (panel, 8.f);
    g.setColour (Colours::white.withAlpha (0.13f));
    g.drawRoundedRectangle (panel, 8.f, 1.f);

    if (currentTab == 0)   // corchetes que unen las parejas Ratio/Freq
    {
        g.setColour (kBlueLight.withAlpha (0.45f));
        for (int pair = 0; pair < 2; ++pair)
        {
            const auto a = tabK[0][pair * 2].slider.getBounds(), b = tabK[0][pair * 2 + 1].slider.getBounds();
            const float xa = (float) a.getCentreX(), xb = (float) b.getCentreX(), yTop = (float) a.getY() - 3.f, yBar = yTop - 8.f;
            Path p;
            p.startNewSubPath (xa, yTop); p.lineTo (xa, yBar); p.lineTo (xb, yBar); p.lineTo (xb, yTop);
            g.strokePath (p, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    // Marco de la zona principal
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (Rectangle<float> (34.f, 40.f, 832.f, 314.f), 10.f, 1.f);
}
