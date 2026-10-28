#pragma once

#include "complex.hpp"
#include <vector>

class Equation
{
public: 
    virtual ~Equation() = default;

    virtual Complex operator()(const Complex& x)const = 0;
    virtual std::vector<Complex> solve() const = 0;

};

class LinearEquation : public Equation{
    private:
        Complex a_;
        Complex b_;

    public:
        LinearEquation(const Complex& a,const Complex& b);

        Complex operator()(const Complex& x) const override;
        std::vector<Complex> solve() const override;
};

class QuadraticEquation : public Equation {
private:
    Complex a_;
    Complex b_;
    Complex c_;

public:
    QuadraticEquation(
        const Complex& a,
        const Complex& b,
        const Complex& c
    );

    Complex operator()(const Complex& x) const override;
    std::vector<Complex> solve() const override;
};