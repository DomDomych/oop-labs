#include "complex.hpp"
#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

static_assert(std::is_copy_constructible_v<Complex>);
static_assert(std::is_move_constructible_v<Complex>);
static_assert(!std::is_copy_assignable_v<Complex>);
static_assert(!std::is_move_assignable_v<Complex>);

TEST(ComplexTest, StoresRealAndImaginaryParts) {
    const Complex value(2.5, -3);
    EXPECT_DOUBLE_EQ(value.real(), 2.5);
    EXPECT_DOUBLE_EQ(value.imag(), -3);
}

TEST(ComplexTest, AddsComponents) {
    const Complex result = Complex(2, 3) + Complex(-4, 5);
    EXPECT_NEAR(result.real(), -2, 1e-10);
    EXPECT_NEAR(result.imag(), 8, 1e-10);
}

TEST(ComplexTest, SubtractsComponents) {
    const Complex result = Complex(2, 3) - Complex(-4, 5);
    EXPECT_NEAR(result.real(), 6, 1e-10);
    EXPECT_NEAR(result.imag(), -2, 1e-10);
}

TEST(ComplexTest, MultipliesComplexNumbers) {
    const Complex result = Complex(2, 3) * Complex(-4, 5);
    EXPECT_NEAR(result.real(), -23, 1e-10);
    EXPECT_NEAR(result.imag(), -2, 1e-10);
}

TEST(ComplexTest, DividesComplexNumbers) {
    const Complex result = Complex(3, 4) / Complex(1, -2);
    EXPECT_NEAR(result.real(), -1, 1e-10);
    EXPECT_NEAR(result.imag(), 2, 1e-10);
    const Complex result2 = Complex(1, 0) / Complex(3, 0);
    EXPECT_NEAR(result2.real(), 1.0 / 3.0, 1e-10);
    EXPECT_NEAR(result2.imag(), 0, 1e-10);
}

TEST(ComplexTest, RejectsDivisionByZero) {
    EXPECT_THROW((Complex(1, 2) / Complex(0, 0)), std::domain_error);
}

TEST(ComplexTest, NegatesBothComponents) {
    const Complex result = -Complex(2, -3);
    EXPECT_NEAR(result.real(), -2, 1e-10);
    EXPECT_NEAR(result.imag(), 3, 1e-10);
}

TEST(ComplexTest, EqualityChecksBothComponents) {
    EXPECT_TRUE(Complex(2, 3) == Complex(2, 3));
    EXPECT_FALSE(Complex(2, 3) == Complex(4, 3));
    EXPECT_FALSE(Complex(2, 3) == Complex(2, 4));
    EXPECT_FALSE(Complex(2, 3) != Complex(2, 3));
    EXPECT_TRUE(Complex(2, 3) != Complex(4, 3));
    EXPECT_TRUE(Complex(2, 3) != Complex(2, 4));
}

TEST(ComplexTest, SquareRootHandlesZeroAndRealNumbers) {
    const Complex result = Complex(0, 0).sqrt();
    EXPECT_NEAR(result.real(), 0, 1e-10);
    EXPECT_NEAR(result.imag(), 0, 1e-10);
    const Complex result2 = Complex(4, 0).sqrt();
    EXPECT_NEAR(result2.real(), 2, 1e-10);
    EXPECT_NEAR(result2.imag(), 0, 1e-10);
    const Complex result3 = Complex(-4, 0).sqrt();
    EXPECT_NEAR(result3.real(), 0, 1e-10);
    EXPECT_NEAR(result3.imag(), 2, 1e-10);
}

TEST(ComplexTest, SquareRootHandlesBothImaginarySigns) {
    const Complex result = Complex(3, 4).sqrt();
    EXPECT_NEAR(result.real(), 2, 1e-10);
    EXPECT_NEAR(result.imag(), 1, 1e-10);
    const Complex result2 = Complex(3, -4).sqrt();
    EXPECT_NEAR(result2.real(), 2, 1e-10);
    EXPECT_NEAR(result2.imag(), -1, 1e-10);
}

TEST(ComplexTest, OperationsPreserveOperands) {
    Complex left(3, 4);
    Complex right(1, -2);
    (void)(left + right);
    (void)(left - right);
    (void)(left * right);
    (void)(left / right);
    (void)(-left);
    (void)left.sqrt();
    const Complex result = left;
    EXPECT_NEAR(result.real(), 3, 1e-10);
    EXPECT_NEAR(result.imag(), 4, 1e-10);
    const Complex result2 = right;
    EXPECT_NEAR(result2.real(), 1, 1e-10);
    EXPECT_NEAR(result2.imag(), -2, 1e-10);
}
