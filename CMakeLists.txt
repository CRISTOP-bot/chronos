cmake_minimum_required(VERSION 3.20)
project(chronos VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHRONOS_BUILD_TESTS "Build tests" ON)
option(CHRONOS_BUILD_TOOLS "Build CLI tools" ON)
option(CHRONOS_BUILD_GUI "Build GUI (Qt6)" OFF)

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror=return-type)
endif()

# Core library
add_subdirectory(core)
add_subdirectory(instrumentation)

if(CHRONOS_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(CHRONOS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(CHRONOS_BUILD_GUI)
  add_subdirectory(gui)
endif()

option(CHRONOS_BUILD_BENCHMARKS "Build benchmarks" ON)
if(CHRONOS_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
