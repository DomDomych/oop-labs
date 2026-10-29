#pragma once

#include "model/Image.hpp"
#include "model/Scene.hpp"

#include <cstddef>

namespace compositor {

class Compositor {
public:
    // Both region dimensions must be positive; no hardware-specific defaults.
    Compositor(std::size_t regionWidth, std::size_t regionHeight);

    // Draws sequentially onto a caller-owned canvas without changing the scene.
    // An empty moved-from canvas is rejected with invalid_argument, even for an empty scene.
    // Priorities and input indices apply only within this invocation.
    // Uncovered pixels keep their complete previous state; an empty scene is a no-op.
    // Calls accessing the same canvas must not overlap.
    // Strong exception guarantee: any exception leaves the canvas unchanged.
    // Successful rendering replaces storage and invalidates references to pixels.
    void render(const Scene& scene, Image& canvas) const;

private:
    std::size_t regionWidth_;
    std::size_t regionHeight_;
};

} // namespace compositor
