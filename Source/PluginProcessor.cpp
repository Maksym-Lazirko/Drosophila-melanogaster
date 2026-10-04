#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>
#include <chrono>

juce::AudioProcessorValueTreeState::ParameterLayout FlyAudioProcessor::parameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto add = [&layout](const char* id, const char* name, float low, float high, float initial, float skew = 1.0f) {
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
                    juce::NormalisableRange<float>{low, high, 0.0f, skew}, initial)); };
    add("sensoryGain", "Johnston's organ sensory gain", 0, 32, 1, 0.5f);
    add("balance", "JO-B to JO-A balance", 0, 1, 0.5f);
    add("speed", "Simulation time-scale", 0.25f, 4, 1, 0.5f);
    add("centre", "JO band centre Hz", 60, 8000, 300, 0.3f);
    add("bands", "JO band separation octaves", 0.5f, 3, 1.5f);
    add("wet", "Wet / dry", 0, 1, 1);
    add("depth", "Neural influence", 0, 1, 1);
    add("brightness", "Spike brightness", 0.2f, 3, 1.5f);
    add("decay", "Spike decay seconds", 0.03f, 1, 0.18f);
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Neural output mode",
               juce::StringArray{"Auditory filter (clean)", "Auditory gating / compression", "Auditory waveshaping", "Artistic circuit blend"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"colour", 1}, "Neuron colour",
               juce::StringArray{"Population", "Predicted neurotransmitter", "Recent spiking activity"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"connections", 1}, "Show connections", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"spatial", 1}, "FAFB anchor coordinates", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"disableVisualization", 1}, "Disable visualisation", false));
    for (std::size_t p = 0; p < fly::populationCount; ++p)
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"show" + juce::String(static_cast<int>(p)), 1}, fly::populationNames[p], true));
    return layout;
}
FlyAudioProcessor::FlyAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "FlyAuditoryState", parameterLayout()) {
    centre = parameters.getRawParameterValue("centre"); bands = parameters.getRawParameterValue("bands");
    wet = parameters.getRawParameterValue("wet"); depth = parameters.getRawParameterValue("depth"); mode = parameters.getRawParameterValue("mode");
    try {
        auditoryGraph = fly::loadGraph(BinaryData::auditory_flygraph, static_cast<std::size_t>(BinaryData::auditory_flygraphSize));
        worker = std::make_unique<Worker>(auditoryGraph, transport, parameters);
    } catch (const std::exception& e) { graphError = e.what(); }
}
FlyAudioProcessor::~FlyAudioProcessor() { stopWorker(); }
FlyAudioProcessor::Worker::Worker(const fly::Graph& graph, fly::SharedState& state, juce::AudioProcessorValueTreeState& params)
    : juce::Thread("Fly auditory LIF"), shared(state), engine(graph, state),
      gain(params.getRawParameterValue("sensoryGain")), balance(params.getRawParameterValue("balance")), speed(params.getRawParameterValue("speed")) {}
void FlyAudioProcessor::Worker::run() {
    while (!threadShouldExit()) {
        const auto start = std::chrono::steady_clock::now();
        fly::FeatureFrame frame; double audioSeconds = 0; int drained = 0;
        while (drained < 16 && !threadShouldExit() && shared.features.pop(frame)) {
            engine.advance(frame, {gain->load(std::memory_order_relaxed), balance->load(std::memory_order_relaxed), speed->load(std::memory_order_relaxed)});
            audioSeconds += frame.seconds; ++drained;
        }
        if (audioSeconds > 0) {
            const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            shared.telemetry.workerLoad.store(static_cast<float>(elapsed / audioSeconds), std::memory_order_relaxed);
        } else wait(1); // no audio-thread notification or OS calls
    }
}
void FlyAudioProcessor::stopWorker() {
    transport.prepared.store(false, std::memory_order_release);
    if (worker && worker->isThreadRunning()) { worker->signalThreadShouldExit(); worker->notify(); worker->stopThread(-1); }
}
void FlyAudioProcessor::prepareToPlay(double sampleRate, int) {
    stopWorker(); fly::FeatureFrame stale;
    while (transport.features.pop(stale)) {}
    features.prepare(sampleRate); effects.prepare(sampleRate);
    transport.telemetry.input.store(0, std::memory_order_relaxed);
    for (auto& rate : transport.telemetry.audioRates) rate.store(0, std::memory_order_relaxed);
    if (worker) { worker->reset(); worker->startThread(juce::Thread::Priority::high); transport.prepared.store(true, std::memory_order_release); }
}
void FlyAudioProcessor::releaseResources() { stopWorker(); }
bool FlyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto channels = layout.getMainInputChannelSet();
    return (channels == juce::AudioChannelSet::mono() || channels == juce::AudioChannelSet::stereo()) && channels == layout.getMainOutputChannelSet();
}
void FlyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals guard;
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, buffer.getNumSamples());
    if (!transport.prepared.load(std::memory_order_acquire)) return; // invalid graph: explicit dry bypass, never fake data
    const int channels = juce::jmin(2, getTotalNumInputChannels());
    if (channels == 0) return;
    features.setBands(centre->load(std::memory_order_relaxed), bands->load(std::memory_order_relaxed));
    std::array<float, fly::populationCount> rates {};
    for (std::size_t p = 0; p < rates.size(); ++p) {
        rates[p] = transport.telemetry.rates[p].load(std::memory_order_relaxed);
        transport.telemetry.audioRates[p].store(rates[p], std::memory_order_relaxed);
    }
    effects.setTargets(rates, {wet->load(std::memory_order_relaxed), depth->load(std::memory_order_relaxed), static_cast<int>(mode->load(std::memory_order_relaxed))});
    auto* left = buffer.getWritePointer(0); auto* right = channels == 2 ? buffer.getWritePointer(1) : nullptr;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const float l = std::isfinite(left[i]) ? left[i] : 0;
        const float r = right && std::isfinite(right[i]) ? right[i] : 0;
        fly::FeatureFrame frame;
        if (features.consume(l, r, channels, frame)) {
            transport.telemetry.input.store(frame.envelope, std::memory_order_relaxed);
            if (!transport.features.push(frame)) transport.telemetry.droppedFeatures.fetch_add(1, std::memory_order_relaxed);
        }
        effects.nextSample(); left[i] = effects.process(l, 0); if (right) right[i] = effects.process(r, 1);
    }
}
void FlyAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void FlyAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* FlyAudioProcessor::createEditor() { return new FlyAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FlyAudioProcessor(); }
