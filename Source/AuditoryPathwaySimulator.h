#pragma once
#include "RealtimeTransport.h"
#include <vector>

namespace fly {
struct DriveSettings { float gain = 1.0f, balance = 0.5f, speed = 1.0f; };
class AuditoryPathwaySimulator {
public:
    static constexpr float stepSeconds = 0.0002f;
    explicit AuditoryPathwaySimulator(const Graph&, SharedState&);
    void reset() noexcept;
    void advance(const FeatureFrame&, const DriveSettings&) noexcept;
    const std::array<float, populationCount>& rates() const noexcept { return populationRates; }
    std::uint64_t totalSpikes() const noexcept { return spikeCount; }
    std::uint64_t downstreamSpikes() const noexcept { return centralSpikes; }
    const std::vector<float>& voltages() const noexcept { return voltage; }
private:
    void step(float aProbability, float bProbability) noexcept;
    float uniform() noexcept;
    const Graph& graph; SharedState& shared;
    std::vector<float> voltage, conductance, rate, delayed;
    std::vector<std::uint8_t> refractory;
    std::array<float, populationCount> populationRates {};
    std::uint32_t randomState = 0x4f6c7931, tick = 0;
    std::uint64_t spikeCount = 0, centralSpikes = 0;
    double remainder = 0;
    static constexpr std::size_t delaySlots = 10; // 1.8 ms = nine 0.2 ms ticks
};
}
