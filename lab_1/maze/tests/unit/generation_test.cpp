#include "algorithms/generation.hpp"

#include <gtest/gtest.h>
#include <queue>
#include <string>

TEST(GenerationTest, RejectsDimensionsSmallerThanTwo) {
    std::mt19937 random(42);
    EXPECT_THROW(generateMaze<int>(0, 2, random), std::invalid_argument);
    EXPECT_THROW(generateMaze<int>(1, 3, random), std::invalid_argument);
    EXPECT_THROW(generateMaze<int>(3, 1, random), std::invalid_argument);
}

TEST(GenerationTest, ProducesConnectedAcyclicMazes) {
    // Check connectivity independently of the production findPath algorithm.
    for (unsigned seed : {0u, 42u, 123u}) {
        for (Position size : {Position{2, 2}, Position{2, 7}, Position{7, 2}, Position{5, 8}}) {
            SCOPED_TRACE(::testing::Message()
                         << "seed=" << seed << ", size=" << size.row << 'x' << size.column);
            std::mt19937 random(seed);
            const auto maze = generateMaze<int>(size.row, size.column, random);
            ASSERT_EQ(maze.rows(), size.row);
            ASSERT_EQ(maze.columns(), size.column);
            std::vector<std::vector<bool>> visited(size.row, std::vector<bool>(size.column, false));
            std::queue<Position> pending;
            pending.push({0, 0});
            visited[0][0] = true;
            std::size_t reached = 0;
            std::size_t directedEdges = 0;
            while (!pending.empty()) {
                const Position cell = pending.front();
                pending.pop();
                ++reached;
                for (Position next : maze.neighbors(cell)) {
                    if (!maze.hasPassage(cell, next))
                        continue;
                    ++directedEdges;
                    EXPECT_TRUE(maze.hasPassage(next, cell));
                    if (!visited[next.row][next.column]) {
                        visited[next.row][next.column] = true;
                        pending.push(next);
                    }
                }
            }
            const auto cells = size.row * size.column;
            EXPECT_EQ(reached, cells);
            // A connected undirected graph with V-1 edges is a tree.
            EXPECT_EQ(directedEdges, 2 * (cells - 1));
        }
    }
}

TEST(GenerationTest, SameSeedReproducesPassagesWithinTheSameBuild) {
    std::mt19937 firstRandom(42);
    std::mt19937 secondRandom(42);
    const auto first = generateMaze<int>(4, 6, firstRandom);
    const auto second = generateMaze<int>(4, 6, secondRandom);
    for (std::size_t row = 0; row < first.rows(); ++row) {
        for (std::size_t column = 0; column < first.columns(); ++column) {
            Position cell{row, column};
            for (Position next : first.neighbors(cell)) {
                EXPECT_EQ(first.hasPassage(cell, next), second.hasPassage(cell, next));
            }
        }
    }
}

TEST(GenerationTest, DoesNotUseCellDataForBookkeeping) {
    std::mt19937 random(42);
    const auto maze = generateMaze<std::string>(3, 4, random);
    for (std::size_t row = 0; row < maze.rows(); ++row) {
        for (std::size_t column = 0; column < maze.columns(); ++column) {
            EXPECT_TRUE(maze.at({row, column}).empty());
        }
    }
}
