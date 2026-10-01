cmake_minimum_required(VERSION 3.16)
project(io_guitar VERSION 4.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(io_guitar
    src/dsp.cpp
    src/music.cpp
    src/runtime.cpp
    src/teaching.cpp
    src/ui.cpp
    src/lesson_data.cpp
)

target_include_directories(io_guitar PUBLIC include)

# Optional: unit tests
option(IO_GUITAR_BUILD_TESTS "Build unit tests" ON)
if(IO_GUITAR_BUILD_TESTS)
    enable_testing()
    add_executable(test_all tests/test_all.cpp)
    target_link_libraries(test_all PRIVATE io_guitar)
    add_test(NAME all COMMAND test_all)
endif()
