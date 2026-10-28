#include "cli.hpp"
#include "rendering.hpp"
#include "algorithms/generation.hpp"
#include "algorithms/pathfinding.hpp"

#include <iostream>
#include <random>
#include <sstream>
#include <string>

namespace {
    struct EndOfInput {};

    Position readPair(const std::string& prompt) {
        while (true) {
            std::cout << prompt;

            std::string line;
            if (!std::getline(std::cin, line)) {
                throw EndOfInput{};
            }

            std::istringstream input(line);
            int first;
            int second;

            if ((input >> first >> second) &&
                (input >> std::ws).eof() &&
                first >= 0 && second >= 0) {
                return {
                    static_cast<std::size_t>(first),
                    static_cast<std::size_t>(second)
                };
            }

            std::cout << "Enter two non-negative integers.\n";
        }
    }

    Position readDimensions() {
        while (true) {
            Position size = readPair("Enter rows and columns: ");

            // Keep console output manageable without restricting the core.
            if (size.row >= 2 && size.row <= 30 &&
                size.column >= 2 && size.column <= 50) {
                return size;
            }

            std::cout << "Use 2-30 rows and 2-50 columns.\n";
        }
    }

    Position readPosition(
        const Grid<int>& maze,
        const std::string& prompt
    ) {
        while (true) {
            Position position = readPair(prompt);

            if (maze.contains(position)) {
                return position;
            }

            std::cout << "Position is outside the maze.\n";
        }
    }
}

void runCli() {
    std::mt19937 random(std::random_device{}());

    try {
        while (true) {
            std::cout
                << "\n1. Generate and solve a maze\n"
                << "0. Exit\n"
                << "Choose an option: ";

            std::string choice;
            if (!std::getline(std::cin, choice)) {
                return;
            }

            if (choice == "0") {
                return;
            }

            if (choice != "1") {
                std::cout << "Unknown option.\n";
                continue;
            }

            Position size = readDimensions();
            auto maze = generateMaze<int>(size.row, size.column, random);

            std::cout << renderMaze(maze);
            std::cout << "Coordinates start at zero: row column.\n";
            std::cout << "Legend: # wall, . route, S start, E finish, B both.\n";

            Position start = readPosition(maze, "Enter start: ");
            Position finish = readPosition(maze, "Enter finish: ");

            auto path = findPath(maze, start, finish);

            if (path.empty()) {
                std::cout << "No route found.\n";
            } else {
                std::cout << renderMaze(maze, path);
                std::cout << "Route length: "
                          << path.size() - 1 << " moves.\n";
            }
        }
    } catch (const EndOfInput&) {
        std::cout << "\nInput closed. Exiting.\n";
    }
}
