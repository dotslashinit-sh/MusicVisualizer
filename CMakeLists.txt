# CMakeLists.txt: CMake project to build and configure the Music Visualizer project.

cmake_minimum_required(VERSION 3.21)

project("MusicVisualizer" VERSION 4.0)

include(CPack)

add_subdirectory(Libraries)

add_subdirectory(AudioProc)

# Generate configuration header.
configure_file(CMake/config.h.in config.h)

add_subdirectory("MusicVisualizer")

# # Add the main module.
# add_subdirectory("Visualizer")