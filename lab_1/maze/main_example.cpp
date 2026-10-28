#include "algorithms/generation.hpp"
#include "algorithms/pathfinding.hpp"
#include "rendering.hpp"

#include <iostream>
#include <random>

int main() {
    std::mt19937 random(42);

    std::cout << "Maze examples. Dimensions are measured in cells.\n"
              << "# wall, . route, S start, E finish\n"
              << "Coordinates start at zero: row column.\n";

    for (Position size : {Position{5, 5}, Position{4, 7}}) {
        const auto maze = generateMaze<int>(size.row, size.column, random);
        const Position start{0, 0};
        const Position finish{size.row - 1, size.column - 1};
        const auto path = findPath(maze, start, finish);

        std::cout << "\n=== " << size.row << " x " << size.column << " cells ===\n"
                  << "Generated maze:\n"
                  << renderMaze(maze);

        if (path.empty()) {
            std::cerr << "Error: generated maze has no route.\n";
            return 1;
        }

        std::cout << "Route from (0, 0) to (" << finish.row << ", "
                  << finish.column << "):\n"
                  << renderMaze(maze, path)
                  << "Route length: " << path.size() - 1 << " moves.\n";
    }
}
