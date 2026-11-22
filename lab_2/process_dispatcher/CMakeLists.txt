cmake_minimum_required(VERSION 3.20)
project(process_dispatcher VERSION 1.0.0 LANGUAGES CXX)
if(WIN32)
    set(platform Windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(platform Posix)
else()
    message(FATAL_ERROR "Supported platforms: Windows and Linux")
endif()
find_package(Boost 1.83 CONFIG REQUIRED COMPONENTS json filesystem system)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
add_library(dispatcher_core
    src/platform/Paths.cpp src/platform/${platform}.cpp
    src/model/Model.cpp src/execution/RunSession.cpp
    src/logging/BufferedLogger.cpp src/logging/JsonLogFormatter.cpp
    src/logging/FileLogSink.cpp src/logging/StreamLogSink.cpp
    src/logging/LokiLogSink.cpp src/logging/CurlHttpTransport.cpp
    src/output/FileOutputStore.cpp src/output/JsonReportWriter.cpp
    src/input/JsonConfigLoader.cpp src/application/Application.cpp)
target_include_directories(dispatcher_core PUBLIC include PRIVATE src)
target_compile_features(dispatcher_core PUBLIC cxx_std_20)
target_link_libraries(dispatcher_core PUBLIC Boost::json Threads::Threads PRIVATE CURL::libcurl)
add_library(dispatcher_execution
    src/platform/${platform}PipeDrainer.cpp
    src/process/ProcessSession.cpp src/execution/SyncDispatcher.cpp
    src/execution/AsyncDispatcher.cpp)
target_include_directories(dispatcher_execution PUBLIC include PRIVATE src)
target_link_libraries(dispatcher_execution PUBLIC dispatcher_core PRIVATE Boost::filesystem Boost::system)
add_executable(process_dispatcher src/main.cpp src/application/Fixture.cpp)
target_include_directories(process_dispatcher PRIVATE src)
target_link_libraries(process_dispatcher PRIVATE dispatcher_execution)
foreach(target dispatcher_core dispatcher_execution process_dispatcher)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
if(WIN32)
    target_compile_definitions(dispatcher_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
    target_link_libraries(dispatcher_execution PRIVATE ws2_32)
endif()
include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(dispatcher_unit_tests tests/UnitTests.cpp)
    target_include_directories(dispatcher_unit_tests PRIVATE src)
    target_link_libraries(dispatcher_unit_tests PRIVATE dispatcher_execution)
    if(MSVC)
        target_compile_options(dispatcher_unit_tests PRIVATE /utf-8 /permissive-)
    endif()
    add_test(NAME dispatcher_unit COMMAND dispatcher_unit_tests $<TARGET_FILE:process_dispatcher>)
    add_test(NAME dispatcher_integration COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration.py $<TARGET_FILE:process_dispatcher>)
    add_test(NAME dispatcher_reference COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_reference.py --program $<TARGET_FILE:process_dispatcher>
        --report ${CMAKE_CURRENT_BINARY_DIR}/reference-check.json)
    set_tests_properties(dispatcher_unit dispatcher_integration dispatcher_reference PROPERTIES TIMEOUT 60)
endif()
add_custom_target(run_dispatcher_demo
    COMMAND process_dispatcher ${CMAKE_CURRENT_SOURCE_DIR}/configs/example.json
    DEPENDS process_dispatcher USES_TERMINAL VERBATIM)
