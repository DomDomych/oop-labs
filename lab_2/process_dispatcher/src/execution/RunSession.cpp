#include "execution/RunSession.hpp"
#include <algorithm>
#include <stdexcept>
namespace dispatcher {
RunSession::RunSession(const ExecutionPlan &plan, std::string mode, logging::ILogger &logger)
    : plan_(plan), logger_(logger), start_(SteadyClock::now()), started_(plan.jobs().size()),
      completed_(plan.jobs().size()), notified_(plan.jobs().size()) {
    report_.mode = std::move(mode);
    report_.requestedConcurrency = plan.concurrency();
    report_.startedAt = WallClock::now();
    report_.results.resize(plan.jobs().size());
    logger_.log({report_.startedAt,
                 logging::Level::Info,
                 "dispatch_started",
                 {{"mode", report_.mode}, {"jobs", std::to_string(plan.jobs().size())}}});
}
void RunSession::started(std::size_t index) {
    if (started_.at(index))
        throw std::logic_error("Job started twice");
    started_[index] = true;
    report_.peakConcurrency = std::max(report_.peakConcurrency, ++active_);
}
void RunSession::exited(std::size_t index, std::optional<int> code, std::string kind) {
    if (notified_.at(index))
        throw std::logic_error("Duplicate completion notification");
    notified_[index] = true;
    const auto now = WallClock::now();
    const auto sequence = report_.completions.size() + 1;
    report_.completions.push_back({sequence, plan_.jobs()[index].id(), kind, code, now});
    logger_.log({now,
                 logging::Level::Info,
                 std::move(kind),
                 {{"job_id", plan_.jobs()[index].id()},
                  {"sequence", std::to_string(sequence)},
                  {"exit_code", code ? std::to_string(*code) : ""}}});
}
void RunSession::finished(std::size_t index, JobResult result) {
    if (completed_.at(index))
        throw std::logic_error("Job completed twice");
    completed_[index] = true;
    if (started_[index])
        --active_;
    logger_.log({result.finishedAt,
                 result.status == JobStatus::Succeeded ? logging::Level::Info : logging::Level::Error,
                 "job_finished",
                 {{"job_id", result.id},
                  {"status", std::string(toString(result.status))},
                  {"duration_ns", std::to_string(result.duration.count())},
                  {"stdout_bytes", std::to_string(result.standardOutput.bytes)},
                  {"stderr_bytes", std::to_string(result.standardError.bytes)},
                  {"error", result.error},
                  {"stdout_error", result.standardOutput.error},
                  {"stderr_error", result.standardError.error}}});
    report_.results[index] = std::move(result);
}
RunReport RunSession::finish() {
    if (active_ || std::find(completed_.begin(), completed_.end(), false) != completed_.end())
        throw std::logic_error("Run finished with incomplete jobs");
    report_.finishedAt = WallClock::now();
    report_.duration = SteadyClock::now() - start_;
    logger_.log({report_.finishedAt,
                 logging::Level::Info,
                 "dispatch_finished",
                 {{"duration_ns", std::to_string(report_.duration.count())},
                  {"success", report_.successful() ? "true" : "false"}}});
    return std::move(report_);
}
void finalizeStatus(JobResult &result) {
    if (!result.exitCode)
        return; // Explicit launch/wait failure already supplied.
    if (!result.standardOutput.error.empty() || !result.standardError.error.empty())
        result.status = JobStatus::IoFailed;
    else
        result.status = *result.exitCode == 0 ? JobStatus::Succeeded : JobStatus::NonzeroExit;
}
} // namespace dispatcher
