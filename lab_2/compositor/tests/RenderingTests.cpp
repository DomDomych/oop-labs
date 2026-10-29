#include "rendering/Compositor.hpp"
#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {
using namespace compositor;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void checkClipping() {
    const RectangleClipper clipper;
    const Color color{1, 2, 3};
    require(clipper.clip(Rectangle{1, 2, 3, 4, color, 0}, 10, 10) ==
                PixelBounds{1, 2, 4, 6}, "Interior half-open bounds");
    require(clipper.clip(Rectangle{-2, -1, 5, 4, color, 0}, 4, 4) ==
                PixelBounds{0, 0, 3, 3}, "Clip left and top");
    require(clipper.clip(Rectangle{2, 1, 8, 9, color, 0}, 4, 3) ==
                PixelBounds{2, 1, 4, 3}, "Clip right and bottom");
    require(clipper.clip(Rectangle{-2, -2, 10, 10, color, 0}, 4, 3) ==
                PixelBounds{0, 0, 4, 3}, "Rectangle surrounds canvas");
    for (const auto& rectangle : {
             Rectangle{-3, 0, 3, 1, color, 0}, Rectangle{0, -3, 1, 3, color, 0},
             Rectangle{4, 0, 1, 1, color, 0}, Rectangle{0, 3, 1, 1, color, 0}}) {
        require(!clipper.clip(rectangle, 4, 3), "Touching boundary is not coverage");
    }
    const Rectangle unit{0, 0, 1, 1, color, 0};
    require(!clipper.clip(unit, 0, 3) && !clipper.clip(unit, 4, 0), "Empty canvas");
    const int maximum = std::numeric_limits<int>::max();
    const Rectangle large{maximum, maximum, maximum, maximum, color, 0};
    require(!clipper.clip(large, 4, 3), "Large positive coordinates");
    require(!clipper.clip(Rectangle{std::numeric_limits<int>::min(), 0,
                                   maximum, 1, color, 0}, 4, 3),
            "Large negative coordinates");
    const std::size_t edge = static_cast<std::size_t>(maximum) + 1;
    require(clipper.clip(large, edge, edge) ==
                PixelBounds{edge - 1, edge - 1, edge, edge},
            "Canvas dimensions beyond int range");
}

void checkOverlay() {
    const Color color{1, 2, 3};
    require(shouldReplace(PixelState{color}, {std::numeric_limits<int>::min(), 0}),
            "Background loses to negative priority");
    const PixelState current{color, RectangleMetadata{5, 3}};
    require(shouldReplace(current, {6, 0}), "Higher priority wins");
    require(!shouldReplace(current, {4, 100}), "Lower priority loses");
    require(shouldReplace(current, {5, 4}), "Later index wins tie");
    require(!shouldReplace(current, {5, 2}), "Earlier index loses tie");
    require(!shouldReplace(current, {5, 3}), "Identical candidate needs no update");
}

void checkComposition() {
    const Color background{255, 255, 255}, red{255, 0, 0}, blue{0, 0, 255};
    const Color green{0, 255, 0};
    const Compositor compositor;
    const Scene empty{{}};
    Image blank{2, 2, background};
    compositor.render(empty, blank);
    for (std::size_t y = 0; y < 2; ++y) {
        for (std::size_t x = 0; x < 2; ++x) {
            require(blank.at(x, y) == PixelState{background}, "Empty scene background");
        }
    }

    // Expected winner indices, calculated by hand:
    // 0 1 1 .
    // 0 2 2 .
    // 3 2 2 .
    const Scene scene{{
        Rectangle{-1, 0, 4, 2, red, -5},
        Rectangle{1, -1, 2, 3, blue, 7},
        Rectangle{1, 1, 2, 5, green, 7},
        Rectangle{0, 0, 3, 3, red, -10},
        Rectangle{4, 0, 1, 3, red, 100},
        Rectangle{-4, -4, 1, 1, red, 100}}};
    const int expected[3][4] = {{0, 1, 1, -1}, {0, 2, 2, -1}, {3, 2, 2, -1}};
    const auto original = scene.rectangles();
    Image image{4, 3, background};
    Image repeated{4, 3, background};
    compositor.render(scene, image);
    compositor.render(scene, repeated);
    require(image.width() == 4 && image.height() == 3, "Result dimensions");
    require(scene.rectangles() == original, "Scene must not change");
    Image small{1, 1, blue};
    compositor.render(scene, small);
    require(small.at(0, 0) == PixelState{red, RectangleMetadata{-5, 0}},
            "Same scene uses the supplied canvas dimensions");
    Image wide{5, 3, blue};
    compositor.render(scene, wide);
    require(wide.colorAt(3, 0) == blue && !wide.at(3, 0).winner,
            "Initial color belongs to the canvas");
    for (std::size_t y = 0; y < 3; ++y) {
        for (std::size_t x = 0; x < 4; ++x) {
            const int index = expected[y][x];
            if (index >= 0) {
                const auto id = static_cast<std::size_t>(index);
                const auto& rectangle = original[id];
                const PixelState wanted{rectangle.color(), RectangleMetadata{rectangle.priority(), id}};
                require(image.at(x, y) == wanted, "Expected color and winner metadata");
            } else {
                require(image.colorAt(x, y) == background && !image.at(x, y).winner,
                        "Uncovered pixel stays background");
            }
            require(repeated.at(x, y) == image.at(x, y), "Repeatability");
        }
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::invalid_argument("Expected clipping, overlay or composition");
        }
        const std::string_view component{argv[1]};
        if (component == "clipping") {
            checkClipping();
        } else if (component == "overlay") {
            checkOverlay();
        } else if (component == "composition") {
            checkComposition();
        } else {
            throw std::invalid_argument("Unknown test component");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
