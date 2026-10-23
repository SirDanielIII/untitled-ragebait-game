cmake_minimum_required(VERSION 3.24...3.30)
project(untitled-ragebait-game)

# Require C++20 without compiler-specific language extensions.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Generate compile_commands.json with Makefile and Ninja generators.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prefer an installed raylib package; otherwise fetch the configured release.
set(RAYLIB_VERSION 6.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(raylib)

# Create the executable; src/CMakeLists.txt discovers sources, headers and tests.
add_executable(${PROJECT_NAME})
add_subdirectory(src)

# Register window-free desktop self-tests when BUILD_TESTING is enabled.
include(CTest)
if (BUILD_TESTING AND NOT "${PLATFORM}" STREQUAL "Web")
    add_test(NAME game_self_tests COMMAND $<TARGET_FILE:${PROJECT_NAME}> --self-test)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>)

# Copy runtime resources on every build, including asset-only edits.
# Desktop: beside the executable. Web: in the build root for preloading.
if ("${PLATFORM}" STREQUAL "Web")
    set(GAME_RESOURCE_DESTINATION "${CMAKE_BINARY_DIR}/resources")
else()
    set(GAME_RESOURCE_DESTINATION "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources")
endif()
add_custom_target(stage_game_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "${GAME_RESOURCE_DESTINATION}"
    VERBATIM)
add_dependencies(${PROJECT_NAME} stage_game_resources)

#set(raylib_VERBOSE 1)
target_link_libraries(${PROJECT_NAME} raylib)

# Web builds require an Emscripten toolchain and PLATFORM=Web.
if ("${PLATFORM}" STREQUAL "Web")
    # Emit untitled-ragebait-game.html and preload the staged resources.
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
    target_link_options(${PROJECT_NAME} PUBLIC -sUSE_GLFW=3 PUBLIC --preload-file resources)
endif()

# Link the Apple frameworks on Apple targets.
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()
