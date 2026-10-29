#include "input/JsonConfigLoader.hpp"

#include "model/PixelState.hpp"

#include <boost/json.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace compositor {
namespace {
namespace json = boost::json;

[[noreturn]] void fail(const std::string& field, const std::string& reason) {
    throw std::runtime_error(field + ": " + reason);
}

void validateObject(const json::value& value, std::string field,
                           std::initializer_list<std::string_view> keys) {
    if (!value.is_object()) fail(field, "expected an object");
    const auto& result = value.as_object();
    for (const auto key : keys) {
        if (!result.contains(key)) fail(field + "." + std::string(key), "missing required field");
    }
    for (const auto& item : result) {
        const std::string_view key{item.key().data(), item.key().size()};
        if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
            fail(field + "." + std::string(key), "unknown field");
        }
    }
}

std::int64_t integer(const json::value& value, const std::string& field,
                     std::int64_t low, std::int64_t high) {
    std::int64_t number;
    if (value.is_int64()) number = value.as_int64();
    else if (value.is_uint64() && value.as_uint64() <= static_cast<std::uint64_t>(high)) {
        number = static_cast<std::int64_t>(value.as_uint64());
    } else fail(field, "expected an integer in range [" + std::to_string(low) + ", " + std::to_string(high) + "]");
    if (number < low || number > high) fail(field, "integer is outside the allowed range");
    return number;
}

std::size_t positive(const json::value& value, const std::string& field) {
    constexpr auto max = std::min<std::uintmax_t>(std::numeric_limits<std::size_t>::max(),
                                                 std::numeric_limits<std::int64_t>::max());
    return static_cast<std::size_t>(integer(value, field, 1, static_cast<std::int64_t>(max)));
}

int component(const json::value& value, const std::string& field, int low = std::numeric_limits<int>::min(),
              int high = std::numeric_limits<int>::max()) {
    return static_cast<int>(integer(value, field, low, high));
}

Color color(const json::value& value, const std::string& field) {
    if (!value.is_array() || value.as_array().size() != 3) fail(field, "expected three RGB integers");
    const auto& rgb = value.as_array();
    return Color{component(rgb[0], field + "[0]", 0, 255),
                 component(rgb[1], field + "[1]", 0, 255),
                 component(rgb[2], field + "[2]", 0, 255)};
}

std::string string(const json::value& value, const std::string& field) {
    if (!value.is_string()) fail(field, "expected a string");
    return std::string(value.as_string());
}

RunConfig decode(const json::value& value, const std::filesystem::path& directory) {
    validateObject(value, "$", {"canvas", "render", "rectangles", "output"});
    const auto& root = value.as_object();
    validateObject(root.at("canvas"), "canvas", {"width", "height", "background"});
    const auto& canvas = root.at("canvas").as_object();
    CanvasConfig canvasConfig{positive(canvas.at("width"), "canvas.width"),
                              positive(canvas.at("height"), "canvas.height"),
                              color(canvas.at("background"), "canvas.background")};
    if (canvasConfig.width > std::vector<PixelState>{}.max_size() / canvasConfig.height) {
        fail("canvas", "dimensions exceed image storage capacity");
    }
    validateObject(root.at("render"), "render", {"mode", "threads", "region"});
    const auto& render = root.at("render").as_object();
    const auto modeName = string(render.at("mode"), "render.mode");
    RenderMode mode;
    if (modeName == "sequential") mode = RenderMode::Sequential;
    else if (modeName == "parallel-unsafe") mode = RenderMode::ParallelUnsafe;
    else if (modeName == "parallel-safe") mode = RenderMode::ParallelSafe;
    else fail("render.mode", "expected sequential, parallel-unsafe or parallel-safe");
    validateObject(render.at("region"), "render.region", {"width", "height"});
    const auto& region = render.at("region").as_object();
    RenderConfig renderConfig{mode, positive(render.at("threads"), "render.threads"),
                              positive(region.at("width"), "render.region.width"),
                              positive(region.at("height"), "render.region.height")};
    const auto& entries = root.at("rectangles");
    if (!entries.is_array()) fail("rectangles", "expected an array");
    std::vector<Rectangle> rectangles;
    rectangles.reserve(entries.as_array().size());
    for (std::size_t i = 0; i < entries.as_array().size(); ++i) {
        const auto field = "rectangles[" + std::to_string(i) + "]";
        validateObject(entries.as_array()[i], field, {"x", "y", "width", "height", "color", "priority"});
        const auto& rect = entries.as_array()[i].as_object();
        rectangles.emplace_back(component(rect.at("x"), field + ".x"),
                                component(rect.at("y"), field + ".y"),
                                component(rect.at("width"), field + ".width", 1),
                                component(rect.at("height"), field + ".height", 1),
                                color(rect.at("color"), field + ".color"),
                                component(rect.at("priority"), field + ".priority"));
    }
    validateObject(root.at("output"), "output", {"image"});
    const auto& output = root.at("output").as_object();
    const auto outputName = string(output.at("image"), "output.image");
    if (outputName.empty() || outputName.find('\0') != std::string::npos) {
        fail("output.image", "expected a nonempty path without NUL characters");
    }
    auto outputPath = (directory / std::filesystem::path(outputName)).lexically_normal();
    return {canvasConfig, renderConfig, Scene{std::move(rectangles)}, std::move(outputPath)};
}
} // namespace

RunConfig JsonConfigLoader::load(const std::filesystem::path& path) const {
    try {
        const auto absolutePath = std::filesystem::absolute(path);
        std::ifstream input(absolutePath, std::ios::binary);
        if (!input) fail("file", "cannot open for reading");
        const std::string contents{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
        if (input.bad()) fail("file", "read failed");
        boost::system::error_code error;
        auto value = json::parse(contents, error);
        if (error) fail("JSON", error.message());
        return decode(value, absolutePath.parent_path());
    } catch (const std::runtime_error& error) {
        throw std::runtime_error(path.string() + ": " + error.what());
    }
}

} // namespace compositor
