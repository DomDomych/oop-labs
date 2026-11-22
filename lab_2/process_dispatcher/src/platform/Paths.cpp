#include "platform/Platform.hpp"
namespace dispatcher::platform {
std::filesystem::path fromUtf8(const std::string &text) {
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}
std::string toUtf8(const std::filesystem::path &path) {
    const auto text = path.u8string();
    return {text.begin(), text.end()};
}
} // namespace dispatcher::platform
