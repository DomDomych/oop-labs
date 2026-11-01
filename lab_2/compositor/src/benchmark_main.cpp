#include "application/RenderComponents.hpp"
#include "benchmark/BenchmarkPlan.hpp"
#include "benchmark/BenchmarkRunner.hpp"
#include "benchmark/JsonBenchmarkReportWriter.hpp"
#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: compositor_bench <benchmark.json>\n"; return 2; }
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
        return matchesReference(result) ? 0 : 3;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
