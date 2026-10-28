#pragma once

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "equation.hpp"

class EquationCollection {
private:
    std::vector<std::unique_ptr<Equation>> equations_;

public:
    template<std::derived_from<Equation> T>
    void add(T equation) {
        equations_.push_back(
            std::make_unique<T>(std::move(equation))
        );
    }

    std::vector<std::vector<Complex>> solveAll() const;
    std::vector<Complex> evaluateAll(const Complex& x) const;
    std::vector<const Equation*> findByRoot(const Complex& x) const;
};