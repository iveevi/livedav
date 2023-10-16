cmake_minimum_required(VERSION 3.20)

project(rawcava C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused")

file(GLOB_RECURSE SOURCES "rawcava/*.c")
add_executable(rawcava ${SOURCES})

target_compile_definitions(rawcava PRIVATE PACKAGE="rawcava")

include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(rawcava PRIVATE m pthread fftw3 pulse pulse-simple)
