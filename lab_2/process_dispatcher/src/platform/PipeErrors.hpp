#pragma once
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
namespace dispatcher {
inline bool pipeEnded(const boost::system::error_code &error) {
    // Windows named pipes report ERROR_BROKEN_PIPE when the last writer closes.
    return error == boost::asio::error::eof
#ifdef _WIN32
           || error == boost::asio::error::broken_pipe
#endif
        ;
}
} // namespace dispatcher
