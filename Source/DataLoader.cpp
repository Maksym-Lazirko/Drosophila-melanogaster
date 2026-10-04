#include "DataLoader.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace fly {
namespace {
class Reader {
public:
    Reader(const void* data, std::size_t size) : p(static_cast<const std::uint8_t*>(data)), left(size) {}
    std::uint64_t integer(std::size_t n) {
        require(n); std::uint64_t v = 0;
        for (std::size_t i = 0; i < n; ++i) v |= std::uint64_t(*p++) << (8 * i);
        left -= n; return v;
    }
    float real() {
        const auto bits = static_cast<std::uint32_t>(integer(4));
        float v; std::memcpy(&v, &bits, 4);
        if (!std::isfinite(v)) throw std::runtime_error("Non-finite graph value");
        return v;
    }
    std::string text(std::size_t n) {
        require(n); std::string v(reinterpret_cast<const char*>(p), n); p += n; left -= n; return v;
    }
    void require(std::size_t n) const { if (n > left) throw std::runtime_error("Truncated auditory graph"); }
    std::size_t remaining() const { return left; }
private:
    const std::uint8_t* p; std::size_t left;
};
}
Graph loadGraph(const void* bytes, std::size_t size) {
    if (!bytes || size > 32 * 1024 * 1024) throw std::runtime_error("Invalid auditory graph size");
    Reader r(bytes, size);
    if (r.text(8) != std::string("FLYAUD1\0", 8) || r.integer(4) != 783)
        throw std::runtime_error("Expected FLYAUD1 FlyWire v783 graph");
    const auto n = static_cast<std::uint32_t>(r.integer(4));
    const auto m = static_cast<std::uint32_t>(r.integer(4));
    if (n == 0 || n > 4096 || m > 1000000) throw std::runtime_error("Auditory graph exceeds safety limits");
    Graph graph; graph.neurons.reserve(n); graph.synapses.reserve(m); graph.offsets.assign(n + 1, 0);
    std::unordered_set<std::uint64_t> ids;
    for (std::uint32_t i = 0; i < n; ++i) {
        Neuron v; v.rootId = r.integer(8); v.population = static_cast<std::uint8_t>(r.integer(1));
        v.neurotransmitter = static_cast<std::uint8_t>(r.integer(1));
        const auto sign = r.integer(1); v.polarity = sign == 255 ? -1 : static_cast<int>(sign); r.integer(1);
        v.x = r.real(); v.y = r.real(); v.z = r.real(); v.confidence = r.real();
        const auto length = r.integer(2);
        if (v.rootId == 0 || !ids.insert(v.rootId).second || v.population >= populationCount
            || v.neurotransmitter > 6 || v.polarity < -1 || v.polarity > 1
            || v.confidence < 0 || v.confidence > 1 || length == 0 || length > 256)
            throw std::runtime_error("Invalid neuron metadata");
        const auto expectedSign = v.neurotransmitter == 1 ? 1 : (v.neurotransmitter == 2 || v.neurotransmitter == 3) ? -1 : 0;
        if (v.polarity != expectedSign) throw std::runtime_error("Inconsistent modeled neurotransmitter sign");
        v.cellType = r.text(static_cast<std::size_t>(length)); ++graph.counts[v.population]; graph.neurons.push_back(std::move(v));
    }
    std::vector<std::pair<std::uint32_t, Synapse>> edges; edges.reserve(m);
    std::unordered_set<std::uint64_t> pairs;
    for (std::uint32_t i = 0; i < m; ++i) {
        auto pre = static_cast<std::uint32_t>(r.integer(4)); auto post = static_cast<std::uint32_t>(r.integer(4));
        auto count = static_cast<std::uint32_t>(r.integer(4)); auto weight = r.real();
        if (pre >= n || post >= n || count == 0 || count > 1000000
            || !pairs.insert((std::uint64_t(pre) << 32) | post).second)
            throw std::runtime_error("Invalid or duplicate graph edge");
        const auto expected = 0.275f * static_cast<float>(count) * static_cast<float>(graph.neurons[pre].polarity);
        if (std::abs(weight - expected) > 0.001f * std::max(1.0f, std::abs(expected)))
            throw std::runtime_error("Edge weight does not match count and modeled sign");
        ++graph.offsets[pre + 1]; edges.push_back({pre, {post, count, weight}});
    }
    if (r.remaining() != 0 || graph.counts[0] == 0 || graph.counts[1] == 0)
        throw std::runtime_error("Trailing data or missing JO sensory populations");
    for (std::size_t i = 1; i < graph.offsets.size(); ++i) graph.offsets[i] += graph.offsets[i - 1];
    std::sort(edges.begin(), edges.end(), [](const auto& a, const auto& b) {
        return a.first != b.first ? a.first < b.first : a.second.target < b.second.target; });
    for (const auto& e : edges) graph.synapses.push_back(e.second);
    return graph;
}
Graph loadGraphFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open auditory graph: " + path);
    const auto size = file.tellg();
    if (size <= 0 || size > 32 * 1024 * 1024) throw std::runtime_error("Invalid auditory graph file size");
    std::vector<char> bytes(static_cast<std::size_t>(size)); file.seekg(0);
    if (!file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) throw std::runtime_error("Cannot read auditory graph");
    return loadGraph(bytes.data(), bytes.size());
}
}
