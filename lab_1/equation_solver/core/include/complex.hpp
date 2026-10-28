#pragma once


class Complex
{
    private:
        const double re_;
        const double im_;

    public:
        Complex(double real,double imag);

        Complex(const Complex&) = default;
        Complex(Complex&&) = default;
        Complex& operator=(const Complex&) = delete;
        Complex& operator=(Complex&&) = delete;

        double real() const;
        double imag() const;

        Complex operator+(const Complex& other) const;
        Complex operator-(const Complex& other) const;
        Complex operator*(const Complex& other) const;
        Complex operator/(const Complex& other) const;
        Complex operator-() const;
        Complex sqrt() const;

        bool operator==(const Complex& other) const;
        bool operator!=(const Complex& other) const;
};
