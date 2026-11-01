#include "logging/CurlHttpTransport.hpp"

#include <curl/curl.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace compositor::logging {
namespace {
class CurlRuntime {
public:
    CurlRuntime() {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) throw std::runtime_error("Cannot initialize curl");
    }
    ~CurlRuntime() { curl_global_cleanup(); }
};
void check(CURLcode code) {
    if (code != CURLE_OK) throw std::runtime_error(std::string("HTTP transport: ") + curl_easy_strerror(code));
}
std::size_t discard(char*, std::size_t size, std::size_t count, void*) { return size * count; }
}

CurlHttpTransport::CurlHttpTransport() {
    static const CurlRuntime runtime;
    (void)runtime;
}

unsigned CurlHttpTransport::postJson(std::string_view url, std::string_view body,
                                    std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0 || timeout.count() > std::numeric_limits<long>::max())
        throw std::invalid_argument("Invalid HTTP timeout");
    if ((!url.starts_with("http://") && !url.starts_with("https://")) || url.find('\0') != std::string_view::npos)
        throw std::invalid_argument("Expected HTTP(S) URL");
    const std::string address(url);
    const std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);
    if (!handle) throw std::runtime_error("Cannot create HTTP request");
    const std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(
        curl_slist_append(nullptr, "Content-Type: application/json"), curl_slist_free_all);
    if (!headers) throw std::bad_alloc();
    check(curl_easy_setopt(handle.get(), CURLOPT_URL, address.c_str()));
    check(curl_easy_setopt(handle.get(), CURLOPT_POST, 1L));
    check(curl_easy_setopt(handle.get(), CURLOPT_HTTPHEADER, headers.get()));
    check(curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDS, body.data()));
    check(curl_easy_setopt(handle.get(), CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size())));
    check(curl_easy_setopt(handle.get(), CURLOPT_TIMEOUT_MS, static_cast<long>(timeout.count())));
    check(curl_easy_setopt(handle.get(), CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout.count())));
    check(curl_easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L));
    check(curl_easy_setopt(handle.get(), CURLOPT_FOLLOWLOCATION, 0L));
    check(curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, discard));
    check(curl_easy_perform(handle.get()));
    long status = 0;
    check(curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &status));
    return static_cast<unsigned>(status);
}

} // namespace compositor::logging
