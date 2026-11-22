#include "dispatcher/output/IReportWriter.hpp"
#include "platform/Platform.hpp"
#include <atomic>
#include <boost/json.hpp>
#include <fstream>
#include <stdexcept>
namespace dispatcher {
namespace {
boost::json::value code(std::optional<int> value) {
    return value ? boost::json::value(*value) : boost::json::value(nullptr);
}
boost::json::object stream(const CapturedStream &value) {
    return {{"path", platform::toUtf8(value.path)}, {"bytes", value.bytes}, {"error", value.error}};
}
} // namespace
void JsonReportWriter::write(const RunReport &report) {
    namespace json = boost::json;
    json::array results, completions;
    for (const auto &r : report.results)
        results.push_back(json::object{{"id", r.id},
                                       {"status", toString(r.status)},
                                       {"exit_code", code(r.exitCode)},
                                       {"stdout", stream(r.standardOutput)},
                                       {"stderr", stream(r.standardError)},
                                       {"error", r.error},
                                       {"started_at_ns", std::to_string(timestampNs(r.startedAt))},
                                       {"finished_at_ns", std::to_string(timestampNs(r.finishedAt))},
                                       {"duration_ns", r.duration.count()}});
    for (const auto &c : report.completions)
        completions.push_back(json::object{{"sequence", c.sequence},
                                           {"job_id", c.jobId},
                                           {"kind", c.kind},
                                           {"exit_code", code(c.exitCode)},
                                           {"timestamp_ns", std::to_string(timestampNs(c.timestamp))}});
    const auto document =
        json::serialize(json::object{{"schema_version", 1},
                                     {"mode", report.mode},
                                     {"started_at_ns", std::to_string(timestampNs(report.startedAt))},
                                     {"finished_at_ns", std::to_string(timestampNs(report.finishedAt))},
                                     {"duration_ns", report.duration.count()},
                                     {"requested_concurrency", report.requestedConcurrency},
                                     {"peak_concurrency", report.peakConcurrency},
                                     {"successful", report.successful()},
                                     {"results", std::move(results)},
                                     {"completions", std::move(completions)}});
    const auto target = std::filesystem::absolute(path_);
    std::filesystem::create_directories(target.parent_path());
    static std::atomic<unsigned> sequence{0};
    auto temporary = target;
    temporary += ".tmp-" + std::to_string(platform::processId()) + "-" + std::to_string(sequence++);
    try {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << document << '\n';
        file.close();
        platform::replaceFile(temporary, target);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}
} // namespace dispatcher
