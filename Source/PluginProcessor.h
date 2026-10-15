#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <cmath>

class Modular404Processor final : public juce::AudioProcessor
{
public:
    static constexpr int effectCount = 8;
    static constexpr std::array<const char*, effectCount> effectNames {
        "Vinyl Sim", "Lo-Fi", "Isolator", "Drive", "Chorus", "Tape Echo", "Reverb", "Tremolo"
    };
    Modular404Processor();
    ~Modular404Processor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "404 Modular MKII"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
private:
    struct ChannelState {
        float lp = 0, hp = 0, prev = 0, noiseLp = 0, chorusPhase = 0, tremPhase = 0;
        float reverbLp = 0, reverbFeedback = 0, vinylPhase = 0;
        int holdCounter = 0;
        float heldSample = 0;
        std::vector<float> echo, chorus, reverb;
        size_t echoIndex = 0, chorusIndex = 0, reverbIndex = 0;
    };
    std::array<ChannelState, 2> channels;
    double sr = 44100.0;
    uint32_t noiseSeed = 0x72ac915bu;
    float randomBipolar() noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modular404Processor)
};
