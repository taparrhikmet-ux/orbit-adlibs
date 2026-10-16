#pragma once
#include "PluginProcessor.h"

class OrbitLook final : public juce::LookAndFeel_V4
{
public:
    OrbitLook();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class OrbitAdlibsEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OrbitAdlibsEditor(OrbitAdlibsProcessor&);
    ~OrbitAdlibsEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    OrbitAdlibsProcessor& processor;
    OrbitLook look;
    std::array<juce::Slider, 9> knobs;
    std::array<juce::Label, 9> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 9> attachments;
    juce::ComboBox presets, timing;
    juce::Label presetLabel, timingLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> timeAttachment;
    float displayedPeak = 0.0f;
    int clipFrames = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitAdlibsEditor)
};
