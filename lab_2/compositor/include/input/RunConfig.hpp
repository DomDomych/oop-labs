#pragma once

#include "model/Color.hpp"
#include "model/Scene.hpp"

#include <cstddef>
#include <filesystem>

namespace compositor {

enum class RenderMode { Sequential, ParallelUnsafe, ParallelSafe };

struct CanvasConfig {
    std::size_t width;
    std::size_t height;
    Color background;
};

struct RenderConfig {
    RenderMode mode;
    std::size_t threads;
    std::size_t regionWidth;
    std::size_t regionHeight;
};

// Input data only: no executors, synchronization state or allocated canvas.
struct RunConfig {
    CanvasConfig canvas;
    RenderConfig render;
    Scene scene;
    std::filesystem::path outputImage;
};

} // namespace compositor
