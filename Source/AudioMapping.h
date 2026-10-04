#pragma once
#include "RealtimeTransport.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace fly {
class AudioFeatures {
public:
    void prepare(double rate) noexcept {
        sampleRate = rate; windowSize = std::max(1, static_cast<int>(std::round(rate * 0.001)));
        count = 0; lowPower = highPower = fullPower = previousEnvelope = 0;
        cachedCentre = cachedOctaves = -1;
        for (auto& channel : state) channel.fill(0);
    }
    void setBands(float centre, float octaves) noexcept {
        if (centre == cachedCentre && octaves == cachedOctaves) return;
        cachedCentre = centre; cachedOctaves = octaves;
        const float spread = std::pow(2.0f, std::clamp(octaves, 0.5f, 3.0f) * 0.5f);
        const float lowCentre = centre / spread, highCentre = centre * spread;
        const auto pole = [this](float hz) { return static_cast<float>(1.0 - std::exp(-6.283185307179586 * std::clamp<double>(hz, 20, sampleRate * 0.4) / sampleRate)); };
        coefficients = {pole(lowCentre * 0.55f), pole(lowCentre * 1.7f), pole(highCentre * 0.55f), pole(highCentre * 1.7f)};
    }
    // Call once per sample, before modifying the input. Stereo powers are summed,
    // so anti-phase signals cannot cancel the sensory drive.
    bool consume(float left, float right, int channels, FeatureFrame& frame) noexcept {
        const float values[2] {left, right};
        for (int c = 0; c < channels; ++c) {
            const float x = std::clamp(values[c], -8.0f, 8.0f); auto& z = state[static_cast<std::size_t>(c)];
            for (std::size_t k = 0; k < 4; ++k) z[k] += coefficients[k] * (x - z[k]);
            const auto lo = z[1] - z[0], hi = z[3] - z[2];
            lowPower += lo * lo / static_cast<float>(channels); highPower += hi * hi / static_cast<float>(channels);
            fullPower += x * x / static_cast<float>(channels);
        }
        if (++count < windowSize) return false;
        const float envelope = std::sqrt(fullPower / static_cast<float>(count));
        frame = {std::sqrt(lowPower / static_cast<float>(count)), std::sqrt(highPower / static_cast<float>(count)),
                 envelope, std::max(0.0f, envelope - previousEnvelope), static_cast<float>(count / sampleRate)};
        previousEnvelope = envelope; count = 0; lowPower = highPower = fullPower = 0; return true;
    }
private:
    double sampleRate = 48000; int windowSize = 48, count = 0;
    float lowPower = 0, highPower = 0, fullPower = 0, previousEnvelope = 0;
    float cachedCentre = -1, cachedOctaves = -1;
    std::array<float, 4> coefficients {};
    std::array<std::array<float, 4>, 2> state {};
};
struct EffectSettings { float wet = 1.0f, depth = 1.0f; int mode = 0; };
class NeuralAudioDSP {
public:
    void prepare(double sr) noexcept {
        sampleRate = sr; smoothing = static_cast<float>(1.0 - std::exp(-1.0 / (0.02 * sr)));
        b1 = wed = ammc = mix = depth = 0;
        for (auto& c : state) c.fill(0);
    }
    void setTargets(const std::array<float, populationCount>& rates, EffectSettings settings) noexcept {
        targetB1 = std::clamp(rates[4] / 100.0f, 0.0f, 1.0f);
        targetWed = std::clamp(rates[6] / 100.0f, 0.0f, 1.0f);
        targetAmmc = std::clamp((rates[2] + rates[3] + rates[5]) / 300.0f, 0.0f, 1.0f);
        targetMix = std::clamp(settings.wet, 0.0f, 1.0f); targetDepth = std::clamp(settings.depth, 0.0f, 1.0f);
        mode = std::clamp(settings.mode, 0, 3);
        // Coefficients changed only at buffer boundaries; modulation smoothing
        // occurs sample-by-sample, preventing audible steps in effect depth.
        // Clean carrier mapping, not a measured fly transfer function. A
        // 1200-to-150 Hz low-pass makes low-rate auditory modulation observable
        // without adding waveshaping, gain drive or a gate in clean mode.
        const float cutoff = 1200.0f * std::pow(0.125f, std::clamp(0.6f * b1 + 0.4f * wed, 0.0f, 1.0f));
        g = static_cast<float>(std::tan(3.141592653589793 * std::clamp<double>(cutoff, 30, sampleRate * 0.4) / sampleRate));
        resonance = 1.4f - 1.1f * ammc;
        a1 = 1.0f / (1.0f + g * (g + resonance)); a2 = g * a1; a3 = g * a2;
    }
    void nextSample() noexcept {
        b1 += smoothing * (targetB1 - b1); wed += smoothing * (targetWed - wed); ammc += smoothing * (targetAmmc - ammc);
        mix += smoothing * (targetMix - mix); depth += smoothing * (targetDepth - depth);
        // Shared stereo modulation, evaluated once per sample. Keep the filter
        // state running in every mode so automated mode switches remain continuous.
        if (mode == 0 || mode == 3) filterAmount = std::clamp(0.7f * b1 + 0.3f * wed, 0.0f, 1.0f);
        if (mode == 1 || mode == 3) gate = 0.15f + 0.85f * std::clamp(0.8f * b1 + 0.2f * wed, 0.0f, 1.0f);
        if (mode == 2 || mode == 3) {
            shapeAmount = std::clamp(0.5f * b1 + 0.5f * wed, 0.0f, 1.0f);
            drive = 1.0f + 12.0f * shapeAmount; driveSaturation = saturate(drive);
        }
    }
    float process(float x, int channel) noexcept {
        auto& z = state[static_cast<std::size_t>(channel)]; const auto safe = std::clamp(x, -8.0f, 8.0f);
        const float v3 = safe - z[1], v1 = a1 * z[0] + a2 * v3, v2 = z[1] + a2 * z[0] + a3 * v3;
        z[0] = 2 * v1 - z[0]; z[1] = 2 * v2 - z[1];
        float wetSignal;
        if (mode == 0) wetSignal = safe + filterAmount * (v2 - safe);
        else if (mode == 1) wetSignal = safe * gate / (1.0f + std::abs(safe) * 4.0f * ammc);
        else if (mode == 2) wetSignal = safe + shapeAmount * (saturate(safe * drive) / driveSaturation - safe);
        else {
            const float filtered = safe + filterAmount * (v2 - safe);
            const float compressed = safe * gate / (1.0f + std::abs(safe) * 4.0f * ammc);
            const float shaped = safe + shapeAmount * (saturate(safe * drive) / driveSaturation - safe);
            wetSignal = 0.45f * filtered + 0.25f * compressed + 0.30f * shaped;
        }
        return x + mix * depth * (wetSignal - x);
    }
private:
    static float saturate(float x) noexcept { x = std::clamp(x, -3.0f, 3.0f); return x * (27 + x * x) / (27 + 9 * x * x); }
    double sampleRate = 48000;
    float smoothing = 0.001f, b1 = 0, wed = 0, ammc = 0, mix = 0, depth = 0;
    float targetB1 = 0, targetWed = 0, targetAmmc = 0, targetMix = 0, targetDepth = 0;
    float g = 0, resonance = 1, a1 = 1, a2 = 0, a3 = 0;
    float filterAmount = 0, gate = 0.15f, shapeAmount = 0, drive = 1, driveSaturation = 1;
    int mode = 0;
    std::array<std::array<float, 2>, 2> state {};
};
}
