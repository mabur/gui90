cmake_minimum_required(VERSION 3.5)
project(gui90_example_sdl)
find_package(SDL2 CONFIG REQUIRED)
add_subdirectory(.. gui90build)
FILE(GLOB source "*.cpp" "*.hpp")
add_executable(gui90_example_sdl ${source})
target_link_libraries(gui90_example_sdl PRIVATE SDL2::SDL2 gui90)
target_include_directories(gui90_example_sdl PRIVATE ..)
target_compile_features(gui90_example_sdl PRIVATE cxx_std_17)
