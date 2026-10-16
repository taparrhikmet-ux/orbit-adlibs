#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour background(0xff0d1018), panel(0xff171c28), text(0xffedf0f9);
const juce::Colour muted(0xff8a94aa), purple(0xffab8bff), cyan(0xff62e3db);
}
OrbitLook::OrbitLook()
{
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId, panel);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff323c50));
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, text);
}
void OrbitLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                 float pos, float start, float end, juce::Slider& slider)
{
    auto b = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                    static_cast<float>(w), static_cast<float>(h)).reduced(10.0f);
    const float radius = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto centre = b.getCentre();
    const float angle = start + pos * (end - start);
    juce::Path track, active;
    track.addCentredArc(centre.x, centre.y, radius - 2.0f, radius - 2.0f, 0.0f, start, end, true);
    active.addCentredArc(centre.x, centre.y, radius - 2.0f, radius - 2.0f, 0.0f, start, angle, true);
    g.setColour(juce::Colour(0xff30394e));
    g.strokePath(track, juce::PathStrokeType(4.0f));
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
    g.setColour(accent); g.strokePath(active, juce::PathStrokeType(4.0f));
    const float inner = radius - 10.0f;
    g.setColour(juce::Colour(0xff222a3b));
    g.fillEllipse(centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
    g.setColour(accent.brighter(0.3f));
    g.drawLine(centre.x + std::sin(angle) * inner * 0.58f,
               centre.y - std::cos(angle) * inner * 0.58f,
               centre.x + std::sin(angle) * inner * 0.85f,
               centre.y - std::cos(angle) * inner * 0.85f, 3.0f);
}

OrbitAdlibsEditor::OrbitAdlibsEditor(OrbitAdlibsProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look);
    const char* names[] {"CHORUS", "SATURATION", "REVERB", "DELAY", "WIDTH", "TONE", "SPACE", "MIX", "OUTPUT"};
    const char* tips[] {
        "Stereo modulated doubling. Zero removes the chorus blend.",
        "Soft-clipped harmonic drive. Zero removes saturation.",
        "Parallel reverb return level. Zero removes reverb from the output.",
        "Tempo-synced ping-pong echo return level.",
        "Stereo side gain: 0 = mono, 50 = neutral, 100 = extra wide. Mono output stays mono.",
        "Dark-to-bright low-pass shaping, about 2.2 kHz to 17.6 kHz.",
        "Reverb room size. Larger values give longer, more diffuse space.",
        "Original vocal / complete processed vocal blend. Not an FX-only send control.",
        "Final output trim in dB. Lower this if the ceiling indicator lights."
    };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 23);
        k.setColour(juce::Slider::rotarySliderFillColourId, i < 3 ? purple : cyan);
        k.setTextValueSuffix(i == 8 ? " dB" : " %");
        k.setTooltip(tips[i]);
        k.setDoubleClickReturnValue(true, i == 8 ? -3.0 : static_cast<double>(p.state.getParameter(OrbitAdlibsProcessor::ids[i])->convertFrom0to1(p.state.getParameter(OrbitAdlibsProcessor::ids[i])->getDefaultValue())));
        addAndMakeVisible(k);
        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setFont(juce::Font(13.0f, juce::Font::bold));
        labels[i].setColour(juce::Label::textColourId, i < 3 ? text : muted);
        addAndMakeVisible(labels[i]);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state, OrbitAdlibsProcessor::ids[i], k);
    }
    presets.addItemList({"Clean Orbit", "Wide Haze", "Dark Grit", "Space Throw"}, 1);
    presets.setSelectedId(1, juce::dontSendNotification);
    presets.setTextWhenNothingSelected("Preset / custom");
    presets.onChange = [this] { if (presets.getSelectedId() > 0) processor.applyPreset(presets.getSelectedId() - 1); };
    addAndMakeVisible(presets);
    timing.addItemList({"1/8", "1/8 dotted", "1/4"}, 1);
    addAndMakeVisible(timing);
    timeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state, "time", timing);
    presetLabel.setText("STARTING POINT", juce::dontSendNotification);
    timingLabel.setText("ECHO SYNC", juce::dontSendNotification);
    for (auto* l : { &presetLabel, &timingLabel })
    {
        l->setColour(juce::Label::textColourId, muted);
        l->setFont(juce::Font(10.0f, juce::Font::bold));
        addAndMakeVisible(l);
    }
    setSize(840, 590);
    startTimerHz(24);
}
OrbitAdlibsEditor::~OrbitAdlibsEditor() { stopTimer(); setLookAndFeel(nullptr); }
void OrbitAdlibsEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(purple); g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText("ORBIT", 30, 16, 180, 40, juce::Justification::centredLeft);
    g.setColour(text); g.setFont(juce::Font(16.0f));
    g.drawText("A D L I B S", 32, 55, 220, 25, juce::Justification::centredLeft);
    g.setColour(muted); g.setFont(juce::Font(11.0f));
    g.drawText("WIDE. DIRTY. OUT OF THIS WORLD.", 32, 81, 310, 20, juce::Justification::centredLeft);
    g.setColour(panel); g.fillRoundedRectangle(22.0f, 122.0f, 796.0f, 245.0f, 14.0f);
    g.fillRoundedRectangle(22.0f, 383.0f, 796.0f, 148.0f, 14.0f);
    g.setColour(muted); g.setFont(juce::Font(11.0f));
    g.drawText("ADLIB INSERT  /  140 Hz LOW CUT  /  HOST-SYNC ECHO", 30, 552, 560, 22, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff30394e));
    g.fillRoundedRectangle(613.0f, 558.0f, 154.0f, 7.0f, 3.0f);
    const float normal = juce::jlimit(0.0f, 1.0f, (juce::Decibels::gainToDecibels(displayedPeak, -60.0f) + 60.0f) / 60.0f);
    if (normal > 0.0f) { g.setColour(cyan); g.fillRoundedRectangle(613.0f, 558.0f, 154.0f * normal, 7.0f, 3.0f); }
    g.setColour(clipFrames > 0 ? juce::Colour(0xffff6f83) : juce::Colour(0xff30394e));
    g.fillEllipse(784.0f, 555.0f, 12.0f, 12.0f);
}
void OrbitAdlibsEditor::resized()
{
    presetLabel.setBounds(463, 28, 165, 18); presets.setBounds(463, 50, 165, 30);
    timingLabel.setBounds(650, 28, 157, 18); timing.setBounds(650, 50, 157, 30);
    for (int i = 0; i < 3; ++i)
    {
        labels[static_cast<size_t>(i)].setBounds(46 + i * 260, 137, 228, 27);
        knobs[static_cast<size_t>(i)].setBounds(68 + i * 260, 166, 184, 184);
    }
    for (int j = 0; j < 6; ++j)
    {
        const auto i = static_cast<size_t>(j + 3);
        labels[i].setBounds(29 + j * 131, 394, 127, 23);
        knobs[i].setBounds(39 + j * 131, 417, 107, 108);
    }
}
void OrbitAdlibsEditor::timerCallback()
{
    displayedPeak = juce::jmax(processor.meter.load(), displayedPeak * 0.83f);
    if (processor.ceilingActive.load()) clipFrames = 24;
    else if (clipFrames > 0) --clipFrames;
    repaint();
}
