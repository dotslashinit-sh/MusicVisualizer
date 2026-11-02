list(APPEND MusicVisualizer_SRCS
    src/main.cpp
    src/VisualVisualizer.cpp
    src/VisualVisualizer.hpp
    src/Window.hpp
    src/Window.cpp
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(MusicVisualizer ${MusicVisualizer_SRCS})
target_link_libraries(MusicVisualizer PRIVATE glad glfw AudioProc)

install(TARGETS MusicVisualizer DESTINATION bin)