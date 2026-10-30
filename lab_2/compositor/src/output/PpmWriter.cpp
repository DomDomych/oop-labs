#include "output/PpmWriter.hpp"

#include <array>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>

namespace compositor {
namespace {

// Reserve a private staging directory atomically, on the destination filesystem.
// This avoids truncating someone else's temporary file on a name collision.
class StagingDirectory {
public:
    explicit StagingDirectory(const std::filesystem::path& parent) {
        std::random_device random;
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            path_ = parent / (".compositor-" + std::to_string(random()) + "-" + std::to_string(random()));
            if (std::filesystem::create_directory(path_)) return;
        }
        throw std::runtime_error("Cannot reserve temporary output directory");
    }
    ~StagingDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    StagingDirectory(const StagingDirectory&) = delete;
    StagingDirectory& operator=(const StagingDirectory&) = delete;
    std::filesystem::path file() const { return path_ / "image.ppm"; }

private:
    std::filesystem::path path_;
};

} // namespace

void PpmWriter::write(const Image& image, const std::filesystem::path& path) const {
    if (image.width() == 0 || image.height() == 0) {
        throw std::invalid_argument("Cannot save an empty moved-from image");
    }
    if (path.empty() || path.filename().empty() || path.native().find('\0') != std::filesystem::path::string_type::npos) {
        throw std::invalid_argument("Output must name a file without NUL characters");
    }
    try {
        const auto destination = std::filesystem::absolute(path);
        const StagingDirectory staging{destination.parent_path()};
        std::ofstream out;
        out.exceptions(std::ios::failbit | std::ios::badbit);
        out.open(staging.file(), std::ios::binary | std::ios::trunc);
        out << "P6\n" << image.width() << ' ' << image.height() << "\n255\n";
        // Fixed-size buffer bounds memory usage independently of canvas width.
        std::array<unsigned char, 12288> buffer;
        std::size_t used = 0;
        for (std::size_t y = 0; y < image.height(); ++y) {
            for (std::size_t x = 0; x < image.width(); ++x) {
                const auto color = image.colorAt(x, y);
                buffer[used++] = color.red();
                buffer[used++] = color.green();
                buffer[used++] = color.blue();
                if (used == buffer.size()) {
                    out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(used));
                    used = 0;
                }
            }
        }
        if (used != 0) out.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(used));
        out.close(); // Detect flush/close failures before replacing the destination.
        std::filesystem::rename(staging.file(), destination);
    } catch (const std::exception& error) {
        throw std::runtime_error("Cannot save '" + path.string() + "': " + error.what());
    }
}

} // namespace compositor
