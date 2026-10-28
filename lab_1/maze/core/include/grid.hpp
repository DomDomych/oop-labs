#pragma once

#include "position.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

template<typename T>
class Grid {
private:
    struct Cell {
        T data{};
        bool passageRight = false;
        bool passageDown = false;
    };

    std::size_t rows_;
    std::size_t columns_;
    std::vector<Cell> cells_;

    std::size_t index(Position position) const;

public:
    Grid(std::size_t rows, std::size_t columns);

    std::size_t rows() const;
    std::size_t columns() const;
    bool contains(Position position) const;

    T& at(Position position);
    const T& at(Position position) const;

    std::vector<Position> neighbors(Position position) const;
    void openPassage(Position from, Position to);
    bool hasPassage(Position from, Position to) const;
};

template<typename T>
Grid<T>::Grid(std::size_t rows, std::size_t columns)
    : rows_(rows), columns_(columns) {
    if (rows == 0 || columns == 0) {
        throw std::invalid_argument("Grid dimensions must be positive");
    }
    if (rows > std::numeric_limits<std::size_t>::max() / columns) {
        throw std::length_error("Grid dimensions are too large");
    }
    cells_.resize(rows * columns);
}

template<typename T>
std::size_t Grid<T>::rows() const {
    return rows_;
}

template<typename T>
std::size_t Grid<T>::columns() const {
    return columns_;
}

template<typename T>
bool Grid<T>::contains(Position position) const {
    return position.row < rows_ && position.column < columns_;
}

template<typename T>
std::size_t Grid<T>::index(Position position) const {
    if (!contains(position)) {
        throw std::out_of_range("Cell position is outside the grid");
    }
    return position.row * columns_ + position.column;
}

template<typename T>
T& Grid<T>::at(Position position) {
    return cells_[index(position)].data;
}

template<typename T>
const T& Grid<T>::at(Position position) const {
    return cells_[index(position)].data;
}

template<typename T>
std::vector<Position> Grid<T>::neighbors(Position position) const {
    index(position);
    std::vector<Position> result;
    result.reserve(4);
    if (position.row > 0) {
        result.push_back({position.row - 1, position.column});
    }
    if (position.column < columns_ - 1) {
        result.push_back({position.row, position.column + 1});
    }
    if (position.row < rows_ - 1) {
        result.push_back({position.row + 1, position.column});
    }
    if (position.column > 0) {
        result.push_back({position.row, position.column - 1});
    }
    return result;
}

template<typename T>
void Grid<T>::openPassage(Position from, Position to) {
    const auto fromIndex = index(from);
    const auto toIndex = index(to);
    const auto first = std::min(fromIndex, toIndex);
    const auto last = std::max(fromIndex, toIndex);

    if (from.row == to.row && last - first == 1) {
        cells_[first].passageRight = true;
    } else if (from.column == to.column && last - first == columns_) {
        cells_[first].passageDown = true;
    } else {
        throw std::invalid_argument("Passages require side-adjacent cells");
    }
}

template<typename T>
bool Grid<T>::hasPassage(Position from, Position to) const {
    const auto fromIndex = index(from);
    const auto toIndex = index(to);
    const auto first = std::min(fromIndex, toIndex);
    const auto last = std::max(fromIndex, toIndex);

    if (from.row == to.row && last - first == 1) {
        return cells_[first].passageRight;
    }
    if (from.column == to.column && last - first == columns_) {
        return cells_[first].passageDown;
    }
    return false;
}
