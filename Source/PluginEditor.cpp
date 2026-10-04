#include "PluginEditor.h"

FlyAudioProcessorEditor::FlyAudioProcessorEditor(FlyAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p), visualizer(p.graph(), p.shared(), p.parameters) {
    theme.setColour(juce::Slider::thumbColourId, juce::Colour(0xff80e8ce));
    theme.setColour(juce::Slider::trackColourId, juce::Colour(0xff355a61));
    theme.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff172638));
    theme.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff172638));
    theme.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff172638));
    setLookAndFeel(&theme);
    const auto label = [this](juce::Label& l, const juce::String& text, float size, juce::Colour c) {
        l.setText(text, juce::dontSendNotification); l.setFont(juce::FontOptions(size)); l.setColour(juce::Label::textColourId, c); addAndMakeVisible(l); };
    label(title, "Drosophila melanogaster", 25, juce::Colour(0xffd8fff2));
    label(subtitle, "Lazirko Records | FAFB v783 | Connectome-constrained auditory LIF model", 13, juce::Colour(0xff7e9baa));
    label(meters, "", 14, juce::Colour(0xffb8d8df));
    label(status, "", 12, juce::Colour(0xff83b7a9));
    label(legend, "JO-A / JO-B -> AMMC-A1 / A2 / B1 / B2 <-> WED / AVLP / PVLP / IPS / SAD / GNG", 13, juce::Colour(0xffaac4d4));
    label(viewHint, "Drag to pan | Wheel to zoom | Double-click to reset | Stage layout is schematic, not feedforward", 11, juce::Colour(0xff718b9e));
    label(mapping, "", 12, juce::Colour(0xffa7d2bf));
    addAndMakeVisible(visualizer);
    constexpr const char* ids[] {"sensoryGain", "balance", "centre", "bands", "speed", "wet", "depth", "brightness", "decay"};
    constexpr const char* names[] {"Johnston's organ gain", "JO-B / JO-A sensory balance", "JO band centre (Hz)", "Band separation (octaves)", "Simulation time-scale", "Wet / dry", "Neural influence", "Spike brightness", "Spike decay (s)"};
    for (std::size_t i = 0; i < sliders.size(); ++i) {
        label(labels[i], names[i], 12, juce::Colour(0xffb5c9d7));
        sliders[i].setSliderStyle(juce::Slider::LinearHorizontal); sliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 64, 20);
        sliders[i].setNumDecimalPlacesToDisplay(i == 2 ? 0 : 2); addAndMakeVisible(sliders[i]);
        sliderAttachments.push_back(std::make_unique<SliderAttachment>(processor.parameters, ids[i], sliders[i]));
    }
    label(modeLabel, "Downstream auditory output", 12, juce::Colour(0xffb5c9d7));
    mode.addItemList({"Auditory filter (clean)", "Auditory gating / compression", "Auditory waveshaping", "Artistic circuit blend"}, 1); addAndMakeVisible(mode);
    modeAttachment = std::make_unique<ComboAttachment>(p.parameters, "mode", mode);
    label(colourLabel, "Neuron colour", 12, juce::Colour(0xffb5c9d7));
    colour.addItemList({"Population", "Predicted neurotransmitter", "Recent spiking activity"}, 1); addAndMakeVisible(colour);
    colourAttachment = std::make_unique<ComboAttachment>(p.parameters, "colour", colour);
    addAndMakeVisible(connections); addAndMakeVisible(spatial); addAndMakeVisible(disableVisualization);
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(p.parameters, "disableVisualization", disableVisualization));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(p.parameters, "connections", connections));
    buttonAttachments.push_back(std::make_unique<ButtonAttachment>(p.parameters, "spatial", spatial));
    for (std::size_t i = 0; i < populations.size(); ++i) {
        populations[i].setButtonText(fly::populationNames[i]); addAndMakeVisible(populations[i]);
        buttonAttachments.push_back(std::make_unique<ButtonAttachment>(p.parameters, "show" + juce::String(static_cast<int>(i)), populations[i]));
    }
    setResizable(true, true); setResizeLimits(1020, 900, 1800, 1400); setSize(1220, 940); startTimerHz(30); timerCallback();
}
FlyAudioProcessorEditor::~FlyAudioProcessorEditor() { stopTimer(); setLookAndFeel(nullptr); }
void FlyAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff0b1422));
    g.setColour(juce::Colour(0xff101d2d)); g.fillRoundedRectangle(static_cast<float>(getWidth() - 294), 85, 278, static_cast<float>(getHeight() - 136), 12);
    g.setColour(juce::Colour(0xff263b4e)); g.drawHorizontalLine(76, 20, static_cast<float>(getWidth() - 20));
}
void FlyAudioProcessorEditor::resized() {
    title.setBounds(22, 14, 550, 32); subtitle.setBounds(24, 48, 900, 21);
    const int panelX = getWidth() - 280, panelWidth = 250;
    visualizer.setBounds(20, 105, getWidth() - 330, getHeight() - 260);
    legend.setBounds(24, 82, getWidth() - 320, 22);
    viewHint.setBounds(24, getHeight() - 150, getWidth() - 320, 22);
    mapping.setBounds(24, getHeight() - 124, getWidth() - 40, 26);
    meters.setBounds(24, getHeight() - 91, getWidth() - 40, 26);
    status.setBounds(24, getHeight() - 58, getWidth() - 40, 40);
    int y = 99;
    for (std::size_t i = 0; i < sliders.size(); ++i) { labels[i].setBounds(panelX, y, panelWidth, 18); sliders[i].setBounds(panelX, y + 17, panelWidth, 25); y += 43; }
    modeLabel.setBounds(panelX, y, panelWidth, 18); mode.setBounds(panelX, y + 20, panelWidth, 26); y += 53;
    colourLabel.setBounds(panelX, y, panelWidth, 18); colour.setBounds(panelX, y + 20, panelWidth, 26); y += 53;
    connections.setBounds(panelX, y, panelWidth, 24); spatial.setBounds(panelX, y + 26, panelWidth, 24);
    disableVisualization.setBounds(panelX, y + 52, panelWidth, 24); y += 81;
    for (std::size_t i = 0; i < populations.size(); ++i) populations[i].setBounds(panelX + static_cast<int>(i % 2) * 125, y + static_cast<int>(i / 2) * 24, 125, 24);
}
void FlyAudioProcessorEditor::timerCallback() {
    visualizer.setRenderingEnabled(processor.parameters.getRawParameterValue("disableVisualization")->load() < 0.5f);
    auto& t = processor.shared().telemetry;
    const auto rate = [&t](std::size_t p) { return t.audioRates[p].load(std::memory_order_relaxed); };
    const auto centreHz = processor.parameters.getRawParameterValue("centre")->load();
    const auto spacing = std::pow(2.0f, processor.parameters.getRawParameterValue("bands")->load() * 0.5f);
    mapping.setText("JO-B / JO-A band centres: " + juce::String(centreHz / spacing, 0) + " / " + juce::String(centreHz * spacing, 0)
                    + " Hz | " + mode.getText() + " | Rates below are the latest audio-control inputs", juce::dontSendNotification);
    const auto input = juce::Decibels::gainToDecibels(t.input.load(), -90.0f);
    const auto totalJO = processor.graph().counts[0] + processor.graph().counts[1];
    const auto jo = totalJO ? (rate(0) * static_cast<float>(processor.graph().counts[0]) + rate(1) * static_cast<float>(processor.graph().counts[1])) / static_cast<float>(totalJO) : 0;
    meters.setText("INPUT  " + juce::String(input, 1) + " dBFS    |    JO  " + juce::String(jo, 1) + " Hz    |    AMMC-B1  " + juce::String(rate(4), 1) + " Hz    |    Central auditory  " + juce::String(rate(6), 1) + " Hz", juce::dontSendNotification);
    juce::String text;
    if (processor.dataError().isNotEmpty()) text = "DATA ERROR - DRY BYPASS: " + processor.dataError();
    else if (visualizer.rendererError().isNotEmpty()) text = "OpenGL error: " + visualizer.rendererError();
    else text = juce::String(static_cast<int>(processor.graph().neurons.size())) + " verified v783 neurons | " + juce::String(static_cast<int>(processor.graph().synapses.size()))
         + " directed edges | LIF load " + juce::String(t.workerLoad.load() * 100, 1) + "% | dropped audio / visual events "
         + juce::String(t.droppedFeatures.load()) + " / " + juce::String(t.droppedSpikes.load()) + " | Model, not calibrated fly hearing | JO detection incomplete";
    status.setText(text, juce::dontSendNotification);
}
