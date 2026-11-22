#include "dispatcher/model/Model.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace dispatcher {
Job::Job(std::string id, std::filesystem::path executable, std::vector<std::string> arguments)
    : id_(std::move(id)), executable_(std::move(executable)), arguments_(std::move(arguments)) {
    if (id_.empty() || id_.size() > 128 || !std::all_of(id_.begin(), id_.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' ||
                   c == '-';
        }))
        throw std::invalid_argument("Job id must contain 1..128 ASCII letters, digits, '_' or '-'");
    if (executable_.empty() || executable_.native().find(std::filesystem::path::value_type{}) !=
                                   std::filesystem::path::string_type::npos)
        throw std::invalid_argument("Executable path is empty or contains NUL");
    for (const auto &arg : arguments_)
        if (arg.find('\0') != std::string::npos)
            throw std::invalid_argument("Argument contains NUL");
}
ExecutionPlan::ExecutionPlan(std::vector<Job> jobs, std::size_t concurrency, std::size_t blockSize)
    : jobs_(std::move(jobs)), concurrency_(concurrency), blockSize_(blockSize) {
    if (jobs_.size() > 10000)
        throw std::invalid_argument("At most 10000 jobs per run");
    if (concurrency == 0 || concurrency > 256)
        throw std::invalid_argument("Concurrency must be 1..256");
    if (blockSize == 0 || blockSize > 1048576)
        throw std::invalid_argument("Block size must be 1..1048576");
    std::set<std::string> ids;
    for (const auto &job : jobs_)
        if (!ids.insert(job.id()).second)
            throw std::invalid_argument("Duplicate job id: " + job.id());
}
std::string_view toString(JobStatus status) {
    switch (status) {
    case JobStatus::Succeeded:
        return "succeeded";
    case JobStatus::NonzeroExit:
        return "nonzero_exit";
    case JobStatus::LaunchFailed:
        return "launch_failed";
    case JobStatus::IoFailed:
        return "io_failed";
    case JobStatus::WaitFailed:
        return "wait_failed";
    }
    throw std::invalid_argument("Unknown job status");
}
bool RunReport::successful() const noexcept {
    return std::all_of(results.begin(), results.end(),
                       [](const auto &r) { return r.status == JobStatus::Succeeded; });
}
std::int64_t timestampNs(WallClock::time_point time) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
}
} // namespace dispatcher
