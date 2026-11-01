#include "benchmark/SceneGenerator.hpp"

#include <algorithm>
#include <limits>
#include <random>
#include <stdexcept>

namespace compositor::benchmark {

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
    std::size_t columns = 1;
    while (columns < width && columns < config.rectangleCount / columns) ++columns;
    columns = std::max(columns, config.rectangleCount / height + (config.rectangleCount % height != 0));
    const auto rows = std::max<std::size_t>(1, config.rectangleCount / columns + (config.rectangleCount % columns != 0));
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
            w = 1 + choose(right-x); h = 1 + choose(bottom-y);
        } else if (config.scenario == Scenario::Overlap) {
            x = width / 4 + choose(std::max<std::size_t>(1, width / 8));
            y = height / 4 + choose(std::max<std::size_t>(1, height / 8));
            w = std::max<std::size_t>(1, width / 2); h = std::max<std::size_t>(1, height / 2);
        } else if (config.scenario == Scenario::Large) {
            x = choose(std::max<std::size_t>(1, width / 4));
            y = choose(std::max<std::size_t>(1, height / 4));
            w = width - x; h = height - y;
        } else {
            x = choose(width); y = choose(height);
            w = i % 4 == 0 ? width-x : 1+choose(std::min<std::size_t>(16, width-x));
            h = i % 4 == 0 ? height-y : 1+choose(std::min<std::size_t>(16, height-y));
        }
        const Color color{static_cast<int>(choose(256)), static_cast<int>(choose(256)), static_cast<int>(choose(256))};
        const int priority = static_cast<int>(choose(21)) - 10;
        rectangles.emplace_back(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), color, priority);
    }
    return Scene{std::move(rectangles)};
}

} // namespace compositor::benchmark
