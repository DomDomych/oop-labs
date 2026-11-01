#pragma once

#include "benchmark/BenchmarkResult.hpp"
#include "rendering/Compositor.hpp"

namespace compositor::benchmark {

class BenchmarkRunner {
public:
    // Candidate dependencies/pool are constructed by the caller and reused.
    // Copies initial BEFORE timing; compares all pixels AFTER timing.
    // Reference is sequential; its one-region grid avoids arbitrary tile tuning.
    // Exceptions propagate. No I/O, logging, thread creation or file loading in timing.
    // Unsafe candidates retain their UB: diagnostics cannot make them reliable.
    BenchmarkResult run(const Scene& scene, const Image& initial,
                        const Compositor& candidate, const BenchmarkOptions& options) const;
};

} // namespace compositor::benchmark
