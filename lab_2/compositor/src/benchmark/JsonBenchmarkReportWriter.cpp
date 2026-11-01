#include "benchmark/JsonBenchmarkReportWriter.hpp"

#include <boost/json.hpp>
#include <chrono>
#include <fstream>
#include <random>
#include <stdexcept>
#include <thread>

namespace compositor::benchmark {
namespace {
namespace json = boost::json;
json::object statistics(const std::vector<Sample>& samples) {
    const auto stats = summarize(samples);
    return {{"min_ns",stats.minimumNs},{"max_ns",stats.maximumNs},{"median_ns",stats.medianNs},
            {"mean_ns",stats.meanNs},{"stddev_ns",stats.standardDeviationNs}};
}
json::array samples(const std::vector<Sample>& values) {
    json::array result;
    for (const auto& value : values) result.push_back(json::object{{"duration_ns",value.durationNs},
                                                                 {"mismatched_pixels",value.mismatchedPixels}});
    return result;
}
json::array counts(const std::vector<std::size_t>& values) {
    json::array result;
    for (auto count : values) result.push_back(count);
    return result;
}
const char* mode(RenderMode value) {
    switch (value) {
    case RenderMode::Sequential: return "sequential";
    case RenderMode::ParallelSafe: return "parallel-safe";
    case RenderMode::ParallelUnsafe: return "parallel-unsafe";
    }
    throw std::invalid_argument("Unknown benchmark mode");
}
const char* scenario(Scenario value) {
    switch (value) {
    case Scenario::Sparse: return "sparse";
    case Scenario::Overlap: return "overlap";
    case Scenario::Large: return "large";
    case Scenario::Mixed: return "mixed";
    }
    throw std::invalid_argument("Unknown benchmark scenario");
}
json::array rgb(Color color) { return {color.red(),color.green(),color.blue()}; }
class TemporaryReport {
public:
    explicit TemporaryReport(const std::filesystem::path& parent) {
        std::random_device random;
        for (unsigned i=0; i<64; ++i) {
            directory_ = parent / (".benchmark-" + std::to_string(random()) + "-" + std::to_string(random()));
            if (std::filesystem::create_directory(directory_)) return;
        }
        throw std::runtime_error("Cannot reserve temporary report directory");
    }
    ~TemporaryReport() { std::error_code error; std::filesystem::remove_all(directory_,error); }
    TemporaryReport(const TemporaryReport&) = delete;
    TemporaryReport& operator=(const TemporaryReport&) = delete;
    std::filesystem::path file() const { return directory_ / "report.json"; }
private:
    std::filesystem::path directory_;
};
}

void JsonBenchmarkReportWriter::write(const BenchmarkPlan& plan, const BenchmarkResult& result) const {
    if (result.baseline.size() != plan.options.repetitions || result.candidate.size() != plan.options.repetitions ||
        result.baselineWarmupMismatches.size() != plan.options.warmup || result.candidateWarmupMismatches.size() != plan.options.warmup)
        throw std::invalid_argument("Incomplete benchmark result");
    const bool correct = matchesReference(result);
    const bool unsafe = plan.run.render.mode == RenderMode::ParallelUnsafe;
    const auto baseline = summarize(result.baseline);
    const auto candidate = summarize(result.candidate);
    json::array rectangles;
    for (const auto& r : plan.run.scene.rectangles()) rectangles.push_back(json::object{
        {"x",r.x()},{"y",r.y()},{"width",r.width()},{"height",r.height()},{"color",rgb(r.color())},{"priority",r.priority()}});
    json::object document{
        {"schema_version",1},
        {"recorded_at_unix_ns",std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count())},
        {"measurement","Compositor::render only; preparation, validation and I/O excluded"},
        {"input",plan.inputPath.string()},
        {"build",json::object{{"compiler",COMPOSITOR_BENCH_COMPILER},{"configuration",COMPOSITOR_BENCH_CONFIGURATION},
            {"revision",COMPOSITOR_BENCH_REVISION},{"dirty_at_configure",COMPOSITOR_BENCH_DIRTY},
            {"platform",COMPOSITOR_BENCH_PLATFORM}}},
        {"hardware_concurrency",std::thread::hardware_concurrency()},
        {"canvas",json::object{{"width",plan.run.canvas.width},{"height",plan.run.canvas.height},{"background",rgb(plan.run.canvas.background)}}},
        {"rectangles",std::move(rectangles)},
        {"render",json::object{{"mode",mode(plan.run.render.mode)},{"requested_threads",plan.run.render.threads},
            {"effective_workers",plan.run.render.mode == RenderMode::Sequential ? 1 : plan.run.render.threads},
            {"region_width",plan.run.render.regionWidth},{"region_height",plan.run.render.regionHeight}}},
        {"baseline_render",json::object{{"mode","sequential"},{"region_width",plan.run.canvas.width},{"region_height",plan.run.canvas.height}}},
        {"warmup",plan.options.warmup},{"repetitions",plan.options.repetitions},
        {"iteration_order","even: baseline then candidate; odd: candidate then baseline"},
        {"baseline",json::object{{"samples",samples(result.baseline)},{"statistics",statistics(result.baseline)},
            {"warmup_mismatches",counts(result.baselineWarmupMismatches)}}},
        {"candidate",json::object{{"samples",samples(result.candidate)},{"statistics",statistics(result.candidate)},
            {"warmup_mismatches",counts(result.candidateWarmupMismatches)}}},
        {"matches_reference",correct},{"unsafe_experiment",unsafe},
        {"comparison_eligible",correct && !unsafe && candidate.medianNs > 0 && baseline.medianNs > 0},
        {"median_speedup",nullptr}, {"generator",nullptr}};
    if (correct && !unsafe && candidate.medianNs > 0 && baseline.medianNs > 0)
        document["median_speedup"] = baseline.medianNs / candidate.medianNs;
    if (plan.generator) document["generator"] = json::object{{"scenario",scenario(plan.generator->scenario)},
        {"rectangles",plan.generator->rectangleCount},{"seed",plan.generator->seed},{"algorithm","mt19937-modulo-v1"}};
    const auto serialized = json::serialize(document);
    const auto target = std::filesystem::absolute(plan.reportPath);
    const TemporaryReport temporary{target.parent_path()};
    std::ofstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output.open(temporary.file(),std::ios::binary);
    output << serialized << '\n';
    output.close();
    std::filesystem::rename(temporary.file(),target);
}

} // namespace compositor::benchmark
