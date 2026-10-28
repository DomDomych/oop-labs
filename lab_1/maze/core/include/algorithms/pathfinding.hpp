#pragma once

#include "grid.hpp"

#include <algorithm>
#include <queue>
#include <stdexcept>
#include <vector>

template<typename T>
std::vector<Position> findPath(
    const Grid<T>& maze,
    Position start,
    Position finish
) {
    if (!maze.contains(start) || !maze.contains(finish)) {
        throw std::out_of_range("Start or finish is outside the maze");
    }

    std::vector<std::vector<bool>> visited(
        maze.rows(), std::vector<bool>(maze.columns(), false)
    );

    std::vector<std::vector<Position>> previous(
        maze.rows(), std::vector<Position>(maze.columns())
    );

    std::queue<Position> pending;
    pending.push(start);
    visited[start.row][start.column] = true;

    while (!pending.empty()) {
        Position current = pending.front();
        pending.pop();

        if (current == finish) {
            break;
        }

        for (Position neighbor : maze.neighbors(current)) {
            if (visited[neighbor.row][neighbor.column]) {
                continue;
            }

            if (!maze.hasPassage(current, neighbor)) {
                continue;
            }

            visited[neighbor.row][neighbor.column] = true;
            previous[neighbor.row][neighbor.column] = current;
            pending.push(neighbor);
        }
    }

    if (!visited[finish.row][finish.column]) {
        return {}; // No route exists.
    }

    std::vector<Position> path;
    Position current = finish;

    while (current != start) {
        path.push_back(current);
        current = previous[current.row][current.column];
    }

    path.push_back(start);
    std::reverse(path.begin(), path.end());

    return path;
}