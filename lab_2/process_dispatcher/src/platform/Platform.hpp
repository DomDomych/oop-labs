#pragma once
#include <filesystem>
#include <span>
#include <string>
#include <vector>
namespace dispatcher::platform {
std::filesystem::path executablePath();
std::filesystem::path fromUtf8(const std::string &text);
std::string toUtf8(const std::filesystem::path &path);
unsigned long processId();
int createExclusive(const std::filesystem::path &path);
std::ptrdiff_t writeFile(int fd, std::span<const char> bytes) noexcept;
int closeFile(int fd) noexcept;
void replaceFile(const std::filesystem::path &source, const std::filesystem::path &target);
void binaryStandardStreams();
#ifdef _WIN32
std::wstring commandLine(const std::filesystem::path &executable, const std::vector<std::string> &arguments);
std::vector<std::string> utf8Arguments(int argc, wchar_t **argv);
#endif
} // namespace dispatcher::platform
