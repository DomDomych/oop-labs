#include "dispatcher/output/IOutputStore.hpp"
#include "platform/Platform.hpp"
#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <stdexcept>
#include <system_error>

namespace dispatcher {
namespace {
class FileCapture final : public IOutputCapture {
  public:
    explicit FileCapture(std::filesystem::path path) {
        result_.path = std::move(path);
        fd_ = platform::createExclusive(result_.path);
        if (fd_ < 0)
            throw std::system_error(errno, std::generic_category(),
                                    "Create output " + platform::toUtf8(result_.path));
    }
    ~FileCapture() override {
        if (fd_ >= 0)
            platform::closeFile(fd_);
    }
    void append(std::span<const char> bytes) noexcept override {
        if (failed_)
            return;
        while (!bytes.empty()) {
            const auto count = platform::writeFile(fd_, bytes);
            if (count < 0 && errno == EINTR)
                continue;
            if (count <= 0) {
                failed_ = true;
                errorCode_ = count < 0 ? errno : EIO;
                return;
            }
            result_.bytes += static_cast<std::uint64_t>(count);
            bytes = bytes.subspan(static_cast<std::size_t>(count));
        }
    }
    CapturedStream finish() override {
        if (fd_ >= 0) {
            if (platform::closeFile(fd_) != 0 && !failed_) {
                failed_ = true;
                errorCode_ = errno;
            }
            fd_ = -1;
        }
        if (failed_)
            result_.error = std::error_code(errorCode_, std::generic_category()).message();
        return result_;
    }

  private:
    int fd_ = -1, errorCode_ = 0;
    bool failed_ = false;
    CapturedStream result_;
};
} // namespace
FileOutputStore::FileOutputStore(const std::filesystem::path &root) {
    std::filesystem::create_directories(root);
    static std::atomic<unsigned long> sequence{0};
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto candidate = std::filesystem::absolute(root) /
                               ("run-" + std::to_string(timestampNs(WallClock::now())) + "-" +
                                std::to_string(platform::processId()) + "-" + std::to_string(sequence++));
        if (std::filesystem::create_directory(candidate)) {
            directory_ = candidate;
            return;
        }
    }
    throw std::runtime_error("Cannot allocate a unique output directory");
}
OutputPair FileOutputStore::open(const Job &job) {
    return {std::make_unique<FileCapture>(directory_ / (job.id() + ".stdout")),
            std::make_unique<FileCapture>(directory_ / (job.id() + ".stderr"))};
}
} // namespace dispatcher
