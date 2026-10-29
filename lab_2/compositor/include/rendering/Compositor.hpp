#pragma once

#include "model/Image.hpp"
#include "model/Scene.hpp"
#include "execution/IExecutor.hpp"
#include "synchronization/IRegionAccessPolicy.hpp"

#include <cstddef>

namespace compositor {

class Compositor {
public:
    // Both region dimensions must be positive; no hardware-specific defaults.
    Compositor(std::size_t regionWidth, std::size_t regionHeight,
               IExecutor& executor, const IRegionAccessPolicy& accessPolicy);

    // Dependencies are borrowed and must outlive this object.
    // Draws onto a caller-owned canvas without changing the scene.
    // An empty moved-from canvas is rejected with invalid_argument, even for an empty scene.
    // Priorities and input indices apply only within this invocation.
    // Uncovered pixels keep their complete previous state; an empty scene is a no-op.
    // Calls accessing the same canvas must not overlap.
    // Strong exception guarantee for data-race-free execution.
    // Parallel execution without region protection can cause UB; no guarantees apply.
    // Successful rendering replaces storage and invalidates references to pixels.
    void render(const Scene& scene, Image& canvas) const;

private:
    IExecutor& executor_;
    const IRegionAccessPolicy& accessPolicy_;
    std::size_t regionWidth_;
    std::size_t regionHeight_;
};

} // namespace compositor
