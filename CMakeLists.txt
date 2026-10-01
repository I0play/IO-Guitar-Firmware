cmake_minimum_required(VERSION 3.16)
project(io_guitar_firmware_v4_4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reasonable warnings for host builds; ESP-IDF will override for target
if(NOT CMAKE_CROSSCOMPILING)
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

enable_testing()

add_library(io_guitar_core
  src/dsp.cpp
  src/teaching.cpp
  src/ui.cpp
  src/lesson_data.cpp
  src/runtime.cpp
  src/music.cpp
)
target_include_directories(io_guitar_core PUBLIC include)

add_executable(io_guitar_tests tests/test_all.cpp)
target_link_libraries(io_guitar_tests PRIVATE io_guitar_core)

add_test(NAME io_guitar_tests COMMAND io_guitar_tests)

# Optional: release-style optimisations for host sanity checks
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(io_guitar_core PRIVATE -O2)
endif()
