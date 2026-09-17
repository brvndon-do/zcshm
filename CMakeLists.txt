cmake_minimum_required(VERSION 3.25)

project(zcshm VERSION 0.1.0 DESCRIPTION "educational project by Brandon Do" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_library(zcshm_warnings INTERFACE)

add_subdirectory(src/)
add_subdirectory(apps/)
# TODO: add_subdirectory(tests/)

target_compile_options(zcshm_warnings INTERFACE -Wall -Wextra -Wpedantic)
