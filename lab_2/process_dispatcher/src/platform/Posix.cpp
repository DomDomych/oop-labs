#include "platform/Platform.hpp"
#include <fcntl.h>
#include <unistd.h>
namespace dispatcher::platform {
std::filesystem::path executablePath() {
    return std::filesystem::read_symlink("/proc/self/exe");
}
unsigned long processId() {
    return static_cast<unsigned long>(::getpid());
}
int createExclusive(const std::filesystem::path &path) {
    return ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
}
std::ptrdiff_t writeFile(int fd, std::span<const char> bytes) noexcept {
    return ::write(fd, bytes.data(), bytes.size());
}
int closeFile(int fd) noexcept {
    return ::close(fd);
}
void replaceFile(const std::filesystem::path &source, const std::filesystem::path &target) {
    std::filesystem::rename(source, target);
}
void binaryStandardStreams() {}
} // namespace dispatcher::platform
