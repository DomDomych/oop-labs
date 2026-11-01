#include "benchmark/BenchmarkRunner.hpp"
#include "benchmark/ImageComparator.hpp"
#include "benchmark/SceneGenerator.hpp"
#include "benchmark/JsonBenchmarkReportWriter.hpp"
#include "application/RenderComponents.hpp"
#include "synchronization/NoLockPolicy.hpp"

#include <boost/json.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
using namespace compositor;
using namespace compositor::benchmark;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template <class F> void fails(F function) {
    try { function(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Expected failure");
}
class TempDirectory {
public:
    std::filesystem::path path;
    TempDirectory() {
        const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i=0; i<100; ++i) {
            path = std::filesystem::temp_directory_path() / ("bench-test-" + std::to_string(tick) + "-" + std::to_string(i));
            if (std::filesystem::create_directory(path)) return;
        }
        throw std::runtime_error("Cannot reserve test directory");
    }
    ~TempDirectory() { std::error_code error; std::filesystem::remove_all(path,error); }
};
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path);
    require(bool(input),"Read report");
    return {std::istreambuf_iterator<char>{input},std::istreambuf_iterator<char>{}};
}
void statisticsTests() {
    const auto summary = summarize({{4,0},{1,0},{3,0},{2,0}});
    require(summary.minimumNs == 1 && summary.maximumNs == 4 && summary.medianNs == 2.5 && summary.meanNs == 2.5,
            "Even median and order-independent statistics");
    require(std::abs(summary.standardDeviationNs-std::sqrt(1.25)) < 1e-10,"Population deviation");
    require(summarize({{5,0}}).standardDeviationNs == 0,"Single sample deviation");
    require(summarize({{10,0},{1,0},{2,0}}).medianNs == 2,"Odd median");
    fails([] { (void)summarize({}); });
    fails([] { (void)summarize({{-1,0}}); });
}
class CountingExecutor final : public IExecutor {
public:
    unsigned calls = 0;
    bool skip = false;
    void execute(std::size_t count, const Task& task) override {
        ++calls;
        if (!skip) for (std::size_t i=0; i<count; ++i) task(i);
    }
};
void runnerTests() {
    const Color white{255,255,255};
    const Image initial{5,3,white};
    const Scene scene{{Rectangle{-1,0,3,3,Color{255,0,0},1},Rectangle{1,1,4,2,Color{0,0,255},1}}};
    CountingExecutor executor;
    const NoLockPolicy policy;
    const Compositor compositor{2,2,executor,policy};
    const auto result = BenchmarkRunner{}.run(scene,initial,compositor,{2,3});
    require(executor.calls == 5 && result.candidate.size() == 3 && result.candidateWarmupMismatches.size() == 2,
            "Only requested warmup/repetitions use candidate executor");
    require(matchesReference(result),"Correct composition matches reference every time");
    require(initial.colorAt(0,0) == white && !initial.at(0,0).winner,"Initial image is preserved");
    executor.skip = true;
    const auto bad = BenchmarkRunner{}.run(scene,initial,compositor,{1,2});
    require(!matchesReference(bad) && bad.candidateWarmupMismatches[0] > 0 && bad.candidate[0].mismatchedPixels > 0,
            "Warmup and measured corruption detected");
    fails([&] { (void)BenchmarkRunner{}.run(scene,initial,compositor,{0,0}); });
    Image metadata{initial};
    metadata.setPixel(0,0,PixelState{white,RectangleMetadata{1,0}});
    require(countMismatchedPixels(initial,metadata) == 1,"Equal color with different metadata is a mismatch");
    fails([&] { (void)countMismatchedPixels(initial,Image{1,1,white}); });
    for (const auto mode : {RenderMode::Sequential,RenderMode::ParallelSafe,RenderMode::ParallelUnsafe}) {
        auto components = createRenderComponents({mode,2,2,2});
        const Compositor candidate{2,2,*components.executor,*components.policy};
        // Disjoint scene avoids executing intentional UB in tests.
        const Scene disjoint{{Rectangle{0,0,1,1,Color{0,0,0},0},Rectangle{4,2,1,1,Color{0,1,0},1}}};
        require(matchesReference(BenchmarkRunner{}.run(disjoint,initial,candidate,{0,2})),"All selected modes compose correctly");
    }
}
void generatorTests() {
    const SceneGenerator generator;
    for (const auto kind : {Scenario::Sparse,Scenario::Overlap,Scenario::Large,Scenario::Mixed}) {
        const auto first = generator.generate(64,32,{kind,100,42});
        const auto second = generator.generate(64,32,{kind,100,42});
        require(first.rectangles() == second.rectangles(),"Seed reproducibility");
        require(first.rectangles() != generator.generate(64,32,{kind,100,43}).rectangles(),"Different seeds change data");
        require(generator.generate(1,1,{kind,1,0}).rectangles().size() == 1,"Minimal canvas");
        require(generator.generate(1,1,{kind,0,0}).rectangles().empty(),"Empty generated scene");
    }
    const auto sparse = generator.generate(13,7,{Scenario::Sparse,90,3});
    std::vector<unsigned> hits(13*7);
    for (const auto& rectangle : sparse.rectangles()) {
        for (int y=rectangle.y(); y<rectangle.bottom(); ++y)
            for (int x=rectangle.x(); x<rectangle.right(); ++x)
                require(++hits.at(static_cast<std::size_t>(y*13+x)) == 1,"Sparse cells never overlap");
    }
    fails([&] { (void)generator.generate(2,2,{Scenario::Sparse,5,0}); });
    fails([&] { (void)generator.generate(0,2,{Scenario::Mixed,1,0}); });
}
void ioTests() {
    TempDirectory directory;
    const auto inputPath = directory.path / "run.json";
    { std::ofstream file(inputPath); file << R"({"canvas":{"width":8,"height":4,"background":[255,255,255]},"render":{"mode":"parallel-safe","threads":2,"region":{"width":2,"height":2}},"rectangles":[],"output":{"image":"unused.ppm"}})"; }
    const auto planPath = directory.path / "plan.json";
    auto load = [&](const std::string& content) {
        std::ofstream file(planPath); file << content; file.close();
        return BenchmarkPlanLoader{}.load(planPath);
    };
    auto plan = load(R"({"input":"run.json","warmup":0,"repetitions":2,"report":"result.json","generate":{"scenario":"mixed","rectangles":5,"seed":42}})");
    require(plan.run.scene.rectangles().size() == 5 && plan.reportPath == directory.path / "result.json","Plan paths and generation");
    auto components = createRenderComponents(plan.run.render);
    const Compositor candidate{2,2,*components.executor,*components.policy};
    const Image initial{8,4,Color{255,255,255}};
    const auto result = BenchmarkRunner{}.run(plan.run.scene,initial,candidate,plan.options);
    const JsonBenchmarkReportWriter writer;
    writer.write(plan,result);
    const auto doc = boost::json::parse(read(plan.reportPath));
    require(doc.at("matches_reference").as_bool() && doc.at("candidate").at("samples").as_array().size() == 2,"Report raw data and correctness");
    require(doc.at("rectangles").as_array().size() == 5 && doc.at("generator").at("seed").as_int64() == 42,"Report reproducible input");
    require(doc.at("build").as_object().contains("revision"),"Build provenance");
    plan.run.render.mode = RenderMode::ParallelUnsafe;
    writer.write(plan,result);
    const auto unsafe = boost::json::parse(read(plan.reportPath));
    require(unsafe.at("median_speedup").is_null() && !unsafe.at("comparison_eligible").as_bool(),"Unsafe timings are not claimed as valid speedup");
    const auto previous = read(plan.reportPath);
    fails([&] { writer.write(plan,{}); });
    require(read(plan.reportPath) == previous,"Invalid result preserves old report");
    auto corrupted = result; corrupted.candidate[0].mismatchedPixels = 1;
    plan.run.render.mode = RenderMode::ParallelSafe;
    writer.write(plan,corrupted);
    require(boost::json::parse(read(plan.reportPath)).at("median_speedup").is_null(),"Corrupt result has no speedup");
    fails([&] { (void)load(R"({"input":"run.json","warmup":0,"repetitions":0,"report":"result.json"})"); });
    fails([&] { (void)load(R"({"input":"run.json","warmup":0,"repetitions":1,"report":"run.json"})"); });
    fails([&] { (void)load(R"({"input":"run.json","warmup":0,"repetitions":1,"report":"plan.json"})"); });
    fails([&] { (void)load(R"({"input":"run.json","warmup":0,"repetitions":1,"report":"result.json","typo":1})"); });
    fails([&] { (void)load(R"({"input":"run.json","warmup":0,"repetitions":1.5,"report":"result.json"})"); });
}
}
int main() {
    try { statisticsTests(); runnerTests(); generatorTests(); ioTests(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
