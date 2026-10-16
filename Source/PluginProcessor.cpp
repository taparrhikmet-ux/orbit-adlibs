#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

const std::array<const char*, 10> OrbitAdlibsProcessor::ids {
    "chorus", "saturation", "reverb", "delay", "width", "air", "space", "mix", "output", "time"
};

juce::AudioProcessorValueTreeState::ParameterLayout OrbitAdlibsProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto percent = [&p](const char* id, const char* name, float def)
    {
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), def));
    };
    percent("chorus", "Chorus", 35.0f);
    percent("saturation", "Saturation", 20.0f);
    percent("reverb", "Reverb", 25.0f);
    percent("delay", "Delay", 18.0f);
    percent("width", "Width", 65.0f);
    percent("air", "Tone", 65.0f);
    percent("space", "Space", 65.0f);
    percent("mix", "Mix", 100.0f);
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output", 1}, "Output",
        juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f), -3.0f));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"time", 1}, "Delay time",
        juce::StringArray{"1/8", "1/8 dotted", "1/4"}, 1));
    return p;
}

OrbitAdlibsProcessor::OrbitAdlibsProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "OrbitAdlibsState", makeParameters())
{
    for (size_t i = 0; i < ids.size(); ++i)
        raw[i] = state.getRawParameterValue(ids[i]);
}

bool OrbitAdlibsProcessor::isBusesLayoutSupported(const BusesLayout& b) const
{
    const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && (out == juce::AudioChannelSet::stereo() || out == in);
}

void OrbitAdlibsProcessor::prepareToPlay(double sr, int maxBlock)
{
    sampleRate = sr;
    workSize = juce::jmax(64, maxBlock);
    dry.setSize(2, workSize);
    verb.setSize(2, workSize);
    coloured.setSize(2, workSize);
    for (auto& r : chorusRing) r.assign(static_cast<size_t>(sr * 0.08) + 8, 0.0f);
    for (auto& r : echoRing) r.assign(static_cast<size_t>(sr * 2.1) + 8, 0.0f);
    hpInput.fill(0.0f); hpOutput.fill(0.0f); lpState.fill(0.0f); echoDamp.fill(0.0f);
    chorusWrite = echoWrite = 0; phase = 0.0;
    hpPole = std::exp(-2.0f * juce::MathConstants<float>::pi * 140.0f / static_cast<float>(sr));
    reverb.setSampleRate(sr);
    reverb.reset();
    for (size_t i = 0; i < smooth.size(); ++i)
    {
        smooth[i].reset(sr, 0.04);
        smooth[i].setCurrentAndTargetValue(raw[i]->load());
    }
    delaySamples.reset(sr, 0.15);
    delaySamples.setCurrentAndTargetValue(static_cast<float>(sr * 0.375));
    meter.store(0.0f); ceilingActive.store(false);
}

float OrbitAdlibsProcessor::readDelay(const std::vector<float>& ring, int write, float offset)
{
    float pos = static_cast<float>(write) - offset;
    const auto size = static_cast<int>(ring.size());
    if (pos < 0.0f) pos += static_cast<float>(size);
    const int i = static_cast<int>(pos);
    const float f = pos - static_cast<float>(i);
    return ring[static_cast<size_t>(i)] + f * (ring[static_cast<size_t>((i + 1) % size)] - ring[static_cast<size_t>(i)]);
}

void OrbitAdlibsProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    if (n == 0) return;
    const int inputs = getTotalNumInputChannels();
    const int outputs = getTotalNumOutputChannels();
    if (inputs == 1 && outputs == 2) buffer.copyFrom(1, 0, buffer, 0, 0, n);
    for (size_t i = 0; i < smooth.size(); ++i) smooth[i].setTargetValue(raw[i]->load());
    double bpm = 120.0;
    if (auto* playhead = getPlayHead())
        if (auto pos = playhead->getPosition())
            if (auto tempo = pos->getBpm())
                if (std::isfinite(*tempo) && *tempo > 0.0) bpm = *tempo;
    const int timing = juce::jlimit(0, 2, static_cast<int>(raw[9]->load()));
    const float beats[] { 0.5f, 0.75f, 1.0f };
    const double seconds = juce::jlimit(0.03, 2.0, 60.0 / bpm * beats[timing]);
    delaySamples.setTargetValue(static_cast<float>(seconds * sampleRate));
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.35f + raw[6]->load() * 0.0058f;
    rp.damping = 0.55f; rp.wetLevel = 1.0f; rp.dryLevel = 0.0f;
    rp.width = 1.0f; rp.freezeMode = 0.0f;
    reverb.setParameters(rp);
    float blockPeak = 0.0f;
    bool hitCeiling = false;
    const float sr = static_cast<float>(sampleRate);
    for (int start = 0; start < n; start += workSize)
    {
        const int count = juce::jmin(workSize, n - start);
        for (int c = 0; c < 2; ++c)
            dry.copyFrom(c, 0, buffer, juce::jmin(c, outputs - 1), start, count);
        // Render insert coloration, stereo modulation, and tempo echo into a scratch buffer.
        for (int s = 0; s < count; ++s)
        {
            const float ch = smooth[0].getNextValue() * 0.01f;
            const float sat = smooth[1].getNextValue() * 0.01f;
            const float echo = smooth[3].getNextValue() * 0.01f;
            const float tone = smooth[5].getNextValue() * 0.01f;
            smooth[6].getNextValue();
            const float drive = 1.0f + 7.0f * sat;
            const float norm = 1.0f / std::sqrt(drive);
            const float cutoff = 2200.0f * std::pow(8.0f, tone);
            const float pole = std::exp(-2.0f * juce::MathConstants<float>::pi * juce::jmin(cutoff, sr * 0.45f) / sr);
            std::array<float, 2> x {};
            for (int c = 0; c < 2; ++c)
            {
                const float input = dry.getSample(c, s);
                const float high = input - hpInput[c] + hpPole * hpOutput[c];
                hpInput[c] = input; hpOutput[c] = high;
                const float saturated = high * (1.0f - sat) + sat * std::tanh(high * drive) * norm;
                lpState[c] = pole * lpState[c] + (1.0f - pole) * saturated;
                chorusRing[c][static_cast<size_t>(chorusWrite)] = lpState[c];
                const double p = phase + (c == 0 ? 0.0 : juce::MathConstants<double>::halfPi);
                const float milliseconds = 17.0f + 4.0f * static_cast<float>(std::sin(p));
                const float doubled = readDelay(chorusRing[c], chorusWrite, milliseconds * 0.001f * sr);
                x[c] = lpState[c] * (1.0f - 0.45f * ch) + doubled * (0.65f * ch);
            }
            const float dt = delaySamples.getNextValue();
            const float dl = readDelay(echoRing[0], echoWrite, dt);
            const float dr = readDelay(echoRing[1], echoWrite, dt);
            echoDamp[0] = 0.72f * echoDamp[0] + 0.28f * dl;
            echoDamp[1] = 0.72f * echoDamp[1] + 0.28f * dr;
            // Asymmetric first taps make mono sources visibly and audibly ping-pong.
            echoRing[0][static_cast<size_t>(echoWrite)] = x[0] + 0.43f * echoDamp[1];
            echoRing[1][static_cast<size_t>(echoWrite)] = 0.32f * x[1] + 0.43f * echoDamp[0];
            for (int c = 0; c < 2; ++c)
            {
                const float value = x[c] + echo * 0.7f * (c == 0 ? dl : dr);
                coloured.setSample(c, s, value);
                verb.setSample(c, s, value);
            }
            chorusWrite = (chorusWrite + 1) % static_cast<int>(chorusRing[0].size());
            echoWrite = (echoWrite + 1) % static_cast<int>(echoRing[0].size());
            phase += juce::MathConstants<double>::twoPi * 0.32 / sampleRate;
            if (phase >= juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;
        }
        reverb.processStereo(verb.getWritePointer(0), verb.getWritePointer(1), count);
        for (int s = 0; s < count; ++s)
        {
            const float rv = smooth[2].getNextValue() * 0.01f;
            const float width = smooth[4].getNextValue() * 0.02f;
            const float mix = smooth[7].getNextValue() * 0.01f;
            const float gain = juce::Decibels::decibelsToGain(smooth[8].getNextValue());
            float l = coloured.getSample(0, s) + rv * 0.5f * verb.getSample(0, s);
            float r = coloured.getSample(1, s) + rv * 0.5f * verb.getSample(1, s);
            const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * width;
            l = mid + side; r = mid - side;
            for (int c = 0; c < outputs; ++c)
            {
                float wet = outputs == 1 ? mid : (c == 0 ? l : r);
                float y = ((1.0f - mix) * dry.getSample(c, s) + mix * wet) * gain;
                if (!std::isfinite(y)) y = 0.0f;
                hitCeiling = hitCeiling || std::abs(y) > 0.98f;
                y = juce::jlimit(-0.98f, 0.98f, y);
                buffer.setSample(c, start + s, y);
                blockPeak = juce::jmax(blockPeak, std::abs(y));
            }
        }
    }
    meter.store(blockPeak);
    ceilingActive.store(hitCeiling);
}

void OrbitAdlibsProcessor::applyPreset(int index)
{
    static const float presets[4][10] {
        {35, 20, 25, 18, 65, 65, 65, 100, -3, 1},
        {65, 32, 48, 30, 82, 48, 85, 100, -5, 1},
        {22, 58, 16, 14, 55, 34, 45, 100, -5, 0},
        {48, 18, 62, 42, 78, 72, 92, 100, -6, 2}
    };
    index = juce::jlimit(0, 3, index);
    for (size_t i = 0; i < ids.size(); ++i)
        if (auto* p = state.getParameter(ids[i]))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->convertTo0to1(presets[index][i]));
            p->endChangeGesture();
        }
}

void OrbitAdlibsProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, data);
}
void OrbitAdlibsProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* OrbitAdlibsProcessor::createEditor() { return new OrbitAdlibsEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrbitAdlibsProcessor(); }
