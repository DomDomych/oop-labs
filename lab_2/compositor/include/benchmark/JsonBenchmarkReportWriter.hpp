#pragma once

#include "benchmark/IBenchmarkReportWriter.hpp"

namespace compositor::benchmark {

class JsonBenchmarkReportWriter final : public IBenchmarkReportWriter {
public:
    // Stages complete JSON before replacing the report. Parent must exist.
    void write(const BenchmarkPlan& plan, const BenchmarkResult& result) const override;
};

} // namespace compositor::benchmark
