cmake_minimum_required(VERSION 3.16)
project(Aquilon CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
)

FetchContent_MakeAvailable(SDL)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(Aquilon ${SOURCES})
target_link_libraries(Aquilon PRIVATE SDL3::SDL3)