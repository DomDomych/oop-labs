#include "application/RenderApplication.hpp"
#include "application/RenderComponents.hpp"
#include "rendering/Compositor.hpp"

namespace compositor {

RenderApplication::RenderApplication(const IImageWriter& writer) : writer_(writer) {}

void RenderApplication::run(const RunConfig& config) const {
    auto components = createRenderComponents(config.render);
    Image canvas{config.canvas.width, config.canvas.height, config.canvas.background};
    const Compositor compositor{config.render.regionWidth, config.render.regionHeight,
                                *components.executor, *components.policy};
    compositor.render(config.scene, canvas);
    writer_.write(canvas, config.outputImage);
}

} // namespace compositor
