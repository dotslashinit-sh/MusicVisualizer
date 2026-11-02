list(APPEND AudioProc_SRCS
"src/AudioBuffer.hpp"
"src/AudioStream.hpp"
"src/AudioStream.cpp"
"src/Error.hpp"
"src/Error.cpp"
"src/ErrorReporter.hpp"
"src/ErrorReporter.cpp"
"src/Visualizer.hpp"
"src/Visualizer.cpp"
"src/windows/AudioStream.hpp"
"src/windows/AudioStream.cpp"
)

add_library(AudioProc STATIC ${AudioProc_SRCS})

list(APPEND AudioProc_LIBS
    fftw3f
    yaml-cpp
)

target_include_directories(AudioProc PUBLIC src)
target_link_libraries(AudioProc PUBLIC ${AudioProc_LIBS})