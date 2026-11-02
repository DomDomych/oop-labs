#include "benchmark/SceneGenerator.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>

namespace compositor::benchmark {
namespace {
constexpr std::size_t kMinimumGeneratedExtent = 1;
constexpr std::size_t kMinimumGridDimension = 1;
constexpr std::size_t kOverlapStartDivisor = 4;
constexpr std::size_t kOverlapSpreadDivisor = 8;
constexpr std::size_t kOverlapRectangleSizeDivisor = 2;
constexpr std::size_t kLargeRectangleStartDivisor = 4;
constexpr std::size_t kMixedLargeRectanglePeriod = 4;
constexpr std::size_t kMaximumMixedRectangleDimension = 16;
constexpr int kMinimumGeneratedPriority = -10;
constexpr int kMaximumGeneratedPriority = 10;
constexpr std::size_t kGeneratedPriorityCount =
    static_cast<std::size_t>(kMaximumGeneratedPriority - kMinimumGeneratedPriority + 1);
constexpr std::size_t kColorChannelChoiceCount = static_cast<std::size_t>(kMaxColorChannel) + 1;
}

Scene SceneGenerator::generate(std::size_t width, std::size_t height, const GeneratorConfig& config) const {
    if (width == 0 || height == 0 || width > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        height > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument("Generator canvas dimensions must fit positive int");
    if (config.scenario < Scenario::Sparse || config.scenario > Scenario::Mixed)
        throw std::invalid_argument("Unknown generated scenario");
    std::mt19937 random(config.seed);
    const auto choose = [&](std::size_t bound) { return static_cast<std::size_t>(random()) % bound; };
    std::vector<Rectangle> rectangles;
    if (config.scenario == Scenario::Sparse && config.rectangleCount / width + (config.rectangleCount % width != 0) > height)
        throw std::invalid_argument("Sparse rectangle count exceeds available pixels");
    std::size_t columns = kMinimumGridDimension;
    while (columns < width && columns < config.rectangleCount / columns) ++columns;
    columns = std::max(columns, config.rectangleCount / height + (config.rectangleCount % height != 0));
    const auto rows = std::max(kMinimumGridDimension,
        config.rectangleCount / columns + (config.rectangleCount % columns != 0));
    rectangles.reserve(config.rectangleCount);
    for (std::size_t i = 0; i < config.rectangleCount; ++i) {
        std::size_t x, y, w, h;
        if (config.scenario == Scenario::Sparse) {
            const auto cellX = i % columns;
            const auto cellY = i / columns;
            x = static_cast<std::uint64_t>(cellX) * width / columns;
            y = static_cast<std::uint64_t>(cellY) * height / rows;
            const auto right = static_cast<std::uint64_t>(cellX + 1) * width / columns;
            const auto bottom = static_cast<std::uint64_t>(cellY + 1) * height / rows;
            w = kMinimumGeneratedExtent + choose(right-x); h = kMinimumGeneratedExtent + choose(bottom-y);
        } else if (config.scenario == Scenario::Overlap) {
            x = width / kOverlapStartDivisor + choose(std::max<std::size_t>(1, width / kOverlapSpreadDivisor));
            y = height / kOverlapStartDivisor + choose(std::max<std::size_t>(1, height / kOverlapSpreadDivisor));
            w = std::max(kMinimumGeneratedExtent, width / kOverlapRectangleSizeDivisor);
            h = std::max(kMinimumGeneratedExtent, height / kOverlapRectangleSizeDivisor);
        } else if (config.scenario == Scenario::Large) {
            x = choose(std::max(kMinimumGeneratedExtent, width / kLargeRectangleStartDivisor));
            y = choose(std::max(kMinimumGeneratedExtent, height / kLargeRectangleStartDivisor));
            w = width - x; h = height - y;
        } else {
            x = choose(width); y = choose(height);
            w = i % kMixedLargeRectanglePeriod == 0
                ? width-x
                : kMinimumGeneratedExtent + choose(std::min(kMaximumMixedRectangleDimension, width-x));
            h = i % kMixedLargeRectanglePeriod == 0
                ? height-y
                : kMinimumGeneratedExtent + choose(std::min(kMaximumMixedRectangleDimension, height-y));
        }
        const Color color{static_cast<int>(choose(kColorChannelChoiceCount)), static_cast<int>(choose(kColorChannelChoiceCount)),
                          static_cast<int>(choose(kColorChannelChoiceCount))};
        const int priority = static_cast<int>(choose(kGeneratedPriorityCount)) + kMinimumGeneratedPriority;
        rectangles.emplace_back(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), color, priority);
    }
    return Scene{std::move(rectangles)};
}

} // namespace compositor::benchmark
