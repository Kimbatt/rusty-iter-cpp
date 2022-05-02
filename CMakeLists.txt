cmake_minimum_required (VERSION 3.0.0)

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}") 
  message(FATAL_ERROR "In-source builds are disabled.
    Please create a subfolder and use `cmake ..` inside it.
    NOTE: cmake will now create CMakeCache.txt and CMakeFiles/*.
          You must delete them, or cmake will refuse to work.")
endif()

option (COMPILE_WITH_CLANG "COMPILE_WITH_CLANG" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (COMPILE_WITH_CLANG)
    set(CMAKE_GENERATOR_TOOLSET "clangcl")
endif (COMPILE_WITH_CLANG)
# Enable enhanced instruction set:      Advanced Vector Extensions 2 (/arch:AVX2)
# Enable intrinsic functions:           Yes (/Oi)
# Favor size or speed:                  Favor fast code (/Ot)
# Inline function expansion:            Any Suitable (/Ob2)
set(RELEASE_COMPILER_FLAGS         "/arch:AVX2 /Oi /Ot /Ob2")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${RELEASE_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE        "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_COMPILER_FLAGS}")

project (rusty-iter)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/)

add_subdirectory(src)
