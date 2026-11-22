#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dispatcher {
using WallClock = std::chrono::system_clock;
using SteadyClock = std::chrono::steady_clock;

// Validated immutable value object. Arguments are passed verbatim, without a shell.
class Job {
  public:
    Job(std::string id, std::filesystem::path executable, std::vector<std::string> arguments);
    const std::string &id() const noexcept { return id_; }
    const std::filesystem::path &executable() const noexcept { return executable_; }
    const std::vector<std::string> &arguments() const noexcept { return arguments_; }

  private:
    std::string id_;
    std::filesystem::path executable_;
    std::vector<std::string> arguments_;
};

class ExecutionPlan {
  public:
    ExecutionPlan(std::vector<Job> jobs, std::size_t concurrency, std::size_t blockSize = 65536);
    const std::vector<Job> &jobs() const noexcept { return jobs_; }
    std::size_t concurrency() const noexcept { return concurrency_; }
    std::size_t blockSize() const noexcept { return blockSize_; }

  private:
    std::vector<Job> jobs_;
    std::size_t concurrency_, blockSize_;
};

enum class JobStatus { Succeeded, NonzeroExit, LaunchFailed, IoFailed, WaitFailed };
std::string_view toString(JobStatus status);
struct CapturedStream {
    std::filesystem::path path;
    std::uint64_t bytes = 0;
    std::string error;
};
struct JobResult {
    std::string id;
    JobStatus status = JobStatus::LaunchFailed;
    std::optional<int> exitCode;
    CapturedStream standardOutput, standardError;
    std::string error;
    WallClock::time_point startedAt, finishedAt;
    std::chrono::nanoseconds duration{};
};
// Records notification handling order; output may still be draining at this time.
struct CompletionEvent {
    std::size_t sequence;
    std::string jobId, kind;
    std::optional<int> exitCode;
    WallClock::time_point timestamp;
};
struct RunReport {
    std::string mode;
    WallClock::time_point startedAt, finishedAt;
    std::chrono::nanoseconds duration{};
    std::size_t requestedConcurrency = 1, peakConcurrency = 0;
    std::vector<JobResult> results; // Input order, independent of completion order.
    std::vector<CompletionEvent> completions;
    bool successful() const noexcept;
};
std::int64_t timestampNs(WallClock::time_point time);
} // namespace dispatcher
