#include "benchmark/BenchmarkPlan.hpp"
#include "input/JsonConfigLoader.hpp"
#include <boost/json.hpp>
#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace compositor::benchmark {
namespace {
void schema(const boost::json::value& value, std::initializer_list<std::string_view> required,
            std::initializer_list<std::string_view> optional = {}) {
    if (!value.is_object()) throw std::runtime_error("Expected benchmark object");
    for (auto key : required) if (!value.as_object().contains(key))
        throw std::runtime_error("Missing benchmark field: " + std::string(key));
    for (const auto& pair : value.as_object()) {
        const std::string_view key{pair.key().data(),pair.key().size()};
        if (std::find(required.begin(),required.end(),key) == required.end() &&
            std::find(optional.begin(),optional.end(),key) == optional.end())
            throw std::runtime_error("Unknown benchmark field: " + std::string(key));
    }
}
std::size_t integer(const boost::json::value& value, const std::string& field, std::uint64_t min, std::uint64_t max) {
    if (!value.is_int64() || value.as_int64() < 0 || static_cast<std::uint64_t>(value.as_int64()) < min ||
        static_cast<std::uint64_t>(value.as_int64()) > max)
        throw std::runtime_error(field + ": expected integer in [" + std::to_string(min) + "," + std::to_string(max) + "]");
    return static_cast<std::size_t>(value.as_int64());
}
std::string text(const boost::json::value& value, const std::string& field) {
    if (!value.is_string() || value.as_string().empty()) throw std::runtime_error(field + ": expected nonempty string");
    std::string result(value.as_string());
    if (result.find('\0') != std::string::npos) throw std::runtime_error(field + ": NUL is forbidden");
    return result;
}
bool sameFile(const std::filesystem::path& a, const std::filesystem::path& b) {
    std::error_code error;
    return a.lexically_normal() == b.lexically_normal() || std::filesystem::equivalent(a,b,error);
}
}

BenchmarkPlan BenchmarkPlanLoader::load(const std::filesystem::path& path) const {
    try {
        const auto absolute = std::filesystem::absolute(path).lexically_normal();
        std::ifstream input(absolute, std::ios::binary);
        if (!input) throw std::runtime_error("Cannot read benchmark plan");
        const std::string contents{std::istreambuf_iterator<char>{input},std::istreambuf_iterator<char>{}};
        if (input.bad()) throw std::runtime_error("Benchmark plan read failed");
        const auto value = boost::json::parse(contents);
        schema(value, {"input", "warmup", "repetitions", "report"}, {"generate"});
        const auto& root = value.as_object();
        const auto inputPath = (absolute.parent_path() / text(root.at("input"),"input")).lexically_normal();
        const auto reportPath = (absolute.parent_path() / text(root.at("report"),"report")).lexically_normal();
        if (reportPath.filename().empty() || sameFile(reportPath,inputPath) || sameFile(reportPath,absolute))
            throw std::runtime_error("Report must name a file different from input configurations");
        BenchmarkOptions options{integer(root.at("warmup"),"warmup",0,100000),
                                 integer(root.at("repetitions"),"repetitions",1,100000)};
        auto run = JsonConfigLoader{}.load(inputPath);
        std::optional<GeneratorConfig> generator;
        if (const auto* generated = root.if_contains("generate")) {
            schema(*generated, {"scenario", "rectangles", "seed"});
            const auto& settings = generated->as_object();
            const auto name = text(settings.at("scenario"),"generate.scenario");
            Scenario scenario;
            if (name == "sparse") scenario = Scenario::Sparse;
            else if (name == "overlap") scenario = Scenario::Overlap;
            else if (name == "large") scenario = Scenario::Large;
            else if (name == "mixed") scenario = Scenario::Mixed;
            else throw std::runtime_error("Unknown generated scenario: " + name);
            generator = GeneratorConfig{scenario, integer(settings.at("rectangles"),"generate.rectangles",0,10000000),
                static_cast<std::uint32_t>(integer(settings.at("seed"),"generate.seed",0,std::numeric_limits<std::uint32_t>::max()))};
            run.scene = SceneGenerator{}.generate(run.canvas.width,run.canvas.height,*generator);
        }
        return {std::move(run),options,inputPath,reportPath,generator};
    } catch (const std::exception& error) {
        throw std::runtime_error(path.string() + ": " + error.what());
    }
}

} // namespace compositor::benchmark
