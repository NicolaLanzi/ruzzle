cmake_minimum_required(VERSION 3.20)
project(ruzzle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Genera compile_commands.json per clangd/IDE.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Evita di compilare esempi e giochi di raylib.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

# Usa la copia locale di raylib se presente, altrimenti la scarica.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/raylib-master/CMakeLists.txt")
    add_subdirectory(vendor/raylib-master EXCLUDE_FROM_ALL)
else()
    set(RAYLIB_VERSION 5.5)
    FetchContent_Declare(
            raylib
            URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(${PROJECT_NAME})
add_subdirectory(src)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Solo su macOS servono questi framework.
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            "-framework IOKit"
            "-framework Cocoa"
            "-framework OpenGL")
endif()

# Copia il dizionario accanto all'eseguibile.
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
