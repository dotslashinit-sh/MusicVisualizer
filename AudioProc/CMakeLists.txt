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

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(AudioProc STATIC ${AudioProc_SRCS})
target_link_libraries(AudioProc PUBLIC fftw3f)
target_include_directories(AudioProc PUBLIC src)