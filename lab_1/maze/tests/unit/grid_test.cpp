#include "grid.hpp"

#include <algorithm>
#include <gtest/gtest.h>
#include <string>
#include <limits>

TEST(GridTest, RejectsOverflowingDimensions) {
    const auto maximum = std::numeric_limits<std::size_t>::max();
    EXPECT_THROW((Grid<int>(maximum / 2 + 1, 2)), std::length_error);
    EXPECT_THROW((Grid<int>(2, maximum / 2 + 1)), std::length_error);
}

TEST(GridTest, StoresDimensionsAndChecksBounds) {
    const Grid<int> grid(3, 4);
    EXPECT_EQ(grid.rows(), 3u);
    EXPECT_EQ(grid.columns(), 4u);
    EXPECT_TRUE(grid.contains({0, 0}));
    EXPECT_TRUE(grid.contains({2, 3}));
    EXPECT_FALSE(grid.contains({3, 0}));
    EXPECT_FALSE(grid.contains({0, 4}));
}

TEST(GridTest, RejectsZeroDimensions) {
    EXPECT_THROW((Grid<int>(0, 2)), std::invalid_argument);
    EXPECT_THROW((Grid<int>(2, 0)), std::invalid_argument);
    EXPECT_THROW((Grid<int>(0, 0)), std::invalid_argument);
}

TEST(GridTest, StoresIndependentCellDataOfDifferentTypes) {
    Grid<int> numbers(2, 3);
    numbers.at({1, 2}) = 42;
    const auto& view = numbers;
    EXPECT_EQ(view.at({1, 2}), 42);
    EXPECT_EQ(view.at({0, 2}), 0);

    Grid<std::string> words(2, 2);
    words.at({0, 1}) = "treasure";
    EXPECT_EQ(words.at({0, 1}), "treasure");
    EXPECT_TRUE(words.at({1, 0}).empty());
}

TEST(GridTest, RejectsOutOfBoundsAccessAndPassages) {
    Grid<int> grid(2, 2);
    const auto& view = grid;
    EXPECT_THROW(grid.at({2, 0}), std::out_of_range);
    EXPECT_THROW(view.at({0, 2}), std::out_of_range);
    EXPECT_THROW(grid.neighbors({2, 0}), std::out_of_range);
    EXPECT_THROW(grid.openPassage({0, 0}, {0, 2}), std::out_of_range);
    EXPECT_THROW(grid.openPassage({2, 0}, {0, 0}), std::out_of_range);
    EXPECT_THROW(grid.hasPassage({0, 0}, {2, 0}), std::out_of_range);
}

TEST(GridTest, ReturnsOnlySideAdjacentNeighbors) {
    const Grid<int> grid(3, 4);
    for (std::size_t row = 0; row < grid.rows(); ++row) {
        for (std::size_t column = 0; column < grid.columns(); ++column) {
            const Position cell{row, column};
            const auto neighbors = grid.neighbors(cell);
            std::vector<Position> expected;
            if (row > 0)
                expected.push_back({row - 1, column});
            if (row < 2)
                expected.push_back({row + 1, column});
            if (column > 0)
                expected.push_back({row, column - 1});
            if (column < 3)
                expected.push_back({row, column + 1});
            EXPECT_EQ(neighbors.size(), expected.size());
            EXPECT_TRUE(std::is_permutation(neighbors.begin(), neighbors.end(), expected.begin(),
                                            expected.end()));
        }
    }
    EXPECT_TRUE(Grid<int>(1, 1).neighbors({0, 0}).empty());
}

TEST(GridTest, PassagesStartClosedAndOpenInBothDirections) {
    Grid<int> grid(3, 3);
    const Position center{1, 1};
    for (Position neighbor : grid.neighbors(center)) {
        EXPECT_FALSE(grid.hasPassage(center, neighbor));
        grid.openPassage(neighbor, center);
        EXPECT_TRUE(grid.hasPassage(center, neighbor));
        EXPECT_TRUE(grid.hasPassage(neighbor, center));
        grid.openPassage(center, neighbor);
        EXPECT_TRUE(grid.hasPassage(center, neighbor));
    }
    EXPECT_FALSE(grid.hasPassage({0, 0}, {0, 1}));
}

TEST(GridTest, RejectsNonadjacentPassagesWithoutChangingGrid) {
    Grid<int> grid(2, 3);
    EXPECT_THROW(grid.openPassage({0, 0}, {0, 0}), std::invalid_argument);
    EXPECT_THROW(grid.openPassage({0, 0}, {1, 1}), std::invalid_argument);
    EXPECT_THROW(grid.openPassage({0, 0}, {0, 2}), std::invalid_argument);
    EXPECT_THROW(grid.openPassage({0, 2}, {1, 0}), std::invalid_argument);
    EXPECT_FALSE(grid.hasPassage({0, 0}, {1, 1}));
    EXPECT_FALSE(grid.hasPassage({0, 0}, {0, 0}));
    EXPECT_FALSE(grid.hasPassage({0, 0}, {0, 1}));
}
