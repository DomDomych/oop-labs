#include "equation.hpp"
#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

namespace {
void expectRoots(const Equation& equation, const std::vector<Complex>& expected) {
    const auto actual = equation.solve();
    ASSERT_EQ(actual.size(), expected.size());
    std::vector<bool> matched(actual.size(), false);
    for (const auto& root : expected) {
        bool found = false;
        for (std::size_t i = 0; i < actual.size(); ++i) {
            if (!matched[i] && std::abs(actual[i].real() - root.real()) < 1e-10 &&
                std::abs(actual[i].imag() - root.imag()) < 1e-10) {
                matched[i] = true;
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Missing root: " << root.real() << ", " << root.imag();
    }
    for (const auto& root : actual) {
        const Complex result = equation(root);
        EXPECT_NEAR(result.real(), 0, 1e-10);
        EXPECT_NEAR(result.imag(), 0, 1e-10);
    }
}
}

TEST(LinearEquationTest, RejectsZeroLeadingCoefficient) {
    EXPECT_THROW((LinearEquation(Complex(0, 0), Complex(1, 0))), std::invalid_argument);
    EXPECT_THROW((LinearEquation(Complex(0, 0), Complex(0, 0))), std::invalid_argument);
}

TEST(LinearEquationTest, EvaluatesRealAndComplexPoints) {
    const LinearEquation equation(Complex(2, 0), Complex(-4, 0));
    const Complex result = equation(Complex(3, 0));
    EXPECT_NEAR(result.real(), 2, 1e-10);
    EXPECT_NEAR(result.imag(), 0, 1e-10);
    const Complex result2 = equation(Complex(1, 2));
    EXPECT_NEAR(result2.real(), -2, 1e-10);
    EXPECT_NEAR(result2.imag(), 4, 1e-10);
}

TEST(LinearEquationTest, SolvesRealCoefficients) {
    expectRoots(LinearEquation(Complex(2, 0), Complex(-4, 0)), {Complex(2, 0)});
}

TEST(LinearEquationTest, SolvesComplexCoefficients) {
    expectRoots(LinearEquation(Complex(0, 1), Complex(1, 0)), {Complex(0, 1)});
}

TEST(LinearEquationTest, ReturnsZeroRoot) {
    expectRoots(LinearEquation(Complex(2, 0), Complex(0, 0)), {Complex(0, 0)});
}

TEST(QuadraticEquationTest, RejectsZeroLeadingCoefficient) {
    EXPECT_THROW((QuadraticEquation(Complex(0, 0), Complex(1, 0), Complex(2, 0))),
                 std::invalid_argument);
    EXPECT_THROW((QuadraticEquation(Complex(0, 0), Complex(0, 0), Complex(0, 0))),
                 std::invalid_argument);
}

TEST(QuadraticEquationTest, EvaluatesRealAndComplexPoints) {
    const QuadraticEquation equation(Complex(2, 0), Complex(3, 0), Complex(4, 0));
    const Complex result = equation(Complex(2, 0));
    EXPECT_NEAR(result.real(), 18, 1e-10);
    EXPECT_NEAR(result.imag(), 0, 1e-10);
    const Complex result2 = equation(Complex(0, 1));
    EXPECT_NEAR(result2.real(), 2, 1e-10);
    EXPECT_NEAR(result2.imag(), 3, 1e-10);
}

TEST(QuadraticEquationTest, ReturnsTwoRealRoots) {
    expectRoots(QuadraticEquation(Complex(1, 0), Complex(-5, 0), Complex(6, 0)),
                {Complex(2, 0), Complex(3, 0)});
}

TEST(QuadraticEquationTest, ReturnsRepeatedRootOnce) {
    expectRoots(QuadraticEquation(Complex(1, 0), Complex(-4, 0), Complex(4, 0)),
                {Complex(2, 0)});
}

TEST(QuadraticEquationTest, ReturnsImaginaryRoots) {
    expectRoots(QuadraticEquation(Complex(1, 0), Complex(0, 0), Complex(1, 0)),
                {Complex(0, 1), Complex(0, -1)});
}

TEST(QuadraticEquationTest, SolvesComplexCoefficients) {
    expectRoots(QuadraticEquation(Complex(1, 0), Complex(-1, -1), Complex(0, 1)),
                {Complex(1, 0), Complex(0, 1)});
}

TEST(QuadraticEquationTest, ReturnsZeroRootOnce) {
    expectRoots(QuadraticEquation(Complex(1, 0), Complex(0, 0), Complex(0, 0)),
                {Complex(0, 0)});
}
