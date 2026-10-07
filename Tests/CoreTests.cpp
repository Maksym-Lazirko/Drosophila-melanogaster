#include "AuditoryPathwaySimulator.h"
#include "AudioMapping.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>

void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

// Repeatable, same-graph control audit. This deliberately advances the worker
// model sequentially so scheduling noise cannot masquerade as a knob response.
struct ControlSettings {
    fly::DriveSettings drive;
    float centre = 300, bands = 1.5f;
    fly::EffectSettings effect;
};
static std::vector<float> renderControls(const fly::Graph& graph, int sampleRate, ControlSettings settings, bool visuals = false) {
    auto shared = std::make_unique<fly::SharedState>();
    shared->visualActive.store(visuals); shared->visualizationEnabled.store(visuals);
    fly::AuditoryPathwaySimulator sim(graph, *shared);
    fly::AudioFeatures features; features.prepare(sampleRate); features.setBands(settings.centre, settings.bands);
    fly::NeuralAudioDSP dsp; dsp.prepare(sampleRate);
    std::vector<float> output; output.reserve(static_cast<std::size_t>(sampleRate));
    for (int i = 0; i < sampleRate * 2; ++i) {
        if (i % 128 == 0) dsp.setTargets(sim.rates(), settings.effect);
        const auto t = static_cast<double>(i) / sampleRate;
        const auto envelope = std::fmod(t, 0.19) < 0.055 ? 1.0 : 0.35;
        const auto x = static_cast<float>(envelope * (0.28 * std::sin(6.283185307 * 150 * t)
            + 0.22 * std::sin(6.283185307 * 310 * t) + 0.13 * std::sin(6.283185307 * 1100 * t)
            + 0.08 * std::sin(6.283185307 * 3600 * t)));
        fly::FeatureFrame frame;
        if (features.consume(x, -x, 2, frame)) sim.advance(frame, settings.drive);
        dsp.nextSample(); const auto left = dsp.process(x, 0), right = dsp.process(-x, 1);
        require(std::isfinite(left) && std::isfinite(right), "Nonfinite control sweep output");
        require(std::abs(left) < 16 && std::abs(right) < 16, "Unbounded artistic control output");
        if (i >= sampleRate) output.push_back(left);
    }
    require(shared->telemetry.cappedTicks.load() == 0, "Control sweep dropped supported simulation ticks");
    return output;
}
static double differenceRms(const std::vector<float>& a, const std::vector<float>& b) {
    require(a.size() == b.size(), "Control recordings have unequal lengths");
    double energy = 0;
    for (std::size_t i = 0; i < a.size(); ++i) { const auto d = static_cast<double>(a[i]) - b[i]; energy += d * d; }
    return std::sqrt(energy / static_cast<double>(a.size()));
}
static void controlAudit(const fly::Graph& graph) {
    for (const auto sampleRate : {48000, 96000}) {
        const auto baseline = renderControls(graph, sampleRate, {});
        require(baseline == renderControls(graph, sampleRate, {}), "Control baseline is not repeatable");
        require(baseline == renderControls(graph, sampleRate, {}, true), "Visual event production changed audio samples");
        const auto sweep = [&](const char* id, ControlSettings low, ControlSettings high) {
            const auto a = renderControls(graph, sampleRate, low), b = renderControls(graph, sampleRate, high);
            const auto difference = differenceRms(a, b);
            require(difference > 0.000001, "Audio control endpoints have no measurable sound difference");
            float peak = 0;
            for (const auto* recording : {&a, &b}) for (auto x : *recording) peak = std::max(peak, std::abs(x));
            std::cout << "CONTROL " << sampleRate << ' ' << id << " endpoint_difference_rms=" << difference
                << " baseline_to_low_rms=" << differenceRms(baseline, a) << " baseline_to_high_rms=" << differenceRms(baseline, b)
                << " peak=" << peak << '\n';
        };
        ControlSettings low, high;
        low.drive.gain = 0; high.drive.gain = 32; sweep("sensoryGain", low, high);
        low = {}; high = {}; low.drive.balance = 0; high.drive.balance = 1; sweep("balance", low, high);
        low = {}; high = {}; low.drive.speed = 0.25f; high.drive.speed = 4; sweep("speed", low, high);
        low = {}; high = {}; low.centre = 60; high.centre = 8000; sweep("centre", low, high);
        low = {}; high = {}; low.bands = 0.5f; high.bands = 3; sweep("bands", low, high);
        low = {}; high = {}; low.effect.wet = 0; high.effect.wet = 1; sweep("wet", low, high);
        low = {}; high = {}; low.effect.depth = 0; high.effect.depth = 1; sweep("depth_baseline", low, high);
        low = {}; high = {}; high.effect.depth = 4; sweep("depth_artistic", low, high);
        low = {}; high = {}; high.effect.delta = true; sweep("delta", low, high);
        std::array<std::vector<float>, 4> modes;
        for (int mode = 0; mode < 4; ++mode) { ControlSettings s; s.effect.mode = mode; modes[static_cast<std::size_t>(mode)] = renderControls(graph, sampleRate, s); }
        for (int a = 0; a < 4; ++a) for (int b = a + 1; b < 4; ++b) {
            const auto difference = differenceRms(modes[static_cast<std::size_t>(a)], modes[static_cast<std::size_t>(b)]);
            require(difference > 0.000001, "Two audio modes sound numerically identical");
            std::cout << "MODE " << sampleRate << ' ' << a << '-' << b << " difference_rms=" << difference << '\n';
        }
        // Push only the readout: each mapping must react measurably to the
        // same actual weak-rate baseline, without fabricated downstream drive.
        for (int mode = 0; mode < 4; ++mode) {
            ControlSettings s; s.effect.mode = mode;
            const auto normal = renderControls(graph, sampleRate, s);
            s.effect.depth = 4; const auto extreme = renderControls(graph, sampleRate, s);
            const auto difference = differenceRms(normal, extreme);
            require(difference > 0.0001, "Artistic readout boost ineffective in an audio mode");
            std::cout << "ARTISTIC " << sampleRate << " mode=" << mode << " baseline_to_influence4_rms=" << difference << '\n';
            // Full artistic drive: check both channels/rates at all existing
            // parameter endpoints, not only monotonicity (not promised by LIF).
            s.drive.gain = 32; s.drive.speed = 4;
            for (const auto centre : {60.0f, 8000.0f}) for (const auto balance : {0.0f, 1.0f}) {
                s.centre = centre; s.drive.balance = balance;
                renderControls(graph, sampleRate, s);
            }
        }
        std::cout << "PASS: repeatable baseline, each audio control, all mode pairs and visual/audio independence at " << sampleRate << " Hz\n";
    }
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Pass graph path"); const auto graph = fly::loadGraphFile(argv[1]);
        require(graph.neurons.size() == 731 && graph.synapses.size() == 7056, "Unexpected shipped graph");
        controlAudit(graph);
        require(graph.neurons[0].rootId > 720000000000000000ULL, "Root ID precision lost");
        std::ifstream input(argv[1], std::ios::binary); const std::vector<char> bytes((std::istreambuf_iterator<char>(input)), {});
        for (const auto length : {std::size_t(0), std::size_t(19), bytes.size() - 1}) {
            bool rejected = false; try { fly::loadGraph(bytes.data(), length); } catch (const std::exception&) { rejected = true; }
            require(rejected, "Truncated graph accepted");
        }
        auto corrupt = bytes; corrupt[8] = 0; bool rejected = false;
        try { fly::loadGraph(corrupt.data(), corrupt.size()); } catch (const std::exception&) { rejected = true; }
        require(rejected, "Wrong version accepted");
        fly::SpscQueue<std::uint32_t, 256> queue;
        std::thread producer([&] { for (std::uint32_t i = 0; i < 200000; ++i) while (!queue.push(i)) std::this_thread::yield(); });
        for (std::uint32_t i = 0; i < 200000; ++i) { std::uint32_t value; while (!queue.pop(value)) std::this_thread::yield(); require(value == i, "SPSC order corrupt"); } producer.join();
        auto shared = std::make_unique<fly::SharedState>(); fly::AuditoryPathwaySimulator sim(graph, *shared);
        fly::FeatureFrame silence; for (int i = 0; i < 1000; ++i) sim.advance(silence, {});
        require(sim.totalSpikes() == 0, "Spontaneous activity without sensory input");
        sim.reset();
        const fly::FeatureFrame tone {0.2f, 0.2f, 0.3f, 0.02f, 0.001f};
        const auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < 2000; ++i) sim.advance(tone, {});
        const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        require(sim.totalSpikes() > 1000, "JO input does not produce spiking"); require(sim.downstreamSpikes() > 0, "Real edges fail to drive central neurons");
        const auto count = sim.totalSpikes(); sim.reset(); for (int i = 0; i < 2000; ++i) sim.advance(tone, {});
        require(sim.totalSpikes() == count, "Seeded simulation not deterministic");
        sim.reset(); for (int i = 0; i < 1000; ++i) sim.advance(tone, {0, 0.5f, 1}); require(sim.totalSpikes() == 0, "Zero gain does not mute sensory input");
        sim.reset(); for (int i = 0; i < 1000; ++i) sim.advance(tone, {4, 1, 1}); require(sim.rates()[0] > sim.rates()[1], "Balance does not favor JO-A");
        for (const auto v : sim.voltages()) require(std::isfinite(v) && v >= -100 && v <= 100, "Unbounded membrane");
        // Worst supported drive/time-scale on the actual graph, including visual
        // event production. Deliberately saturate its queue to verify that lost
        // visual events cannot alter neural propagation or block the worker.
        const fly::FeatureFrame maximumDrive {8.0f, 8.0f, 8.0f, 8.0f, 0.001f};
        sim.reset(); shared->visualActive.store(true);
        const auto stressStart = std::chrono::steady_clock::now();
        for (int i = 0; i < 1000; ++i) sim.advance(maximumDrive, {32, 0.5f, 4});
        const auto stressSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - stressStart).count();
        const auto stressSpikes = sim.totalSpikes(), stressCentralSpikes = sim.downstreamSpikes();
        const auto stressVoltages = sim.voltages();
        require(stressSpikes > count && stressCentralSpikes > 0, "Maximum drive failed to propagate");
        require(shared->telemetry.droppedSpikes.load() > 0, "Visual overflow stress did not fill queue");
        require(shared->telemetry.cappedTicks.load() == 0, "Supported 4x time-scale dropped simulation ticks");
        for (const auto v : stressVoltages) require(std::isfinite(v) && v >= -100 && v <= 100, "Maximum-drive membrane unstable");
        sim.reset(); shared->visualizationEnabled.store(false);
        fly::SpikeEvent stale; while (shared->spikes.pop(stale)) {}
        const auto droppedBeforeDisable = shared->telemetry.droppedSpikes.load();
        for (int i = 0; i < 1000; ++i) sim.advance(maximumDrive, {32, 0.5f, 4});
        require(!shared->spikes.pop(stale) && shared->telemetry.droppedSpikes.load() == droppedBeforeDisable,
                "Disabled visualisation still produces spike events");
        require(sim.totalSpikes() == stressSpikes && sim.downstreamSpikes() == stressCentralSpikes && sim.voltages() == stressVoltages,
                "Visual queue overflow changed neural trajectory");
        std::cout << "PASS: maximum sensory gain / 4x speed, 1 audio second (4 simulated seconds) in " << stressSeconds
                  << " s (" << stressSeconds * 100 << "% one-core budget), " << stressSpikes
                  << " spikes; visual overflow does not change simulation\n";
        const auto spectral = [](float hz) {
            fly::AudioFeatures f; f.prepare(48000); f.setBands(300, 1.5f); fly::FeatureFrame out; double low = 0, high = 0;
            for (int i = 0; i < 48000; ++i) { const auto x = 0.5f * std::sin(6.2831853f * hz * static_cast<float>(i) / 48000.0f); if (f.consume(x, -x, 2, out) && i > 24000) { low += out.low; high += out.high; } }
            require(out.envelope > 0.2f, "Anti-phase stereo cancels features"); return low / high; };
        require(spectral(150) > spectral(700), "Bands not frequency selective");
        // Clean mode must respond to rates without gate/waveshaping: after
        // settling constant rates it is a linear filter, not a distortion.
        fly::NeuralAudioDSP half, full, silentControl;
        for (auto* dsp : {&half, &full, &silentControl}) dsp->prepare(48000);
        std::array<float, fly::populationCount> drivenRates {}; drivenRates[4] = 8; drivenRates[6] = 3;
        double neuralDifference = 0;
        for (int i = 0; i < 96000; ++i) {
            if (i % 128 == 0) {
                half.setTargets(drivenRates, {}); full.setTargets(drivenRates, {}); silentControl.setTargets({}, {});
            }
            half.nextSample(); full.nextSample(); silentControl.nextSample();
            const float x = 0.4f * std::sin(6.2831853f * 220 * static_cast<float>(i) / 48000);
            const auto h = half.process(x * 0.5f, 0), f = full.process(x, 0), s = silentControl.process(x, 0);
            require(std::abs(h * 2 - f) < 0.000001f, "Clean mode adds nonlinear distortion");
            if (i > 48000) { require(s == x, "Zero neural control changes the carrier"); neuralDifference += (f - s) * (f - s); }
        }
        require(neuralDifference > 0.0001, "Clean mapping is insensitive to low neural rates");
        std::cout << "PASS: clean filter is linear and driven by low downstream rates; no-control path transparent\n";
        // Identical DSP states must satisfy delta + dry == normal after the
        // monitor ramp, for both channels, every mapping, and wet/depth settings.
        for (const auto sampleRate : {48000, 96000}) for (int mode = 0; mode < 4; ++mode)
            for (const auto amounts : {std::array<float, 2>{0, 1}, std::array<float, 2>{1, 0}, std::array<float, 2>{0.35f, 0.6f}, std::array<float, 2>{1, 1}, std::array<float, 2>{1, 4}}) {
                fly::NeuralAudioDSP normal, delta;
                normal.prepare(sampleRate); delta.prepare(sampleRate);
                std::array<float, fly::populationCount> rates {}; rates.fill(40);
                double residualEnergy = 0;
                for (int i = 0; i < sampleRate / 4; ++i) {
                    if (i % 128 == 0) {
                        normal.setTargets(rates, {amounts[0], amounts[1], mode, false});
                        delta.setTargets(rates, {amounts[0], amounts[1], mode, true});
                    }
                    normal.nextSample(); delta.nextSample();
                    for (int c = 0; c < 2; ++c) {
                        const float x = 0.3f * std::sin(6.2831853f * 700 * static_cast<float>(i) / static_cast<float>(sampleRate)) * (c ? -1.0f : 1.0f);
                        const auto n = normal.process(x, c), d = delta.process(x, c);
                        require(std::isfinite(d), "Nonfinite Delta output");
                        if (i > sampleRate / 100) {
                            require(std::abs(d - (n - x)) < 0.000001f, "Delta is not normal output minus dry input");
                            if (amounts[0] == 0 || amounts[1] == 0) require(d == 0, "Delta with wet/depth zero is not silent");
                            residualEnergy += d * d;
                        }
                    }
                }
                if (amounts[0] > 0 && amounts[1] > 0) require(residualEnergy > 0.00001, "Delta missing in an audio mode");
                delta.setTargets({}, {0, 0, mode, false});
                for (int i = 0; i < sampleRate; ++i) { delta.nextSample(); delta.process(0, 0); delta.process(0, 1); }
                require(std::abs(delta.process(0.2f, 0) - 0.2f) < 0.000001f, "Delta disable did not restore normal output");
            }
        // With a transparent carrier the monitor transition is a bounded 5 ms
        // ramp, not a hard switch that creates a full-amplitude discontinuity.
        fly::NeuralAudioDSP monitor; monitor.prepare(48000); monitor.setTargets({}, {0, 1, 0, true});
        float previous = 0.25f;
        for (int i = 0; i < 500; ++i) {
            monitor.nextSample(); const auto current = monitor.process(0.25f, 0);
            require(std::abs(current - previous) <= 0.25f / 240 + 0.000001f, "Delta toggle is not crossfaded"); previous = current;
        }
        require(previous == 0, "Delta ramp never reaches exact silence");
        std::cout << "PASS: Delta = normal minus dry, stereo/all modes/48-96 kHz, wet-depth nulls and 5 ms toggle ramp\n";
        for (int mode = 0; mode < 4; ++mode) {
            fly::NeuralAudioDSP dsp; dsp.prepare(48000); std::array<float, fly::populationCount> rates {}; rates.fill(200);
            dsp.setTargets(rates, {0, 1, mode});
            for (int i = 0; i < 10000; ++i) { dsp.nextSample(); require(dsp.process(0.123f, 0) == 0.123f, "Wet=0 is not exact bypass"); }
            dsp.setTargets(rates, {1, 1, mode});
            for (int i = 0; i < 100000; ++i) { dsp.nextSample(); auto out = dsp.process(std::sin(static_cast<float>(i)), 0); require(std::isfinite(out) && std::abs(out) < 10, "Unstable DSP"); }
        }
        std::cout << "PASS: " << graph.neurons.size() << " neurons, " << graph.synapses.size() << " edges; 2 simulated seconds in " << seconds << " s (" << seconds / 2 * 100 << "% one-core budget), " << count << " spikes\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
