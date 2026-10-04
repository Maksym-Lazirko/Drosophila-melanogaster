#pragma once
#include "DataLoader.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <type_traits>

namespace fly {
static_assert(std::atomic<float>::is_always_lock_free, "Requires lock-free float atomics");
static_assert(std::atomic<std::uint32_t>::is_always_lock_free, "Requires lock-free index atomics");
template <typename T, std::uint32_t Capacity> class SpscQueue {
    static_assert(Capacity > 1 && (Capacity & (Capacity - 1)) == 0, "Use power-of-two capacity");
public:
    bool push(const T& value) noexcept {
        const auto w = write.value.load(std::memory_order_relaxed);
        if (w - read.value.load(std::memory_order_acquire) == Capacity) return false;
        data[w & (Capacity - 1)] = value;
        write.value.store(w + 1, std::memory_order_release); return true;
    }
    bool pop(T& value) noexcept {
        const auto r = read.value.load(std::memory_order_relaxed);
        if (r == write.value.load(std::memory_order_acquire)) return false;
        value = data[r & (Capacity - 1)]; read.value.store(r + 1, std::memory_order_release); return true;
    }
private:
    std::array<T, Capacity> data {};
    struct alignas(64) Cursor {
        std::atomic<std::uint32_t> value {0};
        std::array<std::byte, 64 - sizeof(std::atomic<std::uint32_t>)> padding {};
    };
    Cursor write, read;
};
struct FeatureFrame { float low = 0, high = 0, envelope = 0, onset = 0, seconds = 0.001f; };
struct SpikeEvent { std::uint32_t neuron = 0, tick = 0; };
struct Telemetry {
    std::array<std::atomic<float>, populationCount> rates {};
    std::array<std::atomic<float>, populationCount> audioRates {}; // exact latest buffer input to DSP
    std::atomic<std::uint32_t> renderedFrames {0};
    std::atomic<float> input {0}, workerLoad {0};
    std::atomic<std::uint32_t> droppedFeatures {0}, droppedSpikes {0}, cappedTicks {0};
};
struct SharedState {
    SpscQueue<FeatureFrame, 2048> features;
    SpscQueue<SpikeEvent, 65536> spikes;
    Telemetry telemetry;
    std::atomic<bool> visualActive {false}, visualizationEnabled {true}, prepared {false};
};
}
