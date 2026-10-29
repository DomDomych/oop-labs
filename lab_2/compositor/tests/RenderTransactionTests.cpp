#include "rendering/Compositor.hpp"

#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
#include <utility>

namespace {
// This executable is single-threaded. Injection is enabled only around render.
int allocationsUntilFailure = -1;
}

void* operator new(std::size_t size) {
    if (allocationsUntilFailure == 0) throw std::bad_alloc{};
    if (allocationsUntilFailure > 0) --allocationsUntilFailure;
    if (void* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc{};
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

namespace {
using namespace compositor;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void requireEqual(const Image& left, const Image& right) {
    require(left.width() == right.width() && left.height() == right.height(),
            "Dimensions must remain intact");
    for (std::size_t y = 0; y < left.height(); ++y) {
        for (std::size_t x = 0; x < left.width(); ++x) {
            require(left.at(x, y) == right.at(x, y), "Pixel color and metadata must remain intact");
        }
    }
}

void checkTransaction() {
    static_assert(noexcept(std::declval<Image&>().swap(std::declval<Image&>())));
    const Color white{255, 255, 255}, red{255, 0, 0}, blue{0, 0, 255};
    const Compositor compositor{2, 1};
    Image canvas{3, 2, white};
    canvas.setPixel(1, 0, PixelState{red, RectangleMetadata{100, 42}});
    const Image original = canvas;
    const Scene scene{{Rectangle{0, 0, 2, 2, blue, -1}}};
    bool completed = false;
    unsigned failures = 0;
    // Sweep allocation failures rather than depending on a particular allocation count.
    for (int allowed = 0; allowed < 32; ++allowed) {
        allocationsUntilFailure = allowed;
        try {
            compositor.render(scene, canvas);
            allocationsUntilFailure = -1;
            completed = true;
            break;
        } catch (const std::bad_alloc&) {
            allocationsUntilFailure = -1;
            ++failures;
            requireEqual(canvas, original);
        } catch (...) {
            allocationsUntilFailure = -1;
            throw;
        }
    }
    require(failures > 0 && completed, "Exercise both failed allocation and successful commit");
    for (std::size_t y = 0; y < 2; ++y) {
        for (std::size_t x = 0; x < 2; ++x) {
            require(canvas.at(x, y) == PixelState{blue, RectangleMetadata{-1, 0}},
                    "Successful call commits all covered pixels");
        }
        require(canvas.at(2, y) == original.at(2, y), "Untouched pixels survive commit");
    }

    const Image committed = canvas;
    const Scene empty{{}};
    allocationsUntilFailure = 0;
    try { compositor.render(empty, canvas); }
    catch (...) { allocationsUntilFailure = -1; throw; }
    allocationsUntilFailure = -1;
    requireEqual(canvas, committed);

    Image other{1, 1, red};
    canvas.swap(other);
    requireEqual(other, committed);
    require(canvas.width() == 1 && canvas.height() == 1 && canvas.at(0, 0) == PixelState{red},
            "Swap exchanges dimensions and full storage");
    canvas.swap(canvas);
    require(canvas.colorAt(0, 0) == red, "Self-swap remains valid");
}
} // namespace

int main() {
    try { checkTransaction(); return 0; }
    catch (const std::exception& error) {
        allocationsUntilFailure = -1;
        std::cerr << error.what() << '\n';
        return 1;
    }
}
