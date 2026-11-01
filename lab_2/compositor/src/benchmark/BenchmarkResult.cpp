#include "benchmark/BenchmarkResult.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace compositor::benchmark {

TimingStatistics summarize(const std::vector<Sample>& samples) {
    if (samples.empty()) throw std::invalid_argument("Cannot summarize empty measurements");
    std::vector<double> values;
    values.reserve(samples.size());
    for (const auto& sample : samples) {
        if (sample.durationNs < 0) throw std::invalid_argument("Negative measurement");
        values.push_back(static_cast<double>(sample.durationNs));
    }
    std::sort(values.begin(), values.end());
    const auto middle = values.size() / 2;
    const double median = values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2;
    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0;
    for (const auto value : values) variance += (value - mean) * (value - mean);
    return {values.front(), values.back(), median, mean, std::sqrt(variance / values.size())};
}

bool matchesReference(const BenchmarkResult& result) {
    const auto cleanSamples = [](const auto& samples) {
        return !samples.empty() && std::all_of(samples.begin(), samples.end(), [](const Sample& sample) {
            return sample.mismatchedPixels == 0;
        });
    };
    const auto cleanWarmup = [](const auto& values) {
        return std::all_of(values.begin(), values.end(), [](std::size_t value) { return value == 0; });
    };
    return cleanSamples(result.baseline) && cleanSamples(result.candidate) &&
           cleanWarmup(result.baselineWarmupMismatches) && cleanWarmup(result.candidateWarmupMismatches);
}

} // namespace compositor::benchmark
