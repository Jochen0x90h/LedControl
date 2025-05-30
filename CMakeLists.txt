cmake_minimum_required(VERSION 3.19)

# set project
project(LedControl)

# configure languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)

# build type
message("*** Type: ${CMAKE_BUILD_TYPE}${CMAKE_CONFIGURATION_TYPES}")
add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

# compiler
message("*** Compiler: ${CMAKE_CXX_COMPILER_ID}")
message("*** Crosscompiling: ${CMAKE_CROSSCOMPILING}")

# enable address sanitizer
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

# debug
#set(CMAKE_CXX_FLAGS_RELEASE "-g -Og")
#set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

# platform
#message("*** OS: ${OS}")
message("*** Platform: ${PLATFORM}")

# dependencies
find_package(coco CONFIG)
find_package(coco-loop CONFIG)
find_package(coco-device CONFIG)
find_package(coco-led-strip CONFIG)
find_package(coco-spi CONFIG)
find_package(coco-mono-display CONFIG)
find_package(coco-ir CONFIG)
find_package(coco-flash CONFIG)
find_package(coco-storage CONFIG)
find_package(coco-math CONFIG)
find_package(coco-devboards CONFIG)

# source
add_subdirectory(src)

# generator
if(${PLATFORM} STREQUAL "native" OR ${PLATFORM} STREQUAL "emu")
	add_subdirectory(generator)
endif()

# check if we are on a "normal" operating system such as Windows or Linux
if(NOT ${CMAKE_CROSSCOMPILING})
    find_package(GTest CONFIG)
    enable_testing()

    add_subdirectory(test)
endif()
