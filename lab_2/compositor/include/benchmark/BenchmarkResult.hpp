#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace compositor::benchmark {

inline constexpr std::size_t kDefaultBenchmarkRepetitions = 10;
inline constexpr std::size_t kDefaultBenchmarkWarmupRuns = 2;

struct BenchmarkOptions {
    std::size_t warmup = kDefaultBenchmarkWarmupRuns;
    std::size_t repetitions = kDefaultBenchmarkRepetitions;
};

struct Sample {
    std::int64_t durationNs;
    std::size_t mismatchedPixels;
};

struct TimingStatistics {
    double minimumNs;
    double maximumNs;
    double medianNs;
    double meanNs;
    double standardDeviationNs;
};

struct BenchmarkResult {
    std::vector<Sample> baseline;
    std::vector<Sample> candidate;
    std::vector<std::size_t> baselineWarmupMismatches;
    std::vector<std::size_t> candidateWarmupMismatches;
};

TimingStatistics summarize(const std::vector<Sample>& samples);
bool matchesReference(const BenchmarkResult& result);

} // namespace compositor::benchmark
