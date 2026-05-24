cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(depths_of_hoenn)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenGL REQUIRED)

add_library(glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    add_subdirectory(3rdParty/glfw)
    include_directories(3rdParty/glfw/include/)
endif()

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdParty/assimp)

include_directories(3rdParty/glad/include/
                    3rdParty/glm/
                    3rdParty/stb/
                    3rdParty/assimp/include)

add_subdirectory(src)