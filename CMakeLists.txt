cmake_minimum_required(VERSION 3.17)

project(display VERSION 1.0 LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)

# dependencies
add_subdirectory(dependencies)
find_package(Vulkan REQUIRED)

# Actual code
add_subdirectory(src)
