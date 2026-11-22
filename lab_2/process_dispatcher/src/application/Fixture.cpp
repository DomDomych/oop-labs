#include "application/Fixture.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
namespace dispatcher {
int runFixture(int argc, char **argv) {
    platform::binaryStandardStreams();
    std::string out, err;
    unsigned delay = 0, exitCode = 0, bytes = 0;
    bool binary = false;
    for (int i = 2; i < argc; ++i) {
        std::string option = argv[i];
        if (option == "--binary") {
            binary = true;
            continue;
        }
        if (++i == argc)
            throw std::invalid_argument("Fixture option needs a value");
        std::string value = argv[i];
        if (option == "--stdout")
            out = value;
        else if (option == "--stderr")
            err = value;
        else {
            unsigned n = 0;
            const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), n);
            if (ec != std::errc{} || end != value.data() + value.size())
                throw std::invalid_argument("Invalid fixture number");
            if (option == "--delay-ms" && n <= 60000)
                delay = n;
            else if (option == "--exit-code" && n <= 255)
                exitCode = n;
            else if (option == "--bytes" && n <= 1073741824)
                bytes = n;
            else
                throw std::invalid_argument("Invalid fixture option/range: " + option);
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    std::cout << out;
    std::cerr << err;
    std::string block(8192, 'O'), errorBlock(8192, 'E');
    if (binary)
        for (std::size_t i = 0; i < block.size(); ++i) {
            block[i] = static_cast<char>(i % 256);
            errorBlock[i] = static_cast<char>(255 - i % 256);
        }
    for (unsigned offset = 0; offset < bytes;) {
        const auto count = std::min<unsigned>(8192, bytes - offset);
        std::cout.write(block.data(), count).flush();
        std::cerr.write(errorBlock.data(), count).flush();
        offset += count;
    }
    std::cout.flush();
    std::cerr.flush();
    return static_cast<int>(exitCode);
}
} // namespace dispatcher
