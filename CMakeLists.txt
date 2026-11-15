cmake_minimum_required(VERSION 3.20)

project(SCSE
    VERSION 0.1.0
    DESCRIPTION "Space Combat Simulation Engine"
    LANGUAGES CXX
)

option(SCSE_BUILD_CONSOLE "Build the sample console front end" ON)
include(CTest)

add_library(scse_core
    src/engine/combat.cpp
    src/engine/economy.cpp
    src/engine/engine.cpp
    src/engine/math.cpp
    src/engine/movement.cpp
    src/engine/navigation.cpp
    src/engine/world.cpp
    src/rules/scenario.cpp
    src/rules/calculations/orbit.cpp
    src/rules/calculations/flight.cpp
    src/rules/calculations/weapon_aim.cpp
    src/console/snapshot.cpp
)

target_include_directories(scse_core PUBLIC src)
target_compile_features(scse_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(scse_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(scse_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(SCSE_BUILD_CONSOLE)
    add_executable(scse
        src/main.cpp
        src/console/command_parser.cpp
        src/console/terminal.cpp
    )
    target_link_libraries(scse PRIVATE scse_core)
    if(MSVC)
        target_compile_options(scse PRIVATE /W4 /permissive-)
    else()
        target_compile_options(scse PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(BUILD_TESTING)
        add_executable(scse_rules_tests tests/rules_test.cpp)
        target_link_libraries(scse_rules_tests PRIVATE scse_core)
        add_test(NAME scse_rules_tests COMMAND scse_rules_tests)
    endif()
endif()
