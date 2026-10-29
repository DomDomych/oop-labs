#include "rendering/Compositor.hpp"
#include "execution/SequentialExecutor.hpp"
#include "synchronization/NoLockPolicy.hpp"
#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

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

void checkRegionTraversal() {
    const Color background{12, 34, 56}, red{255, 0, 0}, blue{0, 0, 255};
    const Scene scene{{
        Rectangle{-2, -1, 6, 4, red, -10},
        Rectangle{2, 1, 5, 3, blue, 7},
        Rectangle{3, 2, 2, 6, red, 7},
        Rectangle{0, 3, 9, 2, blue, -20},
        Rectangle{7, 0, 2, 2, red, 999},
        Rectangle{0, 5, 2, 2, red, 999}}};

    // Independent per-pixel reference: no clipper, grid or production overlay rule.
    Image expected{7, 5, background};
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 7; ++x) {
            std::optional<std::size_t> winner;
            for (std::size_t i = 0; i < scene.rectangles().size(); ++i) {
                const auto& rectangle = scene.rectangles()[i];
                if (x < rectangle.x() || x >= rectangle.right() ||
                    y < rectangle.y() || y >= rectangle.bottom()) continue;
                if (!winner || rectangle.priority() >= scene.rectangles()[*winner].priority()) {
                    winner = i;
                }
            }
            if (winner) {
                const auto& rectangle = scene.rectangles()[*winner];
                expected.setPixel(x, y, PixelState{
                    rectangle.color(), RectangleMetadata{rectangle.priority(), *winner}});
            }
        }
    }
    const auto maximum = std::numeric_limits<std::size_t>::max();
    for (const auto rw : {std::size_t{1}, std::size_t{2}, std::size_t{3},
                          std::size_t{7}, std::size_t{10}, maximum}) {
        for (const auto rh : {std::size_t{1}, std::size_t{2}, std::size_t{3},
                              std::size_t{5}, std::size_t{8}, maximum}) {
            Image actual{7, 5, background};
            SequentialExecutor rendererExecutor;
    const NoLockPolicy rendererPolicy;
    const Compositor renderer{rw, rh, rendererExecutor, rendererPolicy};
            renderer.render(scene, actual);
            for (std::size_t y = 0; y < 5; ++y) {
                for (std::size_t x = 0; x < 7; ++x) {
                    require(actual.at(x, y) == expected.at(x, y),
                            "Region dimensions must not affect colors or metadata");
                }
            }
        }
    }
    SequentialExecutor validationExecutor;
    const NoLockPolicy validationPolicy;
    bool rejectedWidth = false, rejectedHeight = false;
    try { Compositor{0, 1, validationExecutor, validationPolicy}; } catch (const std::invalid_argument&) { rejectedWidth = true; }
    try { Compositor{1, 0, validationExecutor, validationPolicy}; } catch (const std::invalid_argument&) { rejectedHeight = true; }
    require(rejectedWidth && rejectedHeight, "Zero region dimensions must be rejected");
}

void checkComposition() {
    const Color background{255, 255, 255}, red{255, 0, 0}, blue{0, 0, 255};
    const Color green{0, 255, 0};
    SequentialExecutor compositorExecutor;
    const NoLockPolicy compositorPolicy;
    const Compositor compositor{2, 2, compositorExecutor, compositorPolicy};
    Image movedSource{1, 1, background};
    Image owner{std::move(movedSource)};
    bool rejectedEmptyScene = false, rejectedNonemptyScene = false;
    try { compositor.render(Scene{{}}, movedSource); }
    catch (const std::invalid_argument&) { rejectedEmptyScene = true; }
    try { compositor.render(Scene{{Rectangle{0, 0, 1, 1, red, 0}}}, movedSource); }
    catch (const std::invalid_argument&) { rejectedNonemptyScene = true; }
    require(rejectedEmptyScene && rejectedNonemptyScene, "Moved-from canvas must be rejected");
    require(movedSource.width() == 0 && movedSource.height() == 0,
            "Rejection must preserve the empty state");
    require(owner.at(0, 0) == PixelState{background}, "Moved storage must remain unchanged");
    movedSource.swap(owner);
    compositor.render(Scene{{Rectangle{0, 0, 1, 1, red, 0}}}, movedSource);
    require(movedSource.colorAt(0, 0) == red, "Restored canvas can be rendered again");

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

void checkRepeatedCalls() {
    const Color white{255, 255, 255}, red{255, 0, 0}, blue{0, 0, 255};
    const Color green{0, 255, 0};
    SequentialExecutor compositorExecutor;
    const NoLockPolicy compositorPolicy;
    const Compositor compositor{2, 2, compositorExecutor, compositorPolicy};
    Image canvas{4, 2, white};
    compositor.render(Scene{{Rectangle{0, 0, 4, 1, red, 100}}}, canvas);
    const auto oldPixel = canvas.at(3, 0);

    // Even INT_MIN beats the previous call's priority. Within this call,
    // a lower-priority object loses and a later equal-priority object wins.
    const int minimum = std::numeric_limits<int>::min();
    const Scene next{{
        Rectangle{0, 0, 3, 1, blue, minimum},
        Rectangle{1, 0, 2, 1, green, -5},
        Rectangle{1, 0, 2, 1, red, minimum},
        Rectangle{2, 0, 1, 1, blue, -5}}};
    compositor.render(next, canvas);
    require(canvas.at(0, 0) == PixelState{blue, RectangleMetadata{minimum, 0}},
            "Old priority must not affect a new call, even with the same index");
    require(canvas.at(1, 0) == PixelState{green, RectangleMetadata{-5, 1}},
            "Current call keeps the greater priority");
    require(canvas.at(2, 0) == PixelState{blue, RectangleMetadata{-5, 3}},
            "Current call breaks ties using input order");
    require(canvas.at(3, 0) == oldPixel, "Uncovered old pixel keeps metadata and color");
    for (std::size_t x = 0; x < 4; ++x) {
        require(canvas.at(x, 1) == PixelState{white}, "Uncovered background stays intact");
    }

    const Image previous = canvas;
    compositor.render(Scene{{}}, canvas);
    compositor.render(Scene{{Rectangle{4, 0, 1, 1, red, 999}}}, canvas);
    compositor.render(next, canvas);
    for (std::size_t y = 0; y < canvas.height(); ++y) {
        for (std::size_t x = 0; x < canvas.width(); ++x) {
            require(canvas.at(x, y) == previous.at(x, y),
                    "Empty, outside and repeated same-scene calls preserve the result");
        }
    }

    SequentialExecutor anotherCompositorExecutor;
    const NoLockPolicy anotherCompositorPolicy;
    const Compositor anotherCompositor{3, 1, anotherCompositorExecutor, anotherCompositorPolicy};
    anotherCompositor.render(Scene{{Rectangle{1, 0, 1, 1, red, -5}}}, canvas);
    require(canvas.at(1, 0) == PixelState{red, RectangleMetadata{-5, 0}},
            "A new call ignores old indices, independent of compositor identity");
    Image fresh{4, 2, white};
    compositor.render(next, fresh);
    require(fresh.at(3, 0) == PixelState{white}, "Call state must not leak between canvases");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::invalid_argument("Expected clipping, overlay, composition or repeated_calls");
        }
        const std::string_view component{argv[1]};
        if (component == "clipping") {
            checkClipping();
        } else if (component == "overlay") {
            checkOverlay();
        } else if (component == "composition") {
            checkComposition();
            checkRegionTraversal();
        } else if (component == "repeated_calls") {
            checkRepeatedCalls();
        } else {
            throw std::invalid_argument("Unknown test component");
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
