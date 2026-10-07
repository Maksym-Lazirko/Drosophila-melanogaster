#include "PluginProcessor.h"
#include "EditorChecks.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <cstdlib>
#include <new>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

// Test-only replacement operators count C++ heap activity on the callback
// thread, not allocations from the independently running simulation worker.
namespace {
thread_local bool auditCallbackHeap = false;
thread_local std::uint64_t callbackAllocations = 0, callbackDeallocations = 0;
void recordAllocation() noexcept { if (auditCallbackHeap) ++callbackAllocations; }
void recordDeallocation(void* p) noexcept { if (auditCallbackHeap && p) ++callbackDeallocations; }
}
void* operator new(std::size_t size) {
    recordAllocation(); if (auto* p = std::malloc(size ? size : 1)) return p; throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { recordDeallocation(p); std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { ::operator delete(p); }
void* operator new(std::size_t size, std::align_val_t alignment) {
    recordAllocation(); const auto a = static_cast<std::size_t>(alignment);
#if defined(_MSC_VER)
    auto* p = _aligned_malloc(size ? size : 1, a);
#else
    const auto rounded = ((std::max<std::size_t>(size, 1) + a - 1) / a) * a;
    auto* p = std::aligned_alloc(a, rounded);
#endif
    if (p) return p; throw std::bad_alloc();
}
void* operator new[](std::size_t size, std::align_val_t alignment) { return ::operator new(size, alignment); }
void operator delete(void* p, std::align_val_t) noexcept {
    recordDeallocation(p);
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p, std::align_val_t a) noexcept { ::operator delete(p, a); }
void operator delete(void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete(p, a); }
void operator delete[](void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete(p, a); }

static void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
static void parameter(FlyAudioProcessor& p, const char* id, float value) {
    auto* v = p.parameters.getParameter(id); v->setValueNotifyingHost(v->convertTo0to1(value));
}
int main(int argc, char** argv) {
    juce::ScopedJuceInitialiser_GUI gui;
    try {
        auditCallbackHeap = true;
        auto* probe = ::operator new(17); ::operator delete(probe);
        auto* alignedProbe = ::operator new(17, std::align_val_t{64}); ::operator delete(alignedProbe, std::align_val_t{64});
        auditCallbackHeap = false;
        require(callbackAllocations == 2 && callbackDeallocations == 2, "Heap audit did not intercept replacement operators");
        callbackAllocations = callbackDeallocations = 0;
        auto p = std::make_unique<FlyAudioProcessor>(); require(p->dataError().isEmpty(), "Embedded graph load failed");
        require(p->graph().neurons.size() == 731, "Embedded graph incorrect");
        require(p->getName() == "Drosophila melanogaster", "Species product name incorrect");
        const std::pair<const char*, float> factoryDefaults[] {
            {"sensoryGain", 1.0f}, {"balance", 0.5f}, {"speed", 1.0f}, {"centre", 300.0f}, {"bands", 1.5f},
            {"wet", 1.0f}, {"depth", 1.0f}, {"brightness", 1.5f}, {"decay", 0.18f}, {"mode", 0.0f}, {"colour", 0.0f},
            {"connections", 1.0f}, {"spatial", 0.0f}, {"disableVisualization", 0.0f}, {"delta", 0.0f}
        };
        for (const auto& entry : factoryDefaults) {
            const auto* value = p->parameters.getParameter(entry.first);
            require(value != nullptr, "Factory parameter missing");
            require(std::abs(p->parameters.getRawParameterValue(entry.first)->load() - entry.second) <= 0.0001f * std::max(1.0f, entry.second), "Factory model baseline differs from documented value");
            require(std::abs(value->convertFrom0to1(value->getDefaultValue()) - entry.second) <= 0.0001f * std::max(1.0f, entry.second), "Host factory default differs from initial state");
        }
        for (std::size_t i = 0; i < fly::populationCount; ++i)
            require(p->parameters.getRawParameterValue("show" + juce::String(static_cast<int>(i)))->load() == 1, "Factory population visibility not enabled");
        require(p->parameters.getParameter("depth")->getNormalisableRange().end == 4, "Artistic influence range missing");
        parameter(*p, "depth", 4);
        juce::MemoryBlock artisticState; p->getStateInformation(artisticState); parameter(*p, "depth", 1);
        p->setStateInformation(artisticState.getData(), static_cast<int>(artisticState.getSize()));
        require(p->parameters.getRawParameterValue("depth")->load() == 4, "Artistic influence state did not recall");
        parameter(*p, "depth", 1);
        std::cout << "PASS: every audio/visual/host factory default, baseline influence 1 and artistic influence 4 recall\n";
        parameter(*p, "delta", 1);
        parameter(*p, "disableVisualization", 1);
        parameter(*p, "wet", 0); parameter(*p, "balance", 0.7f);
        juce::MemoryBlock state; p->getStateInformation(state); parameter(*p, "balance", 0.1f); parameter(*p, "delta", 0);
        p->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        require(std::abs(p->parameters.getRawParameterValue("balance")->load() - 0.7f) < 0.0001f, "Host state did not round-trip");
        require(p->parameters.getRawParameterValue("disableVisualization")->load() == 1, "Visualisation preference not recalled");
        require(p->parameters.getRawParameterValue("delta")->load() == 1, "Delta preference not recalled");
        auto legacy = p->parameters.copyState();
        for (int i = legacy.getNumChildren(); --i >= 0;)
            if (legacy.getChild(i).getProperty("id").toString() == "delta") legacy.removeChild(i, nullptr);
        juce::MemoryBlock oldState;
        if (auto xml = legacy.createXml()) juce::AudioProcessor::copyXmlToBinary(*xml, oldState);
        p->setStateInformation(oldState.getData(), static_cast<int>(oldState.getSize()));
        require(p->parameters.getRawParameterValue("delta")->load() == 0, "Legacy preset retained Delta monitoring");
        parameter(*p, "disableVisualization", 0);
        // The actual VST3 test covers untouched defaults. This repeated short
        // lifecycle/mode stress uses stronger drive so downstream activity is
        // established even in the shortest 64-sample run (~0.4 audio seconds).
        parameter(*p, "sensoryGain", 4);
        for (const auto channelCount : {1, 2}) for (const auto blockSize : {64, 128, 256, 512}) {
            juce::AudioProcessor::BusesLayout layout;
            layout.inputBuses.add(channelCount == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo()); layout.outputBuses = layout.inputBuses;
            require(p->setBusesLayout(layout), "Unsupported mono/stereo layout");
            p->prepareToPlay(48000, blockSize); juce::AudioBuffer<float> b(channelCount, blockSize); juce::MidiBuffer midi;
            fly::NeuralAudioDSP reference; reference.prepare(48000);
            const auto checkResidual = [&](int mode, float wet, int k, bool delta, bool settled) {
                std::array<float, fly::populationCount> rates {};
                for (std::size_t i = 0; i < rates.size(); ++i) rates[i] = p->shared().telemetry.audioRates[i].load();
                reference.setTargets(rates, {wet, 1, mode});
                for (int i = 0; i < blockSize; ++i) {
                    reference.nextSample();
                    for (int c = 0; c < channelCount; ++c) {
                        const float x = (wet == 0 ? 0.25f : 0.3f) * std::sin(6.2831853f * 220.0f * static_cast<float>(k * blockSize + i) / 48000.0f)
                            * (wet == 0 && c == 1 ? -1.0f : 1.0f);
                        const auto expected = reference.process(x, c) - (delta ? x : 0.0f);
                        // The reference uses the exact callback control-rate snapshot;
                        // ignore only the documented monitor ramp, not neural response.
                        if (settled)
                            require(std::abs(b.getSample(c, i) - expected) < 0.000001f, "Processor Delta residual differs from normal minus dry");
                    }
                }
            };
            double elapsed = 0; constexpr int blocks = 300;
            for (int k = 0; k < blocks; ++k) {
                for (int c = 0; c < channelCount; ++c) for (int i = 0; i < blockSize; ++i)
                    b.setSample(c, i, 0.25f * std::sin(6.2831853f * 220.0f * static_cast<float>(k * blockSize + i) / 48000.0f) * (c == 1 ? -1.0f : 1.0f));
                const auto expected = b.getSample(0, 10); const auto start = std::chrono::steady_clock::now();
                auditCallbackHeap = true; p->processBlock(b, midi); auditCallbackHeap = false;
                elapsed += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                checkResidual(0, 0, k, false, true);
                require(b.getSample(0, 10) == expected, "Dry host callback changed samples");
                for (int c = 0; c < channelCount; ++c) for (int i = 0; i < blockSize; ++i) require(std::isfinite(b.getSample(c, i)), "Nonfinite audio output");
                std::this_thread::sleep_for(std::chrono::duration<double>(blockSize / 48000.0));
            }
            require(p->shared().telemetry.rates[0].load() + p->shared().telemetry.rates[1].load() > 0, "Threaded JO simulation not running");
            require(p->shared().telemetry.rates[4].load() + p->shared().telemetry.rates[6].load() > 0, "No downstream neural response");
            parameter(*p, "wet", 1); parameter(*p, "depth", 1);
            for (int mode = 0; mode < 4; ++mode) {
                parameter(*p, "mode", static_cast<float>(mode)); bool changed = false, residual = false;
                parameter(*p, "delta", 0);
                for (int k = 0; k < 100; ++k) {
                    if (k == 25) {
                        parameter(*p, "brightness", mode % 2 ? 0.2f : 3.0f);
                        parameter(*p, "decay", mode % 2 ? 0.03f : 1.0f);
                        parameter(*p, "colour", static_cast<float>(mode % 3));
                        parameter(*p, "connections", static_cast<float>(mode % 2));
                        parameter(*p, "spatial", static_cast<float>(mode % 2));
                        parameter(*p, "disableVisualization", static_cast<float>(mode % 2));
                        for (std::size_t i = 0; i < fly::populationCount; ++i)
                            parameter(*p, ("show" + juce::String(static_cast<int>(i))).toRawUTF8(), static_cast<float>(mode % 2));
                    }
                    if (k == 50) parameter(*p, "delta", 1);
                    for (int c = 0; c < channelCount; ++c) for (int i = 0; i < blockSize; ++i)
                        b.setSample(c, i, 0.3f * std::sin(6.2831853f * 220.0f * static_cast<float>((k + blocks) * blockSize + i) / 48000.0f));
                    const auto before = b.getSample(0, blockSize / 2);
                    auditCallbackHeap = true; p->processBlock(b, midi); auditCallbackHeap = false;
                    checkResidual(mode, 1, k + blocks, k >= 50, (k >= 5 && k < 50) || k >= 55);
                    for (int i = 0; i < blockSize; ++i) require(std::isfinite(b.getSample(0, i)), "Effect mode unstable");
                    if (k >= 5 && k < 50) changed |= std::abs(b.getSample(0, blockSize / 2) - before) > 0.00001f;
                    if (k >= 55) residual |= std::abs(b.getSample(0, blockSize / 2)) > 0.00001f;
                    std::this_thread::sleep_for(std::chrono::duration<double>(blockSize / 48000.0));
                }
                require(changed, "Neural effect leaves normal audio unchanged");
                require(residual, "Delta has no residual in an audio mode");
            }
            parameter(*p, "wet", 0); parameter(*p, "delta", 0); p->releaseResources();
            std::cout << channelCount << "ch / " << blockSize << " samples: callback average " << elapsed / blocks * 1e6 << " us, neural load " << p->shared().telemetry.workerLoad.load() * 100 << "%\n";
        }
        require(p->shared().telemetry.droppedFeatures.load() == 0, "Feature queue overflow in paced host test");
        require(callbackAllocations == 0 && callbackDeallocations == 0, "C++ heap allocation or deallocation occurred in processBlock");
        std::cout << "PASS: callback C++ heap audit, " << callbackAllocations << " allocations / " << callbackDeallocations << " deallocations across 5600 blocks\n";
        std::cout << "PASS: changing all visual-only controls leaves audio equal to the independent reference\n";
        for (const auto& entry : factoryDefaults)
            if (juce::String(entry.first) == "brightness" || juce::String(entry.first) == "decay" || juce::String(entry.first) == "colour"
                || juce::String(entry.first) == "connections" || juce::String(entry.first) == "spatial" || juce::String(entry.first) == "disableVisualization")
                parameter(*p, entry.first, entry.second);
        for (std::size_t i = 0; i < fly::populationCount; ++i) parameter(*p, ("show" + juce::String(static_cast<int>(i))).toRawUTF8(), 1);
        if (argc > 1 && juce::String(argv[1]) == "--editor") {
            parameter(*p, "mode", 0); parameter(*p, "wet", 1); parameter(*p, "depth", 1);
            p->prepareToPlay(48000, 128);
            auto editor = std::unique_ptr<juce::AudioProcessorEditor>(p->createEditor()); editor->addToDesktop(juce::ComponentPeer::windowIsTemporary); editor->setVisible(true);
            EditorChecks editorChecks(*editor, p->shared(), p->parameters); editorChecks.schedule();
            std::atomic<bool> quit {false}, disabledAudioFinite {true}, disabledAudioChanged {false};
            std::atomic<std::uint32_t> disabledAudioSamples {0};
            std::thread audio([&] { juce::AudioBuffer<float> b(2, 128); juce::MidiBuffer midi; int offset = 0;
                while (!quit.load()) { for (int i = 0; i < 128; ++i) { const auto value = 0.3f * std::sin(6.2831853f * 220 * static_cast<float>(offset++) / 48000); b.setSample(0, i, value); b.setSample(1, i, value); } const auto before = b.getSample(0, 64); p->processBlock(b, midi);
                    if (!p->shared().visualizationEnabled.load()) {
                        disabledAudioSamples.fetch_add(128);
                        for (int i = 0; i < 128; ++i) if (!std::isfinite(b.getSample(0, i))) disabledAudioFinite.store(false);
                        if (std::abs(b.getSample(0, 64) - before) > 0.00001f) disabledAudioChanged.store(true);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(3)); } });
            std::thread stopLoop([] { std::this_thread::sleep_for(std::chrono::seconds(10)); juce::MessageManager::getInstance()->stopDispatchLoop(); });
            juce::MessageManager::getInstance()->runDispatchLoop(); stopLoop.join(); quit.store(true); audio.join();
            require(editorChecks.deltaPassed.load(), "Live Delta button/bounds/attachment checks failed");
            require(editorChecks.glPassed.load() && editorChecks.activityPassed.load() && editorChecks.phase.load()==4 && editorChecks.disablePassed.load() && editorChecks.reenablePassed.load(), "Live editor rendering/activity/control checks failed");
            require(disabledAudioSamples.load() > 1000 && disabledAudioFinite.load() && disabledAudioChanged.load(),
                    "Disabling visualisation stopped or corrupted neural audio processing");
            std::cout << "DISABLED VISUAL AUDIO: " << disabledAudioSamples.load() << " finite samples; neural effect remains active\n";
            editor.reset(); p->releaseResources();
        }
        std::cout << "PASS: embedded data, APVTS, threaded simulation, dry bypass, Delta residual/state/legacy recall and all neural effect modes\n"; return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
