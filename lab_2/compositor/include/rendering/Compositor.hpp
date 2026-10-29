#pragma once

#include "model/Image.hpp"
#include "model/Scene.hpp"

namespace compositor {

class Compositor {
public:
    // Draws sequentially onto a caller-owned canvas without changing the scene.
    // For now the canvas must have no previous rectangle winners.
    void render(const Scene& scene, Image& canvas) const;
};

} // namespace compositor
