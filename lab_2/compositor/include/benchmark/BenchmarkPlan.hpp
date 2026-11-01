#pragma once

#include "benchmark/BenchmarkResult.hpp"
#include "benchmark/SceneGenerator.hpp"
#include "input/RunConfig.hpp"
#include <optional>

namespace compositor::benchmark {

struct BenchmarkPlan {
    RunConfig run;
    BenchmarkOptions options;
    std::filesystem::path inputPath;
    std::filesystem::path reportPath;
    std::optional<GeneratorConfig> generator;
};

class BenchmarkPlanLoader {
public:
    // Reads a benchmark plan and its referenced ordinary run config.
    // Optional generator replaces rectangles, retaining canvas/render settings.
    // Paths are relative to their containing configuration file.
    BenchmarkPlan load(const std::filesystem::path& path) const;
};

} // namespace compositor::benchmark
