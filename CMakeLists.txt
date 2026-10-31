cmake_minimum_required(VERSION 3.20)

project(SCSE
    VERSION 0.1.0
    DESCRIPTION "Space Combat Simulation Engine"
    LANGUAGES CXX
)

option(SCSE_BUILD_CONSOLE "Build the sample console front end" ON)

find_package(Lua 5.4 REQUIRED)

add_library(scse_core
    src/engine/combat.cpp
    src/engine/economy.cpp
    src/engine/engine.cpp
    src/engine/math.cpp
    src/engine/movement.cpp
    src/engine/navigation.cpp
    src/engine/world.cpp
    src/lua_rules/read_values.cpp
    src/lua_rules/runtime.cpp
    src/lua_rules/scenario.cpp
    src/console/snapshot.cpp
)

target_include_directories(scse_core PUBLIC src PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(scse_core PUBLIC ${LUA_LIBRARIES})
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
    add_custom_command(TARGET scse POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts
                $<TARGET_FILE_DIR:scse>/scripts
    )
    if(MSVC)
        target_compile_options(scse PRIVATE /W4 /permissive-)
    else()
        target_compile_options(scse PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
