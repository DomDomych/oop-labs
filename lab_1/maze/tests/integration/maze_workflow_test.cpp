#include "algorithms/generation.hpp"
#include "algorithms/pathfinding.hpp"
#include "rendering.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

TEST(MazeWorkflowIntegrationTest, GeneratesSolvesAndRendersTwoDifferentSizes) {
    std::mt19937 random(42);
    for (Position size : {Position{2, 2}, Position{5, 7}}) {
        SCOPED_TRACE(::testing::Message() << size.row << 'x' << size.column);
        auto maze = generateMaze<std::string>(size.row, size.column, random);
        const Position start{0, 0};
        const Position finish{size.row - 1, size.column - 1};
        maze.at(start) = "entrance";
        const auto path = findPath(maze, start, finish);
        ASSERT_FALSE(path.empty());
        EXPECT_EQ(path.front(), start);
        EXPECT_EQ(path.back(), finish);
        for (std::size_t i = 1; i < path.size(); ++i) {
            EXPECT_TRUE(maze.hasPassage(path[i - 1], path[i]));
        }
        const auto unsolved = renderMaze(maze);
        const auto solved = renderMaze(maze, path);
        EXPECT_EQ(unsolved.find('S'), std::string::npos);
        EXPECT_NE(solved.find('S'), std::string::npos);
        EXPECT_NE(solved.find('E'), std::string::npos);
        EXPECT_NE(solved.find('.'), std::string::npos);
        std::istringstream input(solved);
        std::string line;
        std::size_t lines = 0;
        while (std::getline(input, line)) {
            ASSERT_EQ(line.size(), 2 * size.column + 1);
            EXPECT_EQ(line.front(), '#');
            EXPECT_EQ(line.back(), '#');
            if (lines == 0 || lines == 2 * size.row) {
                EXPECT_EQ(line, std::string(line.size(), '#'));
            }
            ++lines;
        }
        EXPECT_EQ(lines, 2 * size.row + 1);
        EXPECT_EQ(maze.at(start), "entrance");
        EXPECT_EQ(findPath(maze, start, finish), path);
        EXPECT_EQ(renderMaze(maze), unsolved);
    }
}
