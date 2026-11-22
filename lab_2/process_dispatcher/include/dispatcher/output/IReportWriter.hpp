#pragma once
#include "dispatcher/model/Model.hpp"
namespace dispatcher {
class IReportWriter {
  public:
    virtual ~IReportWriter() = default;
    virtual void write(const RunReport &report) = 0;
};
class JsonReportWriter final : public IReportWriter {
  public:
    explicit JsonReportWriter(std::filesystem::path path) : path_(std::move(path)) {}
    void write(const RunReport &report) override;

  private:
    std::filesystem::path path_;
};
} // namespace dispatcher
