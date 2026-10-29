#include "model/Image.hpp"
#include "model/Scene.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

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

    const Scene scene{{outside, large}};
    require(scene.rectangles()[0] == outside && scene.rectangles()[1] == large);
    const Scene emptyScene{{}};
    require(emptyScene.rectangles().empty());
    Image emptyImage{1, 1, background};
    require(emptyImage.at(0, 0) == PixelState{background});
    Image image{3, 2, background};
    require(image.width() == 3 && image.height() == 2);
    for (std::size_t y = 0; y < image.height(); ++y) {
        for (std::size_t x = 0; x < image.width(); ++x) {
            require(image.colorAt(x, y) == background && !image.at(x, y).winner);
        }
    }

    static_assert(!std::is_copy_assignable_v<Image>);
    static_assert(!std::is_move_assignable_v<Image>);
    static_assert(std::is_copy_constructible_v<Image>);
    static_assert(std::is_nothrow_move_constructible_v<Image>);

    Image source{2, 3, red};
    source.setPixel(1, 2, PixelState{background, RectangleMetadata{-7, 8}});
    const Image saved = source;
    Image destination{std::move(source)};
    require(destination.width() == 2 && destination.height() == 3);
    for (std::size_t y = 0; y < 3; ++y) {
        for (std::size_t x = 0; x < 2; ++x) {
            require(destination.at(x, y) == saved.at(x, y));
        }
    }
    require(source.width() == 0 && source.height() == 0);
    requireThrows<std::out_of_range>([&] { source.at(0, 0); });
    requireThrows<std::out_of_range>([&] { source.colorAt(0, 0); });
    requireThrows<std::out_of_range>([&] { source.setPixel(0, 0, PixelState{red}); });
    const Image emptyCopy{source};
    Image emptyMove{std::move(source)};
    require(emptyCopy.width() == 0 && emptyCopy.height() == 0);
    require(emptyMove.width() == 0 && emptyMove.height() == 0);
    requireThrows<std::out_of_range>([&] { emptyCopy.at(0, 0); });
    requireThrows<std::out_of_range>([&] { emptyMove.at(0, 0); });
    source.swap(destination);
    require(source.width() == 2 && source.height() == 3);
    require(source.at(1, 2) == saved.at(1, 2));
    require(destination.width() == 0 && destination.height() == 0);

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
