# CMakeLists.txt: CMake project to build and configure the Music Visualizer project.

cmake_minimum_required(VERSION 3.21)

project("MusicVisualizer" VERSION 4.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(CPack)

add_subdirectory(Libraries)

add_subdirectory(AudioProc)

# Generate configuration header.
configure_file(CMake/config.h.in config.h)

# Add the main module
add_subdirectory("MusicVisualizer")

install(FILES Shaders/vertex.glsl Shaders/fragment.glsl DESTINATION Shaders COMPONENT Visualizer)