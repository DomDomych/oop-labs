#include "application/RenderApplication.hpp"
#include "output/PpmWriter.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace compositor;
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template <typename F> void fails(F operation) {
    try { operation(); } catch (const std::exception&) { return; }
    throw std::runtime_error("Expected failure");
}
class TemporaryDirectory {
public:
    std::filesystem::path path;
    TemporaryDirectory() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int i = 0; i < 100; ++i) {
            path = std::filesystem::temp_directory_path() / ("compositor-output-" + std::to_string(stamp) + "-" + std::to_string(i));
            if (std::filesystem::create_directory(path)) return;
        }
        throw std::runtime_error("Cannot reserve test directory");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};
std::string read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    require(bool(file), "Read result");
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}
void outputTests() {
    TemporaryDirectory directory;
    const auto path = directory.path / "result.ppm";
    Image image{2, 2, Color{255,0,128}};
    image.setPixel(1, 0, PixelState{Color{0,10,13}});
    const PpmWriter writer;
    writer.write(image, path);
    std::string expected = "P6\n2 2\n255\n";
    for (unsigned char byte : {255,0,128,0,10,13,255,0,128,255,0,128}) expected.push_back(static_cast<char>(byte));
    require(read(path) == expected, "Exact binary P6 bytes and row order");
    require(image.colorAt(1,0) == Color{0,10,13}, "Image unchanged");
    const Image large{4100,1,Color{1,2,3}};
    writer.write(large, path);
    const auto bytes = read(path);
    const std::string header = "P6\n4100 1\n255\n";
    require(bytes.substr(0,header.size()) == header && bytes.size() == header.size()+12300, "Buffer boundary and replacement");
    for (std::size_t i = header.size(); i < bytes.size(); ++i) {
        require(static_cast<unsigned char>(bytes[i]) == (i-header.size())%3+1, "Buffered RGB payload");
    }
    Image moved{1,1,Color{0,0,0}};
    Image holder{std::move(moved)};
    fails([&] { writer.write(moved,path); });
    require(read(path) == bytes, "Previous result preserved on error");
    fails([&] { writer.write(image,directory.path / "missing" / "result.ppm"); });
    require(!std::filesystem::exists(directory.path / "missing"), "No automatic parent creation");
    const auto blocked = directory.path / "directory.ppm";
    std::filesystem::create_directory(blocked);
    { std::ofstream marker(blocked / "keep"); marker << "keep"; }
    fails([&] { writer.write(image,blocked); });
    require(read(blocked / "keep") == "keep", "Destination preserved after rename failure");
    for (const auto& entry : std::filesystem::directory_iterator(directory.path)) {
        require(entry.path() == path || entry.path() == blocked, "Staging cleaned up after success and failure");
    }
}

class InspectingWriter final : public IImageWriter {
public:
    mutable unsigned calls = 0;
    bool throwOnWrite = false;
    void write(const Image& image, const std::filesystem::path& path) const override {
        ++calls;
        require(path == "result.ppm", "Output path forwarded");
        require(image.width() == 3 && image.height() == 1, "Canvas dimensions");
        require(image.colorAt(0,0) == Color{255,0,0}, "First rectangle");
        require(image.colorAt(1,0) == Color{0,0,255}, "Second rectangle");
        require(image.colorAt(2,0) == Color{255,255,255}, "Background");
        if (throwOnWrite) throw std::runtime_error("Injected writer failure");
    }
};
void applicationTests() {
    // Disjoint objects keep the unsafe-mode test free of intentional data races.
    RunConfig config{{3,1,Color{255,255,255}}, {RenderMode::Sequential,2,2,1},
                     Scene{{Rectangle{0,0,1,1,Color{255,0,0},1}, Rectangle{1,0,1,1,Color{0,0,255},2}}}, "result.ppm"};
    InspectingWriter writer;
    const RenderApplication app{writer};
    for (auto mode : {RenderMode::Sequential,RenderMode::ParallelSafe,RenderMode::ParallelUnsafe}) {
        config.render.mode = mode;
        app.run(config);
    }
    require(writer.calls == 3, "One write per successful run");
    config.render.mode = RenderMode::ParallelSafe;
    writer.throwOnWrite = true;
    fails([&] { app.run(config); });
    require(writer.calls == 4, "Writer exception propagated");
    config.render.mode = static_cast<RenderMode>(99);
    fails([&] { app.run(config); });
    require(writer.calls == 4, "Invalid mode must not save");
}
} // namespace
int main() {
    try { outputTests(); applicationTests(); }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
