#pragma once

#include "grid.hpp"

#include <random>
#include <stdexcept>
#include <vector>

template<typename T>
Grid<T> generateMaze(
    std::size_t rows,
    std::size_t columns,
    std::mt19937& random
) {
    if (rows < 2 || columns < 2) {
        throw std::invalid_argument("Maze must be at least 2 x 2");
    }

    Grid<T> maze(rows, columns);
    std::vector<std::vector<bool>> visited(
        rows, std::vector<bool>(columns, false)
    );
    std::vector<Position> stack;

    stack.push_back({0, 0});
    visited[0][0] = true;

    while (!stack.empty()) {
        Position current = stack.back();
        std::vector<Position> candidates;

        for (Position neighbor : maze.neighbors(current)) {
            if (!visited[neighbor.row][neighbor.column]) {
                candidates.push_back(neighbor);
            }
        }

        if (candidates.empty()) {
            stack.pop_back();
            continue;
        }

        std::uniform_int_distribution<std::size_t> choose(
            0, candidates.size() - 1
        );
        Position next = candidates[choose(random)];

        maze.openPassage(current, next);
        visited[next.row][next.column] = true;
        stack.push_back(next);
    }

    return maze;
}   