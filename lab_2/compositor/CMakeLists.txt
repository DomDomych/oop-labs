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

find_package(Threads REQUIRED)
add_library(compositor_execution
    src/execution/SequentialExecutor.cpp
    src/execution/ParallelExecutor.cpp
)
target_include_directories(compositor_execution PUBLIC include)
target_compile_features(compositor_execution PUBLIC cxx_std_20)
target_link_libraries(compositor_execution PUBLIC Threads::Threads)
set_target_properties(compositor_execution PROPERTIES CXX_EXTENSIONS OFF)

add_library(compositor_access
    src/synchronization/NoLockPolicy.cpp
    src/synchronization/RegionMutexPolicy.cpp
)
target_include_directories(compositor_access PUBLIC include)
target_compile_features(compositor_access PUBLIC cxx_std_20)
target_link_libraries(compositor_access PUBLIC Threads::Threads)
set_target_properties(compositor_access PROPERTIES CXX_EXTENSIONS OFF)

add_library(compositor_rendering
    src/rendering/RectangleClipper.cpp
    src/rendering/RegionGrid.cpp
    src/rendering/OverlayRule.cpp
    src/rendering/Compositor.cpp
    src/rendering/RectanglePainter.cpp
)
target_link_libraries(compositor_rendering PUBLIC compositor_model compositor_execution compositor_access)
set_target_properties(compositor_rendering PROPERTIES CXX_EXTENSIONS OFF)

include(CTest)
if(BUILD_TESTING)
    add_executable(compositor_model_tests tests/ModelTests.cpp)
    target_link_libraries(compositor_model_tests PRIVATE compositor_model)
    add_test(NAME compositor_model_tests COMMAND compositor_model_tests)
    add_executable(compositor_rendering_tests tests/RenderingTests.cpp)
    target_link_libraries(compositor_rendering_tests PRIVATE compositor_rendering)
    foreach(component IN ITEMS clipping overlay composition repeated_calls)
        add_test(NAME compositor_${component}_tests
                 COMMAND compositor_rendering_tests ${component})
    endforeach()
    add_executable(compositor_parallel_tests tests/ParallelTests.cpp)
    target_link_libraries(compositor_parallel_tests PRIVATE compositor_rendering)
    add_test(NAME compositor_parallel_tests COMMAND compositor_parallel_tests)
    set_tests_properties(compositor_parallel_tests PROPERTIES TIMEOUT 30)
    add_executable(compositor_grid_tests tests/RegionGridTests.cpp)
    target_link_libraries(compositor_grid_tests PRIVATE compositor_rendering)
    add_test(NAME compositor_grid_tests COMMAND compositor_grid_tests)
    add_executable(compositor_transaction_tests tests/RenderTransactionTests.cpp)
    target_link_libraries(compositor_transaction_tests PRIVATE compositor_rendering)
    add_test(NAME compositor_transaction_tests COMMAND compositor_transaction_tests)
endif()
