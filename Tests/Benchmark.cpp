#include "AuditoryPathwaySimulator.h"
#include "AudioMapping.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

struct Digest {
    std::uint64_t value = 14695981039346656037ULL;
    template<class T> void add(const T& x) {
        const auto* bytes = reinterpret_cast<const unsigned char*>(&x);
        for (std::size_t i = 0; i < sizeof(T); ++i) { value ^= bytes[i]; value *= 1099511628211ULL; }
    }
};
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("Pass graph path");
        const auto graph = fly::loadGraphFile(argv[1]);
        std::cout << std::setprecision(9);
        for (int scenario = 0; scenario < 3; ++scenario) {
            std::vector<double> times;
            std::uint64_t expected = 0, spikes = 0;
            for (int repeat = 0; repeat < 9; ++repeat) {
                auto shared = std::make_unique<fly::SharedState>();
                fly::AuditoryPathwaySimulator sim(graph, *shared);
                shared->visualActive.store(scenario == 1);
                Digest digest;
                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < 2000; ++i) {
                    const fly::FeatureFrame frame = scenario == 1 ? fly::FeatureFrame{8,8,8,8,0.001f}
                        : fly::FeatureFrame{0.2f,0.2f,0.3f,0.02f,0.001f};
                    const fly::DriveSettings settings = scenario == 0 ? fly::DriveSettings{}
                        : scenario == 1 ? fly::DriveSettings{32,0.5f,4}
                        : fly::DriveSettings{static_cast<float>(i % 11), static_cast<float>(i % 10) / 10, i % 2 ? 0.25f : 4};
                    sim.advance(frame, settings);
                    // Include intermediate trajectories, not only final spike totals.
                    if (i % 100 == 0) { for (auto v : sim.voltages()) digest.add(v); for (auto r : sim.rates()) digest.add(r); }
                }
                times.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
                for (auto v : sim.voltages()) digest.add(v);
                for (auto r : sim.rates()) digest.add(r);
                spikes = sim.totalSpikes(); digest.add(spikes);
                if (repeat && expected != digest.value) throw std::runtime_error("Neural trajectory is not repeatable");
                expected = digest.value;
            }
            std::sort(times.begin(), times.end());
            std::cout << "lif" << scenario << " " << times[4] << " " << expected << " " << spikes << '\n';
        }
        for (const auto sr : {48000, 96000}) for (int mode = 0; mode < 5; ++mode) {
            std::vector<double> times; std::uint64_t expected = 0;
            for (int repeat = 0; repeat < 9; ++repeat) {
                fly::NeuralAudioDSP dsp; dsp.prepare(sr);
                fly::AudioFeatures features; features.prepare(sr);
                Digest digest; fly::FeatureFrame frame;
                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < sr * 2; ++i) {
                    if (i % 128 == 0) {
                        std::array<float, fly::populationCount> rates {};
                        for (std::size_t p = 0; p < rates.size(); ++p) rates[p] = static_cast<float>((i / 128 + p * 13) % 201);
                        dsp.setTargets(rates, {1,1,mode == 4 ? (i / 128) % 4 : mode});
                        features.setBands((i / 4096) % 2 ? 300.0f : 600.0f, 1.5f);
                    }
                    const auto x = 0.4f * std::sin(6.283185307f * 220 * static_cast<float>(i) / static_cast<float>(sr));
                    if (features.consume(x, -x, 2, frame)) { digest.add(frame.low); digest.add(frame.high); digest.add(frame.envelope); digest.add(frame.onset); }
                    dsp.nextSample(); const auto l = dsp.process(x,0), r = dsp.process(-x,1);
                    if (!std::isfinite(l) || !std::isfinite(r)) throw std::runtime_error("Nonfinite benchmark output");
                    digest.add(l); digest.add(r);
                }
                times.push_back(std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
                if (repeat && expected != digest.value) throw std::runtime_error("DSP is not repeatable");
                expected = digest.value;
            }
            std::sort(times.begin(), times.end());
            std::cout << "audio" << sr << "mode" << mode << " " << times[4] << " " << expected << " 0\n";
        }
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
