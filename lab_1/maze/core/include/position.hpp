#pragma once

#include <cstddef>

struct Position {
    std::size_t row;
    std::size_t column;

    bool operator==(const Position& other) const = default;
};
