#include "platform/PipeDrainer.hpp"
#include <exception>
#include <thread>
namespace dispatcher {
void drainBlocking(ProcessSession &process, OutputPair &outputs, std::size_t blockSize,
                   std::array<std::string, 2> &errors) {
    // One child at a time, with independent blocking readers to avoid dual-pipe deadlock.
    // Allocate before launching threads so allocation failures cannot leave a blocked join.
    std::array<std::vector<char>, 2> buffers{std::vector<char>(blockSize), std::vector<char>(blockSize)};
    std::array<boost::asio::readable_pipe *, 2> pipes{&process.output(), &process.error()};
    std::array<IOutputCapture *, 2> captures{outputs.standardOutput.get(), outputs.standardError.get()};
    std::array<std::exception_ptr, 2> exceptions;
    auto read = [&](std::size_t index) {
        try {
            for (;;) {
                boost::system::error_code ec;
                const auto count = pipes[index]->read_some(boost::asio::buffer(buffers[index]), ec);
                if (count)
                    captures[index]->append({buffers[index].data(), count});
                if (ec) {
                    if (ec != boost::asio::error::eof)
                        errors[index] = ec.message();
                    break;
                }
            }
        } catch (...) {
            exceptions[index] = std::current_exception();
        }
        boost::system::error_code ignored;
        pipes[index]->close(ignored);
    };
    std::jthread first;
    std::jthread second;
    try {
        first = std::jthread(read, 0);
        second = std::jthread(read, 1);
    } catch (...) {
        boost::system::error_code ignored;
        process.child().terminate(ignored);
        throw; // jthread joins after terminating the child unblocks its output pipes.
    }
    first.join();
    second.join();
    for (const auto &exception : exceptions)
        if (exception)
            std::rethrow_exception(exception);
}
} // namespace dispatcher
