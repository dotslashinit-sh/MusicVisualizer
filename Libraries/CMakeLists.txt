# CMakeLists.txt: CMake Project to configure all the include directories and targets required for the music visualizer.

# # SFML.
# add_subdirectory("SFML")

# # kissfft
# add_subdirectory("kissfft")

# For fftw
set(CMAKE_POLICY_VERSION_MINIMUM 3.21)

set(BUILD_SHARED_LIBS OFF)

# fftw
set(FFTW_DIR "${CMAKE_CURRENT_LIST_DIR}/fftw-3.3.11")
add_library(fftw3f_imp SHARED IMPORTED)
set_target_properties(fftw3f_imp PROPERTIES IMPORTED_IMPLIB "${FFTW_DIR}/lib/fftw3f.lib"
    IMPORTED_LOCATION "{FFTW_DIR}/bin/fftw3f.dll"
)
add_library(fftw3f INTERFACE)
target_include_directories(fftw3f INTERFACE "${FFTW_DIR}/include")
target_link_libraries(fftw3f INTERFACE fftw3f_imp)
install(IMPORTED_RUNTIME_ARTIFACTS fftw3f_imp RUNTIME DESTINATION bin)

# GLFW
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_TYPE STATIC)
add_subdirectory("GLFW")

# GLAD.
add_subdirectory(glad)