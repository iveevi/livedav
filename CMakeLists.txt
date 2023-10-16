cmake_minimum_required(VERSION 3.20)

project(rawcava C)

set(CMAKE_C_STANDARD 11)

add_executable(rawcava
	cava.c
	cavacore.c
	config.c
	input/pulse.c
	input/common.c
	output/raw.c
)

include_directories(${PROJECT_SOURCE_DIR})

# Macro
target_compile_definitions(rawcava PRIVATE PACKAGE="rawcava")

target_link_libraries(rawcava PRIVATE
	m pthread fftw3 pulse pulse-simple iniparser
)
