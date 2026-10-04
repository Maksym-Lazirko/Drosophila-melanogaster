#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fly {
constexpr std::size_t populationCount = 7;
inline constexpr std::array<const char*, populationCount> populationNames {
    "JO-A", "JO-B", "AMMC-A1", "AMMC-A2", "AMMC-B1", "AMMC-B2", "Central auditory" };
struct Neuron {
    std::uint64_t rootId = 0;
    std::uint8_t population = 0, neurotransmitter = 0;
    int polarity = 0;
    float x = 0, y = 0, z = 0, confidence = 0;
    std::string cellType;
};
struct Synapse { std::uint32_t target = 0, count = 0; float weightMV = 0; };
struct Graph {
    std::vector<Neuron> neurons;
    std::vector<std::uint32_t> offsets;
    std::vector<Synapse> synapses;
    std::array<std::uint32_t, populationCount> counts {};
};
// Throws descriptive exceptions on malformed, oversized or wrong-version data.
Graph loadGraph(const void* bytes, std::size_t size);
Graph loadGraphFile(const std::string& path);
}
