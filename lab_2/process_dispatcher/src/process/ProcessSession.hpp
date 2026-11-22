#pragma once
#include "dispatcher/model/Model.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/process/v2/process.hpp>
namespace dispatcher {
// Owns process + channels. The only adapter that knows how Boost.Process launches a child.
class ProcessSession {
  public:
    ProcessSession(boost::asio::io_context &context, const Job &job);
    ~ProcessSession();
    boost::asio::readable_pipe &output() noexcept { return output_; }
    boost::asio::readable_pipe &error() noexcept { return error_; }
    boost::process::v2::process &child() noexcept { return child_; }

  private:
    boost::asio::readable_pipe output_, error_;
    boost::process::v2::process child_;
};
} // namespace dispatcher
