#include "PluginEditor.h"

namespace {
constexpr auto background = juce::Colour(0xff11131a);
constexpr auto panel = juce::Colour(0xff20242f);
constexpr auto accent = juce::Colour(0xffffa957);
constexpr auto textColour = juce::Colour(0xfff1f0ed);
void configureKnob(juce::Slider& s) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    s.setColour(juce::Slider::rotarySliderFillColourId, accent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff404756));
    s.setColour(juce::Slider::thumbColourId, accent);
    s.setColour(juce::Slider::textBoxTextColourId, textColour);
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
}

Modular404Editor::Modular404Editor(Modular404Processor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    setSize(900, 565);
    for (int i = 0; i < Modular404Processor::effectCount; ++i) {
        auto& m = modules[(size_t)i];
        const auto id = juce::String(i);
        m.on.setButtonText(Modular404Processor::effectNames[(size_t)i]);
        m.on.setColour(juce::ToggleButton::textColourId, textColour);
        m.on.setColour(juce::ToggleButton::tickColourId, accent);
        addAndMakeVisible(m.on);
        configureKnob(m.mix);
        configureKnob(m.amount);
        addAndMakeVisible(m.mix);
        addAndMakeVisible(m.amount);
        m.mixLabel.setText("MIX", juce::dontSendNotification);
        m.amountLabel.setText("AMOUNT", juce::dontSendNotification);
        for (auto* l : {&m.mixLabel, &m.amountLabel}) {
            l->setJustificationType(juce::Justification::centred);
            l->setColour(juce::Label::textColourId, juce::Colour(0xffabb1c1));
            addAndMakeVisible(*l);
        }
        m.onAttach = std::make_unique<ButtonAttachment>(p.parameters, "on" + id, m.on);
        m.mixAttach = std::make_unique<SliderAttachment>(p.parameters, "mix" + id, m.mix);
        m.amountAttach = std::make_unique<SliderAttachment>(p.parameters, "amount" + id, m.amount);
    }
    configureKnob(output);
    output.setRange(0.0, 2.0);
    addAndMakeVisible(output);
    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setColour(juce::Label::textColourId, textColour);
    outputLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputLabel);
    bypass.setButtonText("BYPASS");
    bypass.setColour(juce::ToggleButton::textColourId, textColour);
    bypass.setColour(juce::ToggleButton::tickColourId, accent);
    addAndMakeVisible(bypass);
    outputAttach = std::make_unique<SliderAttachment>(p.parameters, "output", output);
    bypassAttach = std::make_unique<ButtonAttachment>(p.parameters, "bypass", bypass);
}

void Modular404Editor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(textColour);
    g.setFont(juce::Font(juce::FontOptions(29.f, juce::Font::bold)));
    g.drawText("404 / MODULAR", 25, 15, 450, 42, juce::Justification::centredLeft);
    g.setColour(accent);
    g.setFont(juce::Font(juce::FontOptions(12.f)));
    g.drawText("MKII INSPIRED  •  MULTI FX  •  V1.0 FOUNDATION", 27, 54, 500, 20, juce::Justification::centredLeft);
    for (int i = 0; i < Modular404Processor::effectCount; ++i) {
        const int col = i % 4, row = i / 4;
        const auto rect = juce::Rectangle<float>((float)(20 + col * 220), (float)(100 + row * 207), 205.f, 190.f);
        g.setColour(panel);
        g.fillRoundedRectangle(rect, 13.f);
        g.setColour(juce::Colour(0xff373d4a));
        g.drawRoundedRectangle(rect, 13.f, 1.f);
        g.setColour(accent.withAlpha(0.65f));
        g.fillRoundedRectangle(rect.getX() + 15.f, rect.getY() + 42.f, 175.f, 2.f, 1.f);
    }
    g.setColour(juce::Colour(0xff7c8393));
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawText("INDEPENDENT AUDIO  •  ORIGINAL DSP, NOT A ROLAND PRODUCT", 24, 530, 610, 22, juce::Justification::centredLeft);
}

void Modular404Editor::resized()
{
    for (int i = 0; i < Modular404Processor::effectCount; ++i) {
        auto& m = modules[(size_t)i];
        const int x = 20 + (i % 4) * 220, y = 100 + (i / 4) * 207;
        m.on.setBounds(x + 12, y + 10, 180, 28);
        m.mix.setBounds(x + 12, y + 53, 88, 105);
        m.amount.setBounds(x + 105, y + 53, 88, 105);
        m.mixLabel.setBounds(x + 14, y + 158, 84, 21);
        m.amountLabel.setBounds(x + 105, y + 158, 88, 21);
    }
    output.setBounds(733, 5, 90, 88);
    outputLabel.setBounds(734, 81, 88, 17);
    bypass.setBounds(825, 34, 75, 26);
}
