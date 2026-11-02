#include "application/RenderComponents.hpp"
#include "benchmark/BenchmarkPlan.hpp"
#include "benchmark/BenchmarkRunner.hpp"
#include "benchmark/JsonBenchmarkReportWriter.hpp"
#include <exception>
#include <iostream>

namespace {
constexpr int kExitSuccess = 0;
constexpr int kExitFailure = 1;
constexpr int kExitUsageError = 2;
constexpr int kExitReferenceMismatch = 3;
constexpr int kExpectedArgumentCount = 2;
}

int main(int argc, char** argv) {
    if (argc != kExpectedArgumentCount) {
        std::cerr << "Usage: compositor_bench <benchmark.json>\n";
        return kExitUsageError;
    }
    try {
        using namespace compositor;
        using namespace compositor::benchmark;
        const auto plan = BenchmarkPlanLoader{}.load(argv[1]);
        if (plan.run.render.mode == RenderMode::ParallelUnsafe)
            std::cerr << "Experimental unsafe run: data races are UB; process/results are not reliable.\n";
        auto components = createRenderComponents(plan.run.render);
        const Compositor compositor{plan.run.render.regionWidth,plan.run.render.regionHeight,*components.executor,*components.policy};
        const Image initial{plan.run.canvas.width,plan.run.canvas.height,plan.run.canvas.background};
        const auto result = BenchmarkRunner{}.run(plan.run.scene,initial,compositor,plan.options);
        JsonBenchmarkReportWriter{}.write(plan,result);
        std::cout << "Report: " << plan.reportPath << '\n';
        return matchesReference(result) ? kExitSuccess : kExitReferenceMismatch;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return kExitFailure; }
}
