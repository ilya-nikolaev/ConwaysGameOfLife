cmake_minimum_required(VERSION 3.10)
project(game)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_executable(game main.c)
find_package(SDL2)

set_property(TARGET game PROPERTY C_STANDARD 99)
target_link_libraries(game SDL2main SDL2)
