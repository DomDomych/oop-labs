#include "equation.hpp"
#include <stdexcept>

LinearEquation::LinearEquation(const Complex& a,const Complex& b): 
    a_(a),b_(b)
{
    if(a_== Complex(0,0))
    {
        throw std::invalid_argument(
            "Linear equation requires a nonzero leading coefficent"
        );
    }
}

Complex LinearEquation::operator()(const Complex& x) const {
    return a_ * x + b_;
}

std::vector<Complex> LinearEquation::solve() const {
    return {-b_ / a_};
}

QuadraticEquation::QuadraticEquation(
    const Complex& a,
    const Complex& b,
    const Complex& c
)
    : a_(a), b_(b), c_(c)
{
    if (a_ == Complex(0, 0)) {
        throw std::invalid_argument(
            "Quadratic equation requires a nonzero leading coefficient"
        );
    }
}

Complex QuadraticEquation::operator()(const Complex& x) const {
    return a_ * x * x + b_ * x + c_;
}

std::vector<Complex> QuadraticEquation::solve() const {
    const Complex discriminant = b_ * b_ - Complex(4, 0) * a_ * c_;
    const Complex denominator = Complex(2, 0) * a_;

    if (discriminant == Complex(0, 0)) {
        return {-b_ / denominator};
    }

    const Complex sqrtDiscriminant = discriminant.sqrt();

    return {
        (-b_ + sqrtDiscriminant) / denominator,
        (-b_ - sqrtDiscriminant) / denominator
    };
}