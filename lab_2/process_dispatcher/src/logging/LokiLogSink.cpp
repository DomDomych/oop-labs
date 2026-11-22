#include "dispatcher/logging/LokiLogSink.hpp"

#include <algorithm>
#include <boost/json.hpp>
#include <stdexcept>
#include <vector>

namespace dispatcher::logging {
namespace {
constexpr long kLokiPushSuccessStatus = 204;
constexpr long kLokiIngestionBlockedStatus = 260;

bool letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool labelName(const std::string &key) {
    return !key.empty() && letter(key.front()) &&
           std::all_of(key.begin(), key.end(), [](char c) { return letter(c) || (c >= '0' && c <= '9'); });
}
} // namespace

LokiLogSink::LokiLogSink(std::string url, std::map<std::string, std::string> labels,
                         std::chrono::milliseconds timeout, std::unique_ptr<IHttpTransport> transport,
                         std::shared_ptr<const ILogFormatter> formatter)
    : url_(std::move(url)), labels_(std::move(labels)), timeout_(timeout), transport_(std::move(transport)),
      formatter_(std::move(formatter)) {
    if ((!url_.starts_with("http://") && !url_.starts_with("https://")) ||
        url_.find('\0') != std::string::npos)
        throw std::invalid_argument("Loki requires an HTTP(S) push URL");
    if (!transport_ || !formatter_ || timeout_.count() <= 0 || labels_.empty())
        throw std::invalid_argument("Invalid Loki sink dependencies or options");
    for (const auto &[name, value] : labels_) {
        if (!labelName(name) || value.empty())
            throw std::invalid_argument("Invalid Loki stream label");
    }
}

void LokiLogSink::write(std::span<const LogRecord> records) {
    if (records.empty())
        return;
    boost::json::object labels;
    for (const auto &[key, value] : labels_)
        labels[key] = value;
    std::size_t offset = 0;
    while (offset < records.size()) {
        boost::json::array values;
        std::size_t bytes = 0;
        while (offset < records.size() && values.size() < 256 && bytes < 512 * 1024) {
            const auto &record = records[offset++];
            const auto ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(record.timestamp.time_since_epoch())
                    .count();
            // Wall clock adjustments must not reorder completion events. The original
            // timestamp remains inside the JSON log line; only Loki's index is adjusted.
            lastTimestampNs_ = std::max<std::int64_t>(ns, lastTimestampNs_ + 1);
            auto line = formatter_->format(record);
            if (line.size() > 256 * 1024)
                throw std::runtime_error("Loki log line exceeds 256 KiB");
            bytes += line.size();
            values.push_back(boost::json::array{std::to_string(lastTimestampNs_), std::move(line)});
        }
        const auto body = boost::json::serialize(boost::json::object{
            {"streams",
             boost::json::array{boost::json::object{{"stream", labels}, {"values", std::move(values)}}}}});
        const auto status = transport_->postJson(url_, body, timeout_);
        if (status == kLokiIngestionBlockedStatus)
            throw std::runtime_error("Loki ingestion is blocked: HTTP " + std::to_string(status));
        if (status != kLokiPushSuccessStatus)
            throw std::runtime_error("Loki push failed: HTTP " + std::to_string(status));
    }
}

} // namespace dispatcher::logging
