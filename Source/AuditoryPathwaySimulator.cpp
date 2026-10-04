#include "AuditoryPathwaySimulator.h"
#include <algorithm>
#include <cmath>

namespace fly {
AuditoryPathwaySimulator::AuditoryPathwaySimulator(const Graph& g, SharedState& s)
    : graph(g), shared(s), voltage(g.neurons.size()), conductance(g.neurons.size()),
      rate(g.neurons.size()), delayed(g.neurons.size() * delaySlots), refractory(g.neurons.size()) { reset(); }
void AuditoryPathwaySimulator::reset() noexcept {
    std::fill(voltage.begin(), voltage.end(), -52.0f); std::fill(conductance.begin(), conductance.end(), 0.0f);
    std::fill(rate.begin(), rate.end(), 0.0f); std::fill(delayed.begin(), delayed.end(), 0.0f);
    std::fill(refractory.begin(), refractory.end(), std::uint8_t{0}); populationRates.fill(0);
    remainder = 0; tick = 0; spikeCount = centralSpikes = 0; randomState = 0x4f6c7931;
    for (auto& r : shared.telemetry.rates) r.store(0, std::memory_order_relaxed);
}
float AuditoryPathwaySimulator::uniform() noexcept {
    randomState ^= randomState << 13; randomState ^= randomState >> 17; randomState ^= randomState << 5;
    return static_cast<float>(randomState >> 8) * (1.0f / 16777216.0f);
}
void AuditoryPathwaySimulator::advance(const FeatureFrame& input, const DriveSettings& settings) noexcept {
    remainder += std::clamp(static_cast<double>(input.seconds), 0.0, 0.02) * std::clamp(settings.speed, 0.25f, 4.0f);
    const auto requested = static_cast<int>(remainder / stepSeconds);
    const auto steps = std::min(requested, 400);
    if (requested > steps) shared.telemetry.cappedTicks.fetch_add(static_cast<std::uint32_t>(requested - steps), std::memory_order_relaxed);
    remainder -= requested * static_cast<double>(stepSeconds);
    // Input/settings are constant across this feature frame. Hoist sensory
    // exponentials without changing RNG draws, tick order or LIF arithmetic.
    const float gain = std::clamp(settings.gain, 0.0f, 32.0f);
    const float balance = std::clamp(settings.balance, 0.0f, 1.0f);
    // Sensory mapping is an uncalibrated model assumption.
    const float aDrive = gain * (input.high + 0.12f * input.envelope + 0.20f * input.onset) * (2.0f * balance);
    const float bDrive = gain * (input.low + 0.12f * input.envelope + 0.25f * input.onset) * (2.0f * (1.0f - balance));
    const float aProbability = 1.0f - std::exp(-std::clamp(aDrive * 180.0f, 0.0f, 350.0f) * stepSeconds);
    const float bProbability = 1.0f - std::exp(-std::clamp(bDrive * 180.0f, 0.0f, 350.0f) * stepSeconds);
    for (int i = 0; i < steps; ++i) step(aProbability, bProbability);
    for (std::size_t p = 0; p < populationCount; ++p) shared.telemetry.rates[p].store(populationRates[p], std::memory_order_relaxed);
}
void AuditoryPathwaySimulator::step(float aProbability, float bProbability) noexcept {
    const auto n = voltage.size();
    const auto slot = (tick % delaySlots) * n;
    const auto nextSlot = ((tick + 9) % delaySlots) * n;
    // Exponential integration constants for 20 ms membrane, 5 ms synaptic decay.
    constexpr float membraneDecay = 0.99004983375f, synDecay = 0.96078943915f;
    constexpr float rateDecay = 0.99600798934f; // 50 ms EWMA in simulated time
    std::array<float, populationCount> sum {};
    const bool emitVisual = shared.visualActive.load(std::memory_order_relaxed)
                         && shared.visualizationEnabled.load(std::memory_order_relaxed);
    for (std::size_t i = 0; i < n; ++i) {
        const auto p = graph.neurons[i].population;
        conductance[i] = std::clamp(conductance[i] * synDecay + delayed[slot + i], -5000.0f, 5000.0f);
        delayed[slot + i] = 0;
        rate[i] *= rateDecay;
        bool spike = false;
        if (refractory[i] > 0) --refractory[i];
        else {
            voltage[i] = std::clamp(-52.0f + (voltage[i] + 52.0f) * membraneDecay
                                   + conductance[i] * (1.0f - membraneDecay), -100.0f, 100.0f);
            if (p < 2 && uniform() < (p == 0 ? aProbability : bProbability)) voltage[i] += 68.75f;
            spike = voltage[i] > -45.0f;
        }
        if (spike) {
            voltage[i] = -52; conductance[i] = 0; refractory[i] = 11; // 2.2 ms
            rate[i] += (1.0f - rateDecay) / stepSeconds;
            ++spikeCount; if (p >= 2) ++centralSpikes;
            if (emitVisual && !shared.spikes.push({static_cast<std::uint32_t>(i), tick}))
                shared.telemetry.droppedSpikes.fetch_add(1, std::memory_order_relaxed);
            for (auto e = graph.offsets[i]; e < graph.offsets[i + 1]; ++e)
                delayed[nextSlot + graph.synapses[e].target] += graph.synapses[e].weightMV;
        }
        sum[p] += rate[i];
    }
    for (std::size_t p = 0; p < populationCount; ++p)
        populationRates[p] = graph.counts[p] ? sum[p] / static_cast<float>(graph.counts[p]) : 0;
    ++tick;
}
}
