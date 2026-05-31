cmake_minimum_required(VERSION 3.16)
project(Aquilon CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/SDL)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(Aquilon ${SOURCES})

target_link_libraries(Aquilon PRIVATE SDL3::SDL3)