#include "logging/LokiLogSink.hpp"

#include <boost/json.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace compositor::logging {
namespace {
bool letter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool labelName(const std::string& key) {
    return !key.empty() && letter(key.front()) && std::all_of(key.begin(), key.end(), [](char c) {
        return letter(c) || (c >= '0' && c <= '9');
    });
}
}

LokiLogSink::LokiLogSink(std::string url, std::map<std::string, std::string> labels,
                         std::chrono::milliseconds timeout, std::unique_ptr<IHttpTransport> transport,
                         std::shared_ptr<const ILogFormatter> formatter)
    : url_(std::move(url)), labels_(std::move(labels)), timeout_(timeout),
      transport_(std::move(transport)), formatter_(std::move(formatter)) {
    if ((!url_.starts_with("http://") && !url_.starts_with("https://")) || url_.find('\0') != std::string::npos)
        throw std::invalid_argument("Loki requires an HTTP(S) push URL");
    if (!transport_ || !formatter_ || timeout_.count() <= 0 || labels_.empty())
        throw std::invalid_argument("Invalid Loki sink dependencies or options");
    for (const auto& [name, value] : labels_) {
        if (!labelName(name) || value.empty()) throw std::invalid_argument("Invalid Loki stream label");
    }
}

void LokiLogSink::write(std::span<const LogRecord> records) {
    if (records.empty()) return;
    std::vector<const LogRecord*> ordered;
    ordered.reserve(records.size());
    for (const auto& record : records) ordered.push_back(&record);
    std::stable_sort(ordered.begin(), ordered.end(), [](const auto* a, const auto* b) { return a->timestamp < b->timestamp; });
    boost::json::object labels;
    for (const auto& [key, value] : labels_) labels[key] = value;
    boost::json::array values;
    for (const auto* record : ordered) {
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(record->timestamp.time_since_epoch()).count();
        values.push_back(boost::json::array{std::to_string(ns), formatter_->format(*record)});
    }
    const auto body = boost::json::serialize(boost::json::object{{"streams", boost::json::array{
        boost::json::object{{"stream", std::move(labels)}, {"values", std::move(values)}}}}});
    const auto status = transport_->postJson(url_, body, timeout_);
    // Loki's successful push response is 204. In particular, configured ingestion
    // blocking may return status 260, which must not be mistaken for success.
    if (status != 204) throw std::runtime_error("Loki push failed: HTTP " + std::to_string(status));
}

} // namespace compositor::logging
