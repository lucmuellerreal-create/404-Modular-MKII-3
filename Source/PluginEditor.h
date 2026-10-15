#pragma once
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

class Modular404Editor final : public juce::AudioProcessorEditor
{
public:
    explicit Modular404Editor(Modular404Processor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    struct Module {
        juce::ToggleButton on;
        juce::Slider mix, amount;
        juce::Label mixLabel, amountLabel;
        std::unique_ptr<ButtonAttachment> onAttach;
        std::unique_ptr<SliderAttachment> mixAttach, amountAttach;
    };
    Modular404Processor& processor;
    std::array<Module, Modular404Processor::effectCount> modules;
    juce::Slider output;
    juce::ToggleButton bypass;
    juce::Label outputLabel;
    std::unique_ptr<SliderAttachment> outputAttach;
    std::unique_ptr<ButtonAttachment> bypassAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modular404Editor)
};
