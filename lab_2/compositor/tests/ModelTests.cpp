#include "model/Image.hpp"
#include "model/Scene.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

void require(bool condition) {
    if (!condition) {
        throw std::runtime_error("Model check failed");
    }
}

template <typename Exception, typename Function>
void requireThrows(Function operation) {
    try {
        operation();
    } catch (const Exception&) {
        return;
    }
    throw std::runtime_error("Expected exception was not thrown");
}

void checkModel() {
    using namespace compositor;

    const Color background{255, 255, 255};
    const Color red{255, 0, 0};
    require(red.red() == 255 && red.green() == 0 && red.blue() == 0);
    requireThrows<std::invalid_argument>([] { Color{-1, 0, 0}; });
    requireThrows<std::invalid_argument>([] { Color{0, 256, 0}; });
    requireThrows<std::invalid_argument>([] { Color{0, 0, 256}; });

    requireThrows<std::invalid_argument>([&] { Rectangle{0, 0, 0, 1, red, 0}; });
    requireThrows<std::invalid_argument>([&] { Rectangle{0, 0, 1, -1, red, 0}; });
    const Rectangle outside{-10, -20, 3, 5, red, -100};
    require(outside.right() == -7 && outside.bottom() == -15);
    const int maximum = std::numeric_limits<int>::max();
    const Rectangle large{maximum, maximum, maximum, maximum, red, maximum};
    require(large.right() == 2LL * maximum && large.bottom() == 2LL * maximum);

    const Scene scene{3, 2, background, {outside, large}};
    require(scene.rectangles()[0] == outside && scene.rectangles()[1] == large);
    requireThrows<std::invalid_argument>([&] { Scene{0, 1, background, {}}; });
    requireThrows<std::invalid_argument>([&] { Scene{1, 0, background, {}}; });
    requireThrows<std::invalid_argument>([&] { Scene{0, 0, background, {}}; });
    const Scene emptyScene{1, 1, background, {}};
    require(emptyScene.rectangles().empty());
    Image emptyImage{emptyScene.width(), emptyScene.height(), emptyScene.background()};
    require(emptyImage.at(0, 0) == PixelState{background});
    Image image{scene.width(), scene.height(), scene.background()};
    require(image.width() == 3 && image.height() == 2);
    for (std::size_t y = 0; y < image.height(); ++y) {
        for (std::size_t x = 0; x < image.width(); ++x) {
            require(image.colorAt(x, y) == background && !image.at(x, y).winner);
        }
    }

    const PixelState covered{red, RectangleMetadata{-100, 0}};
    image.setPixel(2, 1, covered);
    require(image.at(2, 1) == covered);
    require(image.at(1, 1) == PixelState{background});
    Image copy = image;
    copy.setPixel(2, 1, PixelState{background});
    require(image.at(2, 1) == covered && !copy.at(2, 1).winner);

    requireThrows<std::out_of_range>([&] { image.at(3, 0); });
    requireThrows<std::out_of_range>([&] { image.at(0, 2); });
    requireThrows<std::out_of_range>([&] { image.setPixel(3, 0, covered); });
    requireThrows<std::invalid_argument>([&] { Image{0, 1, background}; });
    requireThrows<std::invalid_argument>([&] { Image{1, 0, background}; });
    requireThrows<std::length_error>([&] {
        Image{std::numeric_limits<std::size_t>::max(), 2, background};
    });
}

} // namespace

int main() {
    try {
        checkModel();
        std::cout << "Model checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
