#include "rendering.hpp"

#include <gtest/gtest.h>

TEST(RenderingTest, RejectsOutOfBoundsPath) {
    const Grid<int> maze(2, 2);
    EXPECT_THROW(renderMaze(maze, {{2, 0}}), std::out_of_range);
    EXPECT_THROW(renderMaze(maze, {{0, 0}, {0, 2}}), std::out_of_range);
}

TEST(RenderingTest, RejectsPathsThroughWallsOrNonadjacentCells) {
    const Grid<int> maze(2, 3);
    EXPECT_THROW(renderMaze(maze, {{0, 0}, {0, 1}}), std::invalid_argument);
    EXPECT_THROW(renderMaze(maze, {{0, 0}, {1, 1}}), std::invalid_argument);
    EXPECT_THROW(renderMaze(maze, {{0, 0}, {0, 2}}), std::invalid_argument);
    EXPECT_THROW(renderMaze(maze, {{0, 0}, {0, 0}}), std::invalid_argument);
}

TEST(RenderingTest, DrawsClosedGrid) {
    const Grid<int> maze(2, 2);
    EXPECT_EQ(renderMaze(maze), "#####\n# # #\n#####\n# # #\n#####\n");
}

TEST(RenderingTest, DrawsHorizontalAndVerticalPassages) {
    Grid<int> maze(2, 2);
    maze.openPassage({0, 0}, {0, 1});
    maze.openPassage({0, 1}, {1, 1});
    EXPECT_EQ(renderMaze(maze), "#####\n#   #\n### #\n# # #\n#####\n");
}

TEST(RenderingTest, MarksRouteCellsAndConnectingPassages) {
    Grid<int> maze(2, 2);
    maze.openPassage({0, 0}, {0, 1});
    maze.openPassage({0, 1}, {1, 1});
    const std::vector<Position> path{{0, 0}, {0, 1}, {1, 1}};
    EXPECT_EQ(renderMaze(maze, path), "#####\n#S..#\n###.#\n# #E#\n#####\n");
    const std::vector<Position> reverse(path.rbegin(), path.rend());
    EXPECT_EQ(renderMaze(maze, reverse), "#####\n#E..#\n###.#\n# #S#\n#####\n");
}

TEST(RenderingTest, MarksCoincidentEndpoints) {
    const Grid<int> maze(2, 2);
    EXPECT_EQ(renderMaze(maze, {{0, 0}}), "#####\n#B# #\n#####\n# # #\n#####\n");
}
