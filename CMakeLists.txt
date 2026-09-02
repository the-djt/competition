cmake_minimum_required(VERSION 3.24)

project(BattlefrontBreakout VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    URL https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/6.0
    URL_HASH SHA256=2b3ee1e2120c7a0796b33062c7e9a694dd8a8caa56a96319ac8c8ecf54a90d0b
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
FetchContent_MakeAvailable(raylib)

add_library(game_core STATIC
    src/core/game_core.cpp
    src/core/profile_store.cpp
)
target_include_directories(game_core PUBLIC src)

add_executable(battlefront_breakout WIN32
    src/main.cpp
    src/app/game_app.cpp
)
target_include_directories(battlefront_breakout PRIVATE src)
target_link_libraries(battlefront_breakout PRIVATE game_core raylib)
set_target_properties(battlefront_breakout PROPERTIES OUTPUT_NAME "BattlefrontBreakout")

if(MSVC)
    target_compile_options(game_core PRIVATE /W4 /utf-8)
    target_compile_options(battlefront_breakout PRIVATE /W4 /utf-8)
else()
    target_compile_options(game_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(battlefront_breakout PRIVATE -Wall -Wextra -Wpedantic -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

add_custom_command(TARGET battlefront_breakout POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:battlefront_breakout>/assets"
    COMMENT "Copying runtime assets"
)

enable_testing()
add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE game_core)
if(MSVC)
    target_compile_options(core_tests PRIVATE /W4 /utf-8)
else()
    target_compile_options(core_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME core_rules COMMAND core_tests)
