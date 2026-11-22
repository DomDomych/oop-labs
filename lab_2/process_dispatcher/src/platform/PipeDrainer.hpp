#pragma once
#include "dispatcher/output/IOutputStore.hpp"
#include "process/ProcessSession.hpp"
#include <array>
namespace dispatcher {
void drainBlocking(ProcessSession &process, OutputPair &outputs, std::size_t blockSize,
                   std::array<std::string, 2> &errors);
}
