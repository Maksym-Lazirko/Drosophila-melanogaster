#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "AuditoryPathwaySimulator.h"
#include "AudioMapping.h"

class FlyAudioProcessor final : public juce::AudioProcessor {
public:
    FlyAudioProcessor();
    ~FlyAudioProcessor() override;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Drosophila melanogaster"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    const fly::Graph& graph() const noexcept { return auditoryGraph; }
    fly::SharedState& shared() noexcept { return transport; }
    const juce::String& dataError() const noexcept { return graphError; }
    juce::AudioProcessorValueTreeState parameters;
    static juce::AudioProcessorValueTreeState::ParameterLayout parameterLayout();
private:
    class Worker final : public juce::Thread {
    public:
        Worker(const fly::Graph&, fly::SharedState&, juce::AudioProcessorValueTreeState&);
        void run() override;
        void reset() { engine.reset(); }
    private:
        fly::SharedState& shared;
        fly::AuditoryPathwaySimulator engine;
        std::atomic<float> *gain, *balance, *speed;
    };
    void stopWorker();
    fly::Graph auditoryGraph;
    fly::SharedState transport;
    juce::String graphError;
    std::unique_ptr<Worker> worker;
    fly::AudioFeatures features;
    fly::NeuralAudioDSP effects;
    std::atomic<float> *centre, *bands, *wet, *depth, *mode;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlyAudioProcessor)
};
