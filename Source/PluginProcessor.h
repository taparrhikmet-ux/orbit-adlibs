#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class OrbitAdlibsProcessor final : public juce::AudioProcessor
{
public:
    OrbitAdlibsProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Orbit Adlibs"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    void applyPreset(int);
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> meter { 0.0f };
    std::atomic<bool> ceilingActive { false };
    static const std::array<const char*, 10> ids;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    std::array<std::atomic<float>*, 10> raw {};
    std::array<juce::SmoothedValue<float>, 9> smooth;
    juce::SmoothedValue<float> delaySamples;
    juce::Reverb reverb;
    juce::AudioBuffer<float> dry, verb, coloured;
    std::array<std::vector<float>, 2> chorusRing, echoRing;
    std::array<float, 2> hpInput {}, hpOutput {}, lpState {}, echoDamp {};
    double sampleRate = 44100.0, phase = 0.0;
    float hpPole = 0.98f;
    int chorusWrite = 0, echoWrite = 0, workSize = 512;
    static float readDelay(const std::vector<float>&, int, float);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitAdlibsProcessor)
};
