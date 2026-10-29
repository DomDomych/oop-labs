#include "rendering/RegionGrid.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace compositor;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Exception, typename Function>
void requireThrows(Function function) {
    try { function(); } catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception was not thrown");
}

void checkGrid() {
    const RegionGrid grid{7, 5, 3, 2};
    require(grid.columns() == 3 && grid.rows() == 3 && grid.count() == 9,
            "Grid dimensions");
    require(grid.bounds(0) == PixelBounds{0, 0, 3, 2}, "First region");
    require(grid.bounds(8) == PixelBounds{6, 4, 7, 5}, "Clipped corner");
    require(grid.regionAt(3, 2) == 4, "Boundary belongs to next region");
    require(RegionGrid{2, 1, 100, 100}.bounds(0) == PixelBounds{0, 0, 2, 1},
            "Region larger than canvas");
    require(RegionGrid{6, 4, 3, 2}.count() == 4, "Exact divisibility");

    for (std::size_t rw = 1; rw <= 9; ++rw) {
        for (std::size_t rh = 1; rh <= 7; ++rh) {
            const RegionGrid candidate{7, 5, rw, rh};
            std::vector<unsigned> visits(35, 0);
            for (std::size_t id = 0; id < candidate.count(); ++id) {
                const auto bounds = candidate.bounds(id);
                require(bounds.left < bounds.right && bounds.top < bounds.bottom,
                        "Nonempty regions");
                require(bounds.right <= 7 && bounds.bottom <= 5, "Within canvas");
                for (auto y = bounds.top; y < bounds.bottom; ++y) {
                    for (auto x = bounds.left; x < bounds.right; ++x) {
                        require(candidate.regionAt(x, y) == id, "Consistent region ID");
                        ++visits[y * 7 + x];
                    }
                }
            }
            for (auto visitsToPixel : visits) {
                require(visitsToPixel == 1, "Every pixel belongs to exactly one region");
            }
        }
    }
    requireThrows<std::invalid_argument>([] { RegionGrid{0, 1, 1, 1}; });
    requireThrows<std::invalid_argument>([] { RegionGrid{1, 0, 1, 1}; });
    requireThrows<std::invalid_argument>([] { RegionGrid{1, 1, 0, 1}; });
    requireThrows<std::invalid_argument>([] { RegionGrid{1, 1, 1, 0}; });
    requireThrows<std::out_of_range>([&] { grid.bounds(9); });
    requireThrows<std::out_of_range>([&] { grid.regionAt(7, 0); });
    requireThrows<std::out_of_range>([&] { grid.regionAt(0, 5); });
    const auto maximum = std::numeric_limits<std::size_t>::max();
    requireThrows<std::length_error>([&] { RegionGrid{maximum, 2, 1, 1}; });
    const RegionGrid enormous{maximum, maximum, maximum - 1, maximum - 1};
    require(enormous.bounds(3) == PixelBounds{maximum - 1, maximum - 1, maximum, maximum},
            "Edge calculations must not overflow");
}
} // namespace

int main() {
    try { checkGrid(); return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
