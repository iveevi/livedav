cmake_minimum_required(VERSION 3.20)

project(livedav CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused")

file(GLOB_RECURSE SOURCES "livedav/*.cpp")
add_executable(livedav ${SOURCES})

include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(livedav PRIVATE m pthread fftw3 pulse pulse-simple)
