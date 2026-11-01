#pragma once

#include "model/Scene.hpp"
#include <cstddef>
#include <cstdint>

namespace compositor::benchmark {

enum class Scenario { Sparse, Overlap, Large, Mixed };

struct GeneratorConfig {
    Scenario scenario;
    std::size_t rectangleCount;
    std::uint32_t seed;
};

class SceneGenerator {
public:
    // Deterministic mt19937 output and integer mapping, independent of std distributions.
    // Sparse uses one rectangle per grid cell and rejects counts above pixel count.
    // Generated geometry fits positive int dimensions; arbitrary input files have
    // the wider limits supported by the existing model/loader.
    Scene generate(std::size_t width, std::size_t height, const GeneratorConfig& config) const;
};

} // namespace compositor::benchmark
