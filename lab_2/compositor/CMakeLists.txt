cmake_minimum_required(VERSION 3.16)
project(compositor LANGUAGES CXX)

add_library(compositor_model
    src/model/Color.cpp
    src/model/Rectangle.cpp
    src/model/Image.cpp
    src/model/Scene.cpp
)
target_include_directories(compositor_model PUBLIC include)
target_compile_features(compositor_model PUBLIC cxx_std_20)
set_target_properties(compositor_model PROPERTIES CXX_EXTENSIONS OFF)

add_library(compositor_rendering
    src/rendering/RectangleClipper.cpp
    src/rendering/OverlayRule.cpp
    src/rendering/Compositor.cpp
)
target_link_libraries(compositor_rendering PUBLIC compositor_model)
set_target_properties(compositor_rendering PROPERTIES CXX_EXTENSIONS OFF)

include(CTest)
if(BUILD_TESTING)
    add_executable(compositor_model_tests tests/ModelTests.cpp)
    target_link_libraries(compositor_model_tests PRIVATE compositor_model)
    add_test(NAME compositor_model_tests COMMAND compositor_model_tests)
    add_executable(compositor_rendering_tests tests/RenderingTests.cpp)
    target_link_libraries(compositor_rendering_tests PRIVATE compositor_rendering)
    foreach(component IN ITEMS clipping overlay composition)
        add_test(NAME compositor_${component}_tests
                 COMMAND compositor_rendering_tests ${component})
    endforeach()
endif()
