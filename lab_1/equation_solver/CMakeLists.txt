cmake_minimum_required(VERSION 3.16)
project(eqsolver LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(eqsolver_core
    core/src/complex.cpp
    core/src/equation.cpp
    core/src/collection.cpp
)
target_include_directories(eqsolver_core PUBLIC core/include)

add_executable(eqsolver
    main.cpp
    cli/src/cli.cpp
)
target_include_directories(eqsolver PRIVATE cli/include)
target_link_libraries(eqsolver PRIVATE eqsolver_core)

if(BUILD_TESTING)
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(eqsolver_tests
        tests/main_test.cpp
        tests/unit/complex_test.cpp
        tests/unit/equation_test.cpp
        tests/integration/collection_test.cpp
    )
    target_link_libraries(eqsolver_tests PRIVATE eqsolver_core GTest::gtest)

    include(GoogleTest)
    gtest_discover_tests(eqsolver_tests)
endif()
