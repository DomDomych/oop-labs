#include "collection.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <stdexcept>

TEST(CollectionIntegrationTest, EmptyCollectionReturnsEmptyResults) {
    const EquationCollection equations;
    EXPECT_TRUE(equations.solveAll().empty());
    EXPECT_TRUE(equations.evaluateAll(Complex(2, 0)).empty());
    EXPECT_TRUE(equations.findByRoot(Complex(2, 0)).empty());
}

TEST(CollectionIntegrationTest, SolvesMixedEquationsInInsertionOrder) {
    EquationCollection equations;
    equations.add(LinearEquation(Complex(2, 0), Complex(-4, 0)));
    equations.add(QuadraticEquation(Complex(1, 0), Complex(0, 0), Complex(1, 0)));
    equations.add(QuadraticEquation(Complex(1, 0), Complex(-4, 0), Complex(4, 0)));

    auto results = equations.solveAll();
    ASSERT_EQ(results.size(), 3u);
    ASSERT_EQ(results[0].size(), 1u);
    EXPECT_NEAR(results[0][0].real(), 2, 1e-10);
    EXPECT_NEAR(results[0][0].imag(), 0, 1e-10);

    ASSERT_EQ(results[1].size(), 2u);
    const auto [negative, positive] = std::minmax_element(results[1].begin(), results[1].end(),
              [](const Complex& left, const Complex& right) {
                  return left.imag() < right.imag();
              });
    EXPECT_NEAR(negative->real(), 0, 1e-10);
    EXPECT_NEAR(negative->imag(), -1, 1e-10);
    EXPECT_NEAR(positive->real(), 0, 1e-10);
    EXPECT_NEAR(positive->imag(), 1, 1e-10);

    ASSERT_EQ(results[2].size(), 1u);
    EXPECT_NEAR(results[2][0].real(), 2, 1e-10);
    EXPECT_NEAR(results[2][0].imag(), 0, 1e-10);
}

TEST(CollectionIntegrationTest, EvaluatesMixedEquationsAtComplexPoint) {
    EquationCollection equations;
    equations.add(LinearEquation(Complex(2, 0), Complex(-4, 0)));
    equations.add(QuadraticEquation(Complex(1, 0), Complex(0, 0), Complex(1, 0)));

    const auto results = equations.evaluateAll(Complex(1, 1));
    ASSERT_EQ(results.size(), 2u);
    EXPECT_NEAR(results[0].real(), -2, 1e-10);
    EXPECT_NEAR(results[0].imag(), 2, 1e-10);
    EXPECT_NEAR(results[1].real(), 1, 1e-10);
    EXPECT_NEAR(results[1].imag(), 2, 1e-10);
}

TEST(CollectionIntegrationTest, FindsSharedRootAcrossEquationTypes) {
    EquationCollection equations;
    equations.add(LinearEquation(Complex(2, 0), Complex(-4, 0)));
    equations.add(QuadraticEquation(Complex(1, 0), Complex(0, 0), Complex(1, 0)));
    equations.add(QuadraticEquation(Complex(1, 0), Complex(-5, 0), Complex(6, 0)));

    const auto matches = equations.findByRoot(Complex(2, 0));
    ASSERT_EQ(matches.size(), 2u);
    ASSERT_NE(matches[0], nullptr);
    ASSERT_NE(matches[1], nullptr);
    EXPECT_NE(matches[0], matches[1]);
    // Evaluating away from the shared root identifies the returned equations.
    const Complex first = (*matches[0])(Complex(0, 0));
    const Complex second = (*matches[1])(Complex(0, 0));
    EXPECT_NEAR(first.real(), -4, 1e-10);
    EXPECT_NEAR(first.imag(), 0, 1e-10);
    EXPECT_NEAR(second.real(), 6, 1e-10);
    EXPECT_NEAR(second.imag(), 0, 1e-10);
}

TEST(CollectionIntegrationTest, FindsOneComplexRootAndRejectsNonRoot) {
    EquationCollection equations;
    equations.add(LinearEquation(Complex(1, 0), Complex(-2, 0)));
    equations.add(QuadraticEquation(Complex(1, 0), Complex(0, 0), Complex(1, 0)));

    const auto matches = equations.findByRoot(Complex(0, 1));
    ASSERT_EQ(matches.size(), 1u);
    ASSERT_NE(matches[0], nullptr);
    const Complex value = (*matches[0])(Complex(0, 0));
    EXPECT_NEAR(value.real(), 1, 1e-10);
    EXPECT_NEAR(value.imag(), 0, 1e-10);
    EXPECT_TRUE(equations.findByRoot(Complex(7, 0)).empty());
}

TEST(CollectionIntegrationTest, OwnsEquationAfterOriginalLeavesScope) {
    EquationCollection equations;
    {
        const LinearEquation original(Complex(0, 1), Complex(1, 0));
        equations.add(original);
    }

    const auto results = equations.solveAll();
    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 1u);
    EXPECT_NEAR(results[0][0].real(), 0, 1e-10);
    EXPECT_NEAR(results[0][0].imag(), 1, 1e-10);
}

TEST(CollectionIntegrationTest, InvalidEquationDoesNotChangeCollection) {
    EquationCollection equations;
    equations.add(LinearEquation(Complex(2, 0), Complex(-4, 0)));

    EXPECT_THROW(equations.add(LinearEquation(Complex(0, 0), Complex(0, 0))),
                 std::invalid_argument);
    EXPECT_THROW(equations.add(QuadraticEquation(Complex(0, 0), Complex(1, 0),
                                                Complex(2, 0))),
                 std::invalid_argument);

    const auto results = equations.solveAll();
    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 1u);
    EXPECT_NEAR(results[0][0].real(), 2, 1e-10);
    EXPECT_NEAR(results[0][0].imag(), 0, 1e-10);
}
