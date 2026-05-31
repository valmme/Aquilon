cmake_minimum_required(VERSION 3.16)
project(Aquilon CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
)

FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.0
)

FetchContent_MakeAvailable(SDL SDL_image)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(Aquilon ${SOURCES})

target_include_directories(Aquilon PRIVATE include)
target_link_libraries(Aquilon PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

add_custom_command(TARGET Aquilon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:Aquilon>/resources
)