cmake_minimum_required(VERSION 3.16)
project(Aquilon CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

if(MSVC)
    add_compile_options(/MP)
endif()

set(SDLIMAGE_BACKEND_WIC OFF CACHE BOOL "Use Windows Imaging Component backend for SDL_image" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "Use stb_image backend for SDL_image" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "Disable AVIF support" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "Disable WEBP support" FORCE)
set(SDLIMAGE_SVG OFF CACHE BOOL "Disable SVG support" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "Disable TIFF support" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "Disable JXL support" FORCE)
set(SDLIMAGE_XCF OFF CACHE BOOL "Disable XCF support" FORCE)
set(SDLIMAGE_XPM OFF CACHE BOOL "Disable XPM support" FORCE)
set(SDLIMAGE_XV OFF CACHE BOOL "Disable XV support" FORCE)
set(SDLIMAGE_LBM OFF CACHE BOOL "Disable LBM support" FORCE)
set(SDLIMAGE_PNM OFF CACHE BOOL "Disable PNM support" FORCE)
set(SDLIMAGE_PCX OFF CACHE BOOL "Disable PCX support" FORCE)
set(SDLIMAGE_QOI OFF CACHE BOOL "Disable QOI support" FORCE)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.10
)

FetchContent_Declare(
    SDL_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.4.4
)

FetchContent_Declare(
    SDL_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.2
)

FetchContent_Declare(
    SDL_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-3.2.4
)

FetchContent_MakeAvailable(SDL SDL_image SDL_ttf SDL_mixer)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(Aquilon ${SOURCES})

target_include_directories(Aquilon PRIVATE include)
target_link_libraries(Aquilon PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf SDL3_mixer::SDL3_mixer)

if(WIN32)
    target_link_libraries(Aquilon PRIVATE Ws2_32)
endif()

add_custom_command(TARGET Aquilon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:Aquilon>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:Aquilon>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3_image::SDL3_image>
    $<TARGET_FILE_DIR:Aquilon>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3_ttf::SDL3_ttf>
    $<TARGET_FILE_DIR:Aquilon>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3_mixer::SDL3_mixer>
    $<TARGET_FILE_DIR:Aquilon>
)
