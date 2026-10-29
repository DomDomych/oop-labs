#include "input/JsonConfigLoader.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace compositor;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class TemporaryDirectory {
public:
    std::filesystem::path path;
    TemporaryDirectory() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int i = 0; i < 100; ++i) {
            path = std::filesystem::temp_directory_path() /
                   ("compositor-input-" + std::to_string(stamp) + "-" + std::to_string(i));
            if (std::filesystem::create_directory(path)) return;
        }
        throw std::runtime_error("Cannot create test directory");
    }
    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

const std::string valid = R"({
 "canvas":{"width":8,"height":6,"background":[255,0,17]},
 "render":{"mode":"parallel-safe","threads":4,"region":{"width":3,"height":2}},
 "rectangles":[
   {"x":-2,"y":1,"width":3,"height":4,"color":[1,2,3],"priority":-5},
   {"x":0,"y":0,"width":1,"height":1,"color":[4,5,6],"priority":-5}
 ],
 "output":{"image":"nested/../result.ppm"}
})";

std::string replace(std::string text, const std::string& from, const std::string& to) {
    const auto position = text.find(from);
    require(position != std::string::npos, "Test replacement not found");
    text.replace(position, from.size(), to);
    return text;
}

void run(const std::filesystem::path& example) {
    TemporaryDirectory directory;
    const auto file = directory.path / "config.json";
    const JsonConfigLoader loader;
    auto write = [&](const std::string& text) {
        std::ofstream out(file);
        out << text;
        out.close();
        require(bool(out), "Write test fixture");
    };
    auto rejected = [&](const std::string& text, const std::string& context) {
        write(text);
        try { (void)loader.load(file); }
        catch (const std::runtime_error& error) {
            require(std::string(error.what()).find(context) != std::string::npos, "Error field context");
            require(std::string(error.what()).find(file.string()) != std::string::npos, "Error filename context");
            return;
        }
        throw std::runtime_error("Invalid configuration accepted");
    };
    write(valid);
    const auto config = loader.load(file);
    require(config.canvas.width == 8 && config.canvas.height == 6 &&
            config.canvas.background == Color{255,0,17}, "Canvas values");
    require(config.render.mode == RenderMode::ParallelSafe && config.render.threads == 4 &&
            config.render.regionWidth == 3 && config.render.regionHeight == 2, "Render values");
    require(config.scene.rectangles().size() == 2 &&
            config.scene.rectangles()[0] == Rectangle{-2,1,3,4,Color{1,2,3},-5} &&
            config.scene.rectangles()[1].color() == Color{4,5,6}, "Rectangle order and values");
    require(config.outputImage == directory.path / "result.ppm", "Path relative to configuration");
    require(!std::filesystem::exists(config.outputImage), "Loading must not create output");
    for (const auto& mode : {std::string("sequential"), std::string("parallel-unsafe")}) {
        write(replace(valid, "parallel-safe", mode));
        require(loader.load(file).render.mode == (mode == "sequential" ? RenderMode::Sequential : RenderMode::ParallelUnsafe), "Other modes");
    }
    auto empty = valid;
    const auto start = empty.find("\"rectangles\":[") + std::string("\"rectangles\":[").size();
    empty.erase(start, empty.find("\n ]", start) - start);
    write(empty);
    require(loader.load(file).scene.rectangles().empty(), "Empty scene allowed");
    rejected("{", "JSON");
    rejected(valid + " garbage", "JSON");
    rejected(replace(valid, "\"width\":8", "\"width\":0"), "canvas.width");
    rejected(replace(valid, "\"width\":8", "\"width\":8.0"), "canvas.width");
    rejected(replace(valid, "\"width\":8", "\"width\":true"), "canvas.width");
    rejected(replace(valid, "\"width\":8", "\"width\":9223372036854775808"), "canvas.width");
    rejected(replace(valid, "\"width\":8", "\"width\":9223372036854775807"), "canvas");
    rejected(replace(valid, "\"height\":6,", ""), "canvas.height");
    rejected(replace(valid, "\"height\":6", "\"height\":6,\"typo\":1"), "canvas.typo");
    rejected(replace(valid, "[255,0,17]", "[256,0,17]"), "canvas.background[0]");
    rejected(replace(valid, "[255,0,17]", "[0,0]"), "canvas.background");
    rejected(replace(valid, "parallel-safe", "typo"), "render.mode");
    rejected(replace(valid, "\"threads\":4", "\"threads\":0"), "render.threads");
    rejected(replace(valid, "\"width\":3,\"height\":2", "\"width\":-1,\"height\":2"), "render.region.width");
    rejected(replace(valid, "\"x\":-2", "\"x\":2147483648"), "rectangles[0].x");
    rejected(replace(valid, "\"width\":3,\"height\":4", "\"width\":0,\"height\":4"), "rectangles[0].width");
    rejected(replace(valid, "\"nested/../result.ppm\"", "\"\""), "output.image");
    rejected(replace(valid, "\"nested/../result.ppm\"", "\"a\\u0000b\""), "output.image");
    std::filesystem::remove(file);
    bool missing = false;
    try { (void)loader.load(file); } catch (const std::runtime_error&) { missing = true; }
    require(missing, "Missing input file");
    require(loader.load(example).scene.rectangles().size() == 2, "Repository example loads");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2, "Example configuration path is required");
        run(argv[1]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
