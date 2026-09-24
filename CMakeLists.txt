cmake_minimum_required(VERSION 3.20)

project(neonspasim
    VERSION 0.1.0
    DESCRIPTION "A structured C++ port of SPASIM"
    LANGUAGES CXX
)

add_executable(neonspasim
    src/main.cpp
    src/spasim/command.cpp
    src/spasim/console.cpp
    src/spasim/simulation.cpp
    src/spasim/world.cpp
)

target_include_directories(neonspasim PRIVATE src)
target_compile_features(neonspasim PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(neonspasim PRIVATE /W4 /permissive-)
else()
    target_compile_options(neonspasim PRIVATE -Wall -Wextra -Wpedantic)
endif()
