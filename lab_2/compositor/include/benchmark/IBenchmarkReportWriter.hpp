#pragma once

#include "benchmark/BenchmarkPlan.hpp"

namespace compositor::benchmark {

class IBenchmarkReportWriter {
public:
    virtual ~IBenchmarkReportWriter() = default;
    // Writes measurements, validation, settings and reproducibility metadata.
    // Throws on failure; no changes to the scene or measurements.
    virtual void write(const BenchmarkPlan& plan, const BenchmarkResult& result) const = 0;
};

} // namespace compositor::benchmark
