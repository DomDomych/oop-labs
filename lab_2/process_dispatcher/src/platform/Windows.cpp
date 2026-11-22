#include "platform/Platform.hpp"
#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <sys/stat.h>
#include <system_error>
namespace dispatcher::platform {
std::filesystem::path executablePath() {
    std::wstring buffer(512, L'\0');
    for (;;) {
        const auto size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (!size)
            throw std::system_error(GetLastError(), std::system_category(), "GetModuleFileNameW");
        if (size < buffer.size()) {
            buffer.resize(size);
            return buffer;
        }
        buffer.resize(buffer.size() * 2);
    }
}
unsigned long processId() {
    return static_cast<unsigned long>(::_getpid());
}
int createExclusive(const std::filesystem::path &path) {
    return ::_wopen(path.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY | _O_NOINHERIT,
                    _S_IREAD | _S_IWRITE);
}
std::ptrdiff_t writeFile(int fd, std::span<const char> bytes) noexcept {
    return ::_write(fd, bytes.data(), static_cast<unsigned>(std::min<std::size_t>(bytes.size(), INT_MAX)));
}
int closeFile(int fd) noexcept {
    return ::_close(fd);
}
void replaceFile(const std::filesystem::path &source, const std::filesystem::path &target) {
    if (!MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::system_error(GetLastError(), std::system_category(), "Replace report");
}
void binaryStandardStreams() {
    if (_setmode(_fileno(stdout), _O_BINARY) == -1 || _setmode(_fileno(stderr), _O_BINARY) == -1)
        throw std::system_error(errno, std::generic_category(), "Set binary output");
}
std::vector<std::string> utf8Arguments(int argc, wchar_t **argv) {
    std::vector<std::string> result;
    result.reserve(argc);
    for (int i = 0; i < argc; ++i)
        result.push_back(toUtf8(std::filesystem::path(argv[i])));
    return result;
}
} // namespace dispatcher::platform
