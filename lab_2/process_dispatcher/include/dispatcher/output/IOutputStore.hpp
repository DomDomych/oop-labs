#pragma once
#include "dispatcher/model/Model.hpp"
#include <memory>
#include <span>
namespace dispatcher {
class IOutputCapture {
  public:
    virtual ~IOutputCapture() = default;
    // Serialized per stream. On failure, record the error and keep accepting (discarding)
    // subsequent bytes, so a broken output file cannot deadlock a child process.
    virtual void append(std::span<const char> bytes) noexcept = 0;
    virtual CapturedStream finish() = 0;
};
struct OutputPair {
    std::unique_ptr<IOutputCapture> standardOutput, standardError;
};
class IOutputStore {
  public:
    virtual ~IOutputStore() = default;
    virtual OutputPair open(const Job &job) = 0;
};
class FileOutputStore final : public IOutputStore {
  public:
    explicit FileOutputStore(const std::filesystem::path &root);
    OutputPair open(const Job &job) override;
    const std::filesystem::path &directory() const noexcept { return directory_; }

  private:
    std::filesystem::path directory_;
};
} // namespace dispatcher
