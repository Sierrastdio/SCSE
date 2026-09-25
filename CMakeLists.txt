cmake_minimum_required(VERSION 3.20)

project(neonspasim
    VERSION 0.1.0
    DESCRIPTION "A structured C++ port of SPASIM"
    LANGUAGES CXX
)

option(SPASIM_BUILD_CONSOLE "Build the sample console front end" ON)

add_library(spasim_core
    src/spasim/command.cpp
    src/spasim/simulation.cpp
    src/spasim/simulation_logistics.cpp
    src/spasim/simulation_navigation.cpp
    src/spasim/simulation_physics.cpp
    src/spasim/simulation_snapshot.cpp
    src/spasim/simulation_utilities.cpp
    src/spasim/simulation_weapons.cpp
    src/spasim/world.cpp
)

target_include_directories(spasim_core PUBLIC src)
target_compile_features(spasim_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(spasim_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(spasim_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SPASIM_BUILD_CONSOLE)
    add_executable(neonspasim
        src/main.cpp
        src/spasim/console.cpp
    )
    target_link_libraries(neonspasim PRIVATE spasim_core)
    if(MSVC)
        target_compile_options(neonspasim PRIVATE /W4 /permissive-)
    else()
        target_compile_options(neonspasim PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
