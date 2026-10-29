#pragma once

#include "model/Image.hpp"
#include "model/Scene.hpp"

namespace compositor {

class Compositor {
public:
    // Draws sequentially onto a caller-owned canvas without changing the scene.
    // Priorities and input indices apply only within this invocation.
    // Uncovered pixels keep their complete previous state; an empty scene is a no-op.
    // Calls accessing the same canvas must not overlap.
    // Scratch allocation happens before writes; allocation errors propagate.
    void render(const Scene& scene, Image& canvas) const;
};

} // namespace compositor
