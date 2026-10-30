#include "application/RenderApplication.hpp"
#include "input/JsonConfigLoader.hpp"
#include "output/PpmWriter.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: compositor <config.json>\n";
        return 2;
    }
    try {
        const auto config = compositor::JsonConfigLoader{}.load(argv[1]);
        const compositor::PpmWriter writer;
        const compositor::RenderApplication application{writer};
        application.run(config);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
