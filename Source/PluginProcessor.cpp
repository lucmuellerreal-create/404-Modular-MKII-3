#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

Modular404Processor::Modular404Processor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", makeParameters()) {}

juce::AudioProcessorValueTreeState::ParameterLayout Modular404Processor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < effectCount; ++i) {
        auto suffix = juce::String(i);
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"on" + suffix, 1}, juce::String(effectNames[(size_t)i]) + " Enabled", false));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix" + suffix, 1}, juce::String(effectNames[(size_t)i]) + " Mix", juce::NormalisableRange<float>(0.f, 1.f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"amount" + suffix, 1}, juce::String(effectNames[(size_t)i]) + " Amount", juce::NormalisableRange<float>(0.f, 1.f), 0.5f));
    }
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output", 1}, "Output Gain", juce::NormalisableRange<float>(0.f, 2.f), 1.f));
    return layout;
}

bool Modular404Processor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto input = layout.getMainInputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
           && layout.getMainOutputChannelSet() == input;
}

void Modular404Processor::prepareToPlay(double sampleRate, int)
{
    sr = std::max(8000.0, sampleRate);
    for (auto& c : channels) {
        c = ChannelState{};
        c.echo.assign((size_t)(sr * 2.0) + 1, 0.f);
        c.chorus.assign((size_t)(sr * 0.07) + 2, 0.f);
        c.reverb.assign((size_t)(sr * 0.23) + 2, 0.f);
    }
}

float Modular404Processor::randomBipolar() noexcept
{
    noiseSeed ^= noiseSeed << 13;
    noiseSeed ^= noiseSeed >> 17;
    noiseSeed ^= noiseSeed << 5;
    return (float)(noiseSeed & 0x00ffffffu) * (2.f / 16777215.f) - 1.f;
}

void Modular404Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numChannels = std::min(2, buffer.getNumChannels());
    if (*parameters.getRawParameterValue("bypass") > 0.5f) return;
    std::array<bool, effectCount> enabled {};
    std::array<float, effectCount> mix {}, amount {};
    for (int e = 0; e < effectCount; ++e) {
        const auto id = juce::String(e);
        enabled[(size_t)e] = *parameters.getRawParameterValue("on" + id) > 0.5f;
        mix[(size_t)e] = *parameters.getRawParameterValue("mix" + id);
        amount[(size_t)e] = *parameters.getRawParameterValue("amount" + id);
    }
    const float output = *parameters.getRawParameterValue("output");
    const float fs = (float)sr;
    for (int ch = 0; ch < numChannels; ++ch) {
        auto& c = channels[(size_t)ch];
        auto* samples = buffer.getWritePointer(ch);
        for (int n = 0; n < buffer.getNumSamples(); ++n) {
            float x = samples[n];
            // Fixed order, each module has an independent wet/dry control.
            if (enabled[0]) {
                const float a = amount[0];
                c.vinylPhase += (0.3f + a * 1.7f) / fs;
                if (c.vinylPhase >= 1.f) c.vinylPhase -= 1.f;
                const float flutter = std::sin(c.vinylPhase * juce::MathConstants<float>::twoPi);
                c.noiseLp += 0.005f * (randomBipolar() - c.noiseLp);
                const float wet = std::tanh(x * (1.f + 0.8f * a)) * (0.98f - a * 0.025f * flutter)
                                  + c.noiseLp * a * 0.035f;
                x += mix[0] * (wet - x);
            }
            if (enabled[1]) {
                const float a = amount[1];
                const int hold = 1 + (int)(a * a * 35.f);
                if (c.holdCounter++ >= hold) { c.holdCounter = 0; c.heldSample = x; }
                const float steps = std::pow(2.f, 15.f - a * 11.f);
                const float wet = std::round(c.heldSample * steps) / steps;
                x += mix[1] * (wet - x);
            }
            if (enabled[2]) {
                const float a = amount[2];
                const float lpAlpha = std::clamp(2.f * juce::MathConstants<float>::pi * (250.f + 4500.f * (1.f - a)) / fs, 0.001f, 0.95f);
                c.lp += lpAlpha * (x - c.lp);
                const float hpAlpha = std::clamp(2.f * juce::MathConstants<float>::pi * (35.f + 650.f * a) / fs, 0.001f, 0.95f);
                c.hp += hpAlpha * (x - c.hp);
                const float wet = c.lp - c.hp * (0.35f + 0.55f * a) + x * (0.15f + 0.35f * a);
                x += mix[2] * (wet - x);
            }
            if (enabled[3]) {
                const float drive = 1.f + 20.f * amount[3] * amount[3];
                const float wet = std::tanh(x * drive) / std::tanh(drive);
                x += mix[3] * (wet - x);
            }
            if (enabled[4] && !c.chorus.empty()) {
                const float a = amount[4];
                c.chorusPhase += (0.15f + a * 2.f) / fs;
                if (c.chorusPhase >= 1.f) c.chorusPhase -= 1.f;
                const float delaySamples = fs * (0.008f + (0.003f + 0.007f * a) *
                    (1.f + std::sin(juce::MathConstants<float>::twoPi * c.chorusPhase)));
                const auto size = c.chorus.size();
                const float readPos = (float)c.chorusIndex + (float)size - delaySamples;
                const float wrapped = std::fmod(readPos + (float)size, (float)size);
                const size_t i0 = (size_t)wrapped;
                const size_t i1 = (i0 + 1) % size;
                const float frac = wrapped - (float)i0;
                const float wet = c.chorus[i0] * (1.f - frac) + c.chorus[i1] * frac;
                c.chorus[c.chorusIndex] = x;
                c.chorusIndex = (c.chorusIndex + 1) % size;
                x += mix[4] * (wet - x);
            }
            if (!c.echo.empty()) {
                const auto size = c.echo.size();
                const size_t delay = (size_t)(fs * (0.09f + amount[5] * 0.76f));
                const size_t read = (c.echoIndex + size - std::min(delay, size - 1)) % size;
                const float wet = c.echo[read];
                c.echo[c.echoIndex] = std::tanh(x + wet * (enabled[5] ? 0.38f + amount[5] * 0.42f : 0.f));
                c.echoIndex = (c.echoIndex + 1) % size;
                if (enabled[5]) x += mix[5] * (wet - x);
            }
            if (!c.reverb.empty()) {
                const auto size = c.reverb.size();
                const size_t delay = (size_t)(fs * (0.061f + amount[6] * 0.13f));
                const float delayed = c.reverb[(c.reverbIndex + size - std::min(delay, size - 1)) % size];
                c.reverbLp += 0.18f * (delayed - c.reverbLp);
                c.reverb[c.reverbIndex] = std::tanh(x * 0.35f + c.reverbLp * (enabled[6] ? 0.6f + 0.3f * amount[6] : 0.f));
                c.reverbIndex = (c.reverbIndex + 1) % size;
                if (enabled[6]) x += mix[6] * (c.reverbLp - x);
            }
            if (enabled[7]) {
                c.tremPhase += (0.5f + amount[7] * 13.f) / fs;
                if (c.tremPhase >= 1.f) c.tremPhase -= 1.f;
                const float mod = 0.5f + 0.5f * std::sin(juce::MathConstants<float>::twoPi * c.tremPhase);
                const float wet = x * mod;
                x += mix[7] * (wet - x);
            }
            samples[n] = std::isfinite(x) ? x * output : 0.f;
        }
    }
}

void Modular404Processor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void Modular404Processor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* Modular404Processor::createEditor() { return new Modular404Editor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Modular404Processor(); }
