#include "collection.hpp"

std::vector<std::vector<Complex>>
EquationCollection::solveAll() const {
    std::vector<std::vector<Complex>> results;

    for (const auto& equation : equations_) {
        results.push_back(equation->solve());
    }

    return results;
}

std::vector<Complex>
EquationCollection::evaluateAll(const Complex& x) const {
    std::vector<Complex> results;

    for (const auto& equation : equations_) {
        results.push_back((*equation)(x));
    }

    return results;
}

std::vector<const Equation*>
EquationCollection::findByRoot(const Complex& x) const {
    std::vector<const Equation*> matches;

    for (const auto& equation : equations_) {
        if ((*equation)(x) == Complex(0, 0)) {
            matches.push_back(equation.get());
        }
    }

    return matches;
}