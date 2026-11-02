#include "benchmark/BenchmarkRunner.hpp"
#include "benchmark/ImageComparator.hpp"
#include "execution/SequentialExecutor.hpp"
#include "synchronization/NoLockPolicy.hpp"

#include <chrono>
#include <stdexcept>

namespace compositor::benchmark {
namespace {
constexpr std::size_t kIterationOrderAlternationPeriod = 2;
}


BenchmarkResult BenchmarkRunner::run(const Scene& scene, const Image& initial,
                                     const Compositor& candidate, const BenchmarkOptions& options) const {
    if (options.repetitions == 0) throw std::invalid_argument("Benchmark requires at least one repetition");
    SequentialExecutor executor;
    const NoLockPolicy policy;
    const Compositor baseline{initial.width(), initial.height(), executor, policy};
    Image reference{initial};
    baseline.render(scene, reference);

    BenchmarkResult result;
    result.baseline.reserve(options.repetitions);
    result.candidate.reserve(options.repetitions);
    result.baselineWarmupMismatches.reserve(options.warmup);
    result.candidateWarmupMismatches.reserve(options.warmup);
    auto warmup = [&](const Compositor& compositor) {
        Image canvas{initial};
        compositor.render(scene, canvas);
        return countMismatchedPixels(reference, canvas);
    };
    for (std::size_t i = 0; i < options.warmup; ++i) {
        result.baselineWarmupMismatches.push_back(warmup(baseline));
        result.candidateWarmupMismatches.push_back(warmup(candidate));
    }
    auto measure = [&](const Compositor& compositor) {
        Image canvas{initial};
        const auto start = std::chrono::steady_clock::now();
        compositor.render(scene, canvas);
        const auto finish = std::chrono::steady_clock::now();
        return Sample{std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count(),
                      countMismatchedPixels(reference, canvas)};
    };
    for (std::size_t i = 0; i < options.repetitions; ++i) {
        // Alternate order to reduce a fixed first/second bias.
        if (i % kIterationOrderAlternationPeriod == 0) {
            result.baseline.push_back(measure(baseline));
            result.candidate.push_back(measure(candidate));
        } else {
            result.candidate.push_back(measure(candidate));
            result.baseline.push_back(measure(baseline));
        }
    }
    return result;
}

} // namespace compositor::benchmark
