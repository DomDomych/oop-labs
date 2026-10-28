#include "algorithms/pathfinding.hpp"

#include <gtest/gtest.h>
#include <string>

TEST(PathfindingTest, ReconstructsKnownRouteInBothDirections) {
    Grid<int> maze(2, 3);
    const std::vector<Position> expected{{0, 0}, {1, 0}, {1, 1}, {1, 2}, {0, 2}};
    for (std::size_t i = 1; i < expected.size(); ++i) {
        maze.openPassage(expected[i - 1], expected[i]);
    }
    EXPECT_EQ(findPath(maze, expected.front(), expected.back()), expected);
    const std::vector<Position> reversed(expected.rbegin(), expected.rend());
    EXPECT_EQ(findPath(maze, expected.back(), expected.front()), reversed);
}

TEST(PathfindingTest, DoesNotCrossClosedWalls) {
    Grid<int> maze(2, 2);
    maze.openPassage({0, 0}, {0, 1});
    EXPECT_TRUE(findPath(maze, {0, 0}, {1, 1}).empty());
}

TEST(PathfindingTest, SameEndpointReturnsOneCell) {
    const Grid<int> maze(2, 2);
    const std::vector<Position> expected{{1, 1}};
    EXPECT_EQ(findPath(maze, {1, 1}, {1, 1}), expected);
}

TEST(PathfindingTest, RejectsInvalidEndpoints) {
    const Grid<int> maze(2, 2);
    EXPECT_THROW(findPath(maze, {2, 0}, {0, 0}), std::out_of_range);
    EXPECT_THROW(findPath(maze, {0, 0}, {0, 2}), std::out_of_range);
}

TEST(PathfindingTest, ChoosesShortestRouteWhenThereIsACycle) {
    Grid<int> maze(2, 3);
    maze.openPassage({0, 0}, {0, 1});
    maze.openPassage({0, 1}, {0, 2});
    maze.openPassage({0, 0}, {1, 0});
    maze.openPassage({1, 0}, {1, 1});
    maze.openPassage({1, 1}, {1, 2});
    maze.openPassage({1, 2}, {0, 2});
    const std::vector<Position> expected{{0, 0}, {0, 1}, {0, 2}};
    EXPECT_EQ(findPath(maze, {0, 0}, {0, 2}), expected);
}

TEST(PathfindingTest, RepeatedSearchesPreserveCellDataAndPassages) {
    Grid<std::string> maze(2, 2);
    maze.at({0, 0}) = "start";
    maze.at({0, 1}) = "finish";
    maze.openPassage({0, 0}, {0, 1});
    const auto first = findPath(maze, {0, 0}, {0, 1});
    EXPECT_EQ(findPath(maze, {0, 0}, {0, 1}), first);
    EXPECT_EQ(maze.at({0, 0}), "start");
    EXPECT_EQ(maze.at({0, 1}), "finish");
    EXPECT_TRUE(maze.hasPassage({0, 0}, {0, 1}));
    EXPECT_FALSE(maze.hasPassage({0, 0}, {1, 0}));
}
