#include "complex.hpp"
#include <stdexcept>
#include <cmath>

Complex::Complex(double real, double imag)
    : re_(real), im_(imag) {}

double Complex::real() const {
    return re_;
}

double Complex::imag() const {
    return im_;
}

Complex Complex::operator+(const Complex& other) const {
    return Complex(re_ + other.real(), im_ + other.imag());
}

Complex Complex::operator-(const Complex& other) const {
    return Complex(re_ - other.real(), im_ - other.imag());
}

Complex Complex::operator*(const Complex& other) const {
    return Complex(
        re_ * other.real() - im_ * other.imag(),
        re_ * other.imag() + im_ * other.real()
    );
}

Complex Complex::operator/(const Complex& other) const {
    if (other.real() == 0 && other.imag() == 0) {
        throw std::domain_error("Division by zero");
    }

    double denominator =
        other.real() * other.real() + other.imag() * other.imag();

    return Complex(
        (re_ * other.real() + im_ * other.imag()) / denominator,
        (im_ * other.real() - re_ * other.imag()) / denominator
    );
}

Complex Complex::operator-() const {
    return Complex(-re_, -im_);
}

bool Complex::operator==(const Complex& other) const {
    return re_ == other.real() && im_ == other.imag();
}

bool Complex::operator!=(const Complex& other) const {
    return !(*this == other);
}

Complex Complex::sqrt() const
{
    const double hypot = std::hypot(re_,im_);
    const double real = std::sqrt((hypot+re_)/2.0);
    const double imag = std::sqrt((hypot-re_)/2.0);

    return Complex(real,im_<0? -imag:imag);
}