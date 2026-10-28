#include "cli.hpp"
#include "collection.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
    struct EndOfInput {};

    std::string readLine() {
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw EndOfInput{};
        }
        return line;
    }

    void printMenu() {
        std::cout
            << "\nEquation solver\n"
            << "1. Add a linear equation\n"
            << "2. Add a quadratic equation\n"
            << "3. Solve all equations\n"
            << "4. Evaluate all equations at a point\n"
            << "5. Find equations with a given root\n"
            << "0. Exit\n"
            << "Choose an option: ";
    }

    int readChoice() {
        while (true) {
            std::istringstream input(readLine());
            int choice;
            if ((input >> choice) && (input >> std::ws).eof()) {
                return choice;
            }
            std::cout << "Enter one integer: ";
        }
    }

    Complex readComplex() {
        while (true) {
            std::istringstream input(readLine());
            double real;
            double imag;
            if ((input >> real >> imag) && (input >> std::ws).eof()
                && std::isfinite(real) && std::isfinite(imag)) {
                return Complex(real, imag);
            }
            std::cout << "Enter two finite numbers (real imaginary): ";
        }
    }

    void printComplex(const Complex& value) {
        std::cout << (value.real() == 0 ? 0.0 : value.real());
        if (value.imag() < 0) {
            std::cout << " - " << -value.imag() << 'i';
        } else if (value.imag() > 0) {
            std::cout << " + " << value.imag() << 'i';
        }
    }

    void printRoots(const std::vector<Complex>& roots) {
        for (std::size_t i = 0; i < roots.size(); ++i) {
            if (i != 0) {
                std::cout << ", ";
            }
            printComplex(roots[i]);
        }
        std::cout << '\n';
    }

    Complex readLeadingCoefficient() {
        while (true) {
            std::cout << "Enter a (real imaginary, nonzero): ";
            const Complex a = readComplex();
            if (a != Complex(0, 0)) {
                return a;
            }
            std::cout << "The leading coefficient cannot be zero.\n";
        }
    }

    void addLinearEquation(EquationCollection& equations) {
        std::cout << "Linear equation: a*x + b = 0\n";
        const Complex a = readLeadingCoefficient();
        std::cout << "Enter b (real imaginary): ";
        const Complex b = readComplex();
        equations.add(LinearEquation(a, b));
        std::cout << "Linear equation added.\n";
    }

    void addQuadraticEquation(EquationCollection& equations) {
        std::cout << "Quadratic equation: a*x^2 + b*x + c = 0\n";
        const Complex a = readLeadingCoefficient();
        std::cout << "Enter b (real imaginary): ";
        const Complex b = readComplex();
        std::cout << "Enter c (real imaginary): ";
        const Complex c = readComplex();
        equations.add(QuadraticEquation(a, b, c));
        std::cout << "Quadratic equation added.\n";
    }

    void printSolutions(const EquationCollection& equations) {
        const auto results = equations.solveAll();
        if (results.empty()) {
            std::cout << "No equations added yet.\n";
            return;
        }
        for (std::size_t i = 0; i < results.size(); ++i) {
            std::cout << "Equation #" << i + 1 << " roots: ";
            printRoots(results[i]);
        }
    }

    void evaluateEquations(const EquationCollection& equations) {
        std::cout << "Enter x (real imaginary): ";
        const Complex x = readComplex();
        const auto results = equations.evaluateAll(x);
        if (results.empty()) {
            std::cout << "No equations added yet.\n";
            return;
        }
        for (std::size_t i = 0; i < results.size(); ++i) {
            std::cout << "Equation #" << i + 1 << ": f(x) = ";
            printComplex(results[i]);
            std::cout << '\n';
        }
    }

    void findEquationsByRoot(const EquationCollection& equations) {
        std::cout << "Enter the root (real imaginary): ";
        const Complex x = readComplex();
        const auto matches = equations.findByRoot(x);
        std::cout << "Matching equations: " << matches.size() << '\n';
        for (std::size_t i = 0; i < matches.size(); ++i) {
            std::cout << "Match #" << i + 1 << " roots: ";
            printRoots(matches[i]->solve());
        }
    }
}

void runCli() {
    EquationCollection equations;
    try {
        while (true) {
            printMenu();
            const int choice = readChoice();
            try {
                switch (choice) {
                case 1:
                    addLinearEquation(equations);
                    break;
                case 2:
                    addQuadraticEquation(equations);
                    break;
                case 3:
                    printSolutions(equations);
                    break;
                case 4:
                    evaluateEquations(equations);
                    break;
                case 5:
                    findEquationsByRoot(equations);
                    break;
                case 0:
                    return;
                default:
                    std::cout << "Unknown option. Try again.\n";
                }
            } catch (const std::logic_error& error) {
                std::cout << "Cannot complete operation: " << error.what() << '\n';
            }
        }
    } catch (const EndOfInput&) {
        std::cout << "\nInput closed. Exiting.\n";
    }
}
