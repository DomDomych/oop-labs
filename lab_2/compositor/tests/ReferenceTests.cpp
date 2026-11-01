#include "application/RenderComponents.hpp"
#include "input/JsonConfigLoader.hpp"
#include "rendering/Compositor.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    try {
        using namespace compositor;
        if (argc != 2) throw std::runtime_error("Reference directory is required");
        const std::filesystem::path root{argv[1]};
        std::ifstream file(root / "expected-pixels.txt");
        std::size_t width, height;
        if (!(file >> width >> height) || width != 4 || height != 3)
            throw std::runtime_error("Invalid reference dimensions");
        std::vector<PixelState> expected;
        for (std::size_t i = 0; i < width * height; ++i) {
            int r, g, b, priority, index;
            if (!(file >> r >> g >> b >> priority >> index) || index < -1)
                throw std::runtime_error("Invalid reference pixel");
            PixelState pixel{Color{r,g,b}};
            if (index >= 0) pixel.winner = RectangleMetadata{priority,static_cast<std::size_t>(index)};
            expected.push_back(pixel);
        }
        std::string extra;
        if (file >> extra) throw std::runtime_error("Unexpected reference data");
        for (const auto* name : {"sequential.json", "parallel-safe.json"}) {
            const auto config = JsonConfigLoader{}.load(root / name);
            if (config.canvas.width != width || config.canvas.height != height)
                throw std::runtime_error("Config/reference size mismatch");
            auto components = createRenderComponents(config.render);
            const Compositor compositor{config.render.regionWidth,config.render.regionHeight,
                                        *components.executor,*components.policy};
            for (unsigned repeat = 0; repeat < 5; ++repeat) {
                Image canvas{width,height,config.canvas.background};
                compositor.render(config.scene,canvas);
                for (std::size_t y=0; y<height; ++y)
                    for (std::size_t x=0; x<width; ++x)
                        if (canvas.at(x,y) != expected[y*width+x])
                            throw std::runtime_error(std::string(name) + ": unexpected PixelState at (" +
                                                     std::to_string(x) + "," + std::to_string(y) + ")");
            }
        }
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
