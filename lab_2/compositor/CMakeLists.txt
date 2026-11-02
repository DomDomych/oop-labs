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
find_package(Boost 1.75 CONFIG REQUIRED COMPONENTS json)
add_library(compositor_logging
    src/logging/BufferedLogger.cpp
    src/logging/JsonLogFormatter.cpp
    src/logging/StreamLogSink.cpp
    src/logging/FileLogSink.cpp
    src/logging/LokiLogSink.cpp
)
target_include_directories(compositor_logging PUBLIC include)
target_compile_features(compositor_logging PUBLIC cxx_std_20)
target_link_libraries(compositor_logging PUBLIC Threads::Threads PRIVATE Boost::json)
set_target_properties(compositor_logging PROPERTIES CXX_EXTENSIONS OFF)

option(COMPOSITOR_WITH_LOKI_HTTP "Enable libcurl HTTP transport for Loki" OFF)
if(COMPOSITOR_WITH_LOKI_HTTP)
    find_package(CURL REQUIRED)
    add_library(compositor_logging_http src/logging/CurlHttpTransport.cpp)
    target_link_libraries(compositor_logging_http PUBLIC compositor_logging PRIVATE CURL::libcurl)
    set_target_properties(compositor_logging_http PROPERTIES CXX_EXTENSIONS OFF)
endif()

add_library(compositor_input src/input/JsonConfigLoader.cpp src/input/LoggingConfigParser.cpp)
target_link_libraries(compositor_input PUBLIC compositor_model PRIVATE Boost::json)
set_target_properties(compositor_input PROPERTIES CXX_EXTENSIONS OFF)

add_library(compositor_output src/output/PpmWriter.cpp)
target_link_libraries(compositor_output PUBLIC compositor_model)
set_target_properties(compositor_output PROPERTIES CXX_EXTENSIONS OFF)

add_library(compositor_application src/application/RenderApplication.cpp src/application/LoggingSetup.cpp
    src/application/RenderComponents.cpp)
target_link_libraries(compositor_application PUBLIC compositor_rendering compositor_logging)
if(COMPOSITOR_WITH_LOKI_HTTP)
    target_link_libraries(compositor_application PRIVATE compositor_logging_http)
    target_compile_definitions(compositor_application PRIVATE COMPOSITOR_WITH_LOKI_HTTP)
endif()
set_target_properties(compositor_application PROPERTIES CXX_EXTENSIONS OFF)

add_executable(compositor src/main.cpp)
target_link_libraries(compositor PRIVATE compositor_application compositor_input compositor_output)
set_target_properties(compositor PROPERTIES CXX_EXTENSIONS OFF)

# Named launch targets keep the executable's configuration argument explicit in
# CMake-based IDEs and work with both single- and multi-configuration generators.
function(add_compositor_run_target target config_path)
    add_custom_target(${target}
        COMMAND $<TARGET_FILE:compositor> "${CMAKE_CURRENT_SOURCE_DIR}/${config_path}"
        DEPENDS compositor
        USES_TERMINAL
        COMMENT "Running compositor with ${config_path}"
        VERBATIM
    )
endfunction()

add_compositor_run_target(run_compositor_demo configs/example.json)
add_compositor_run_target(run_reference_sequential demo/reference/sequential.json)
add_compositor_run_target(run_reference_parallel_safe demo/reference/parallel-safe.json)

# Configure-time provenance: reconfigure after changing the checkout for a new report.
find_package(Git QUIET)
set(BENCH_REVISION "unknown")
set(BENCH_DIRTY "unknown")
if(Git_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BENCH_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND ${GIT_EXECUTABLE} status --porcelain WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BENCH_STATUS ERROR_QUIET)
    if(BENCH_STATUS)
        set(BENCH_DIRTY "true")
    else()
        set(BENCH_DIRTY "false")
    endif()
endif()
add_library(compositor_benchmark
    src/benchmark/BenchmarkResult.cpp
    src/benchmark/ImageComparator.cpp
    src/benchmark/BenchmarkRunner.cpp
    src/benchmark/SceneGenerator.cpp
    src/benchmark/BenchmarkPlan.cpp
    src/benchmark/JsonBenchmarkReportWriter.cpp
)
target_link_libraries(compositor_benchmark PUBLIC compositor_rendering compositor_input PRIVATE Boost::json)
target_compile_definitions(compositor_benchmark PRIVATE
    COMPOSITOR_BENCH_COMPILER="${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
    COMPOSITOR_BENCH_CONFIGURATION="$<CONFIG>"
    COMPOSITOR_BENCH_REVISION="${BENCH_REVISION}"
    COMPOSITOR_BENCH_DIRTY="${BENCH_DIRTY}"
    COMPOSITOR_BENCH_PLATFORM="${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
set_target_properties(compositor_benchmark PROPERTIES CXX_EXTENSIONS OFF)
add_executable(compositor_bench src/benchmark_main.cpp)
target_link_libraries(compositor_bench PRIVATE compositor_benchmark compositor_application)
set_target_properties(compositor_bench PROPERTIES CXX_EXTENSIONS OFF)

if(BUILD_TESTING)
    add_executable(compositor_reference_tests tests/ReferenceTests.cpp)
    target_link_libraries(compositor_reference_tests PRIVATE compositor_application compositor_input)
    add_test(NAME compositor_reference_tests COMMAND compositor_reference_tests
             ${CMAKE_CURRENT_SOURCE_DIR}/demo/reference)
    add_test(NAME compositor_reference_cli_tests COMMAND ${CMAKE_COMMAND}
             -DPROGRAM=$<TARGET_FILE:compositor>
             -DREFERENCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/demo/reference
             -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/reference-test-output
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/ReferenceCliTests.cmake)
    set_tests_properties(compositor_reference_tests compositor_reference_cli_tests PROPERTIES TIMEOUT 30)
    add_executable(compositor_benchmark_tests tests/BenchmarkTests.cpp)
    target_link_libraries(compositor_benchmark_tests PRIVATE compositor_benchmark compositor_application Boost::json)
    add_test(NAME compositor_benchmark_tests COMMAND compositor_benchmark_tests)
    add_test(NAME compositor_benchmark_cli_tests COMMAND ${CMAKE_COMMAND}
             -DPROGRAM=$<TARGET_FILE:compositor_bench>
             -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/benchmark-test-output
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/BenchmarkCliTests.cmake)
    set_tests_properties(compositor_benchmark_tests compositor_benchmark_cli_tests PROPERTIES TIMEOUT 30)
    add_executable(compositor_logging_tests tests/LoggingTests.cpp)
    target_link_libraries(compositor_logging_tests PRIVATE compositor_logging compositor_input compositor_application Boost::json)
    add_test(NAME compositor_logging_tests COMMAND compositor_logging_tests)
    if(COMPOSITOR_WITH_LOKI_HTTP)
        find_package(Python3 COMPONENTS Interpreter REQUIRED)
        add_test(NAME compositor_loki_http_tests COMMAND ${Python3_EXECUTABLE}
                 ${CMAKE_CURRENT_SOURCE_DIR}/tests/LokiHttpTests.py $<TARGET_FILE:compositor>)
        set_tests_properties(compositor_loki_http_tests PROPERTIES TIMEOUT 30)
    endif()
    add_executable(compositor_application_tests tests/ApplicationTests.cpp)
    target_link_libraries(compositor_application_tests PRIVATE compositor_application compositor_output)
    add_test(NAME compositor_application_tests COMMAND compositor_application_tests)
    add_test(NAME compositor_cli_tests COMMAND ${CMAKE_COMMAND}
             -DPROGRAM=$<TARGET_FILE:compositor>
             -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-test-output
             -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/CliTests.cmake)
    add_executable(compositor_input_tests tests/InputTests.cpp)
    target_link_libraries(compositor_input_tests PRIVATE compositor_input)
    add_test(NAME compositor_input_tests COMMAND compositor_input_tests
             ${CMAKE_CURRENT_SOURCE_DIR}/configs/example.json)
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
