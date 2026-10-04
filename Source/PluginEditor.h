#pragma once
#include "PluginProcessor.h"
#include "Visualizer.h"

class FlyAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit FlyAudioProcessorEditor(FlyAudioProcessor&);
    ~FlyAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    FlyAudioProcessor& processor;
    juce::LookAndFeel_V4 theme;
    AuditoryVisualizer visualizer;
    juce::Label title, subtitle, meters, status, legend, viewHint, mapping;
    std::array<juce::Slider, 9> sliders;
    std::array<juce::Label, 9> labels;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    juce::ComboBox mode, colour;
    juce::Label modeLabel, colourLabel;
    std::unique_ptr<ComboAttachment> modeAttachment, colourAttachment;
    juce::ToggleButton connections {"Synaptic connections"}, spatial {"FAFB anchor coordinates"}, disableVisualization {"Disable visualisation"};
    std::array<juce::ToggleButton, fly::populationCount> populations;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlyAudioProcessorEditor)
};
