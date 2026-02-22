cmake_minimum_required(VERSION 3.20)
project(wandb_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------------------------------------------------------
# Dependencies via FetchContent
# ----------------------------------------------------------------------------
include(FetchContent)

# pybind11 — C++/Python bridge
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
)
FetchContent_MakeAvailable(pybind11)

# GoogleTest — unit testing framework
find_package(GTest CONFIG REQUIRED)

# Python3 — interpreter discovery for embedded use
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# ----------------------------------------------------------------------------
# Sub-directories
# ----------------------------------------------------------------------------
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
