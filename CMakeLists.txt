cmake_minimum_required(VERSION 3.14)
project(DigiScrap CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- OpenGL Dependencies: GLFW & GLAD ----
find_package(OpenGL REQUIRED)

# Attempt to find installed GLFW first; fall back to FetchContent for zero-config team setup
find_package(glfw3 3.3 CONFIG QUIET)
if(NOT TARGET glfw AND NOT TARGET glfw3)
    message(STATUS "GLFW not found locally. Fetching official GLFW repository via FetchContent...")
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()

if(TARGET glfw3 AND NOT TARGET glfw)
    add_library(glfw ALIAS glfw3)
endif()

# GLAD library (OpenGL 3.3 Core profile loader)
add_library(glad STATIC
    extern/glad/src/glad.c
    extern/glad/include/glad/glad.h
    extern/glad/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC extern/glad/include)

# ---- Software Scrapbook Modules ----
add_library(scrapbook_modules
    src/lines/lines.cpp
    src/polygons/polygons.cpp
    src/clipping/clipping.cpp
    src/circles/circles.cpp
    src/curves/curves.cpp
    src/colour/colour.cpp
    src/easing/easing.cpp
    src/paths/paths.cpp
    src/transitions/transitions.cpp
    src/animation/exporter.cpp
    src/animation/demo_sequences.cpp
    src/stickers/demo_drag.cpp
    src/textures/image_loader.cpp
    src/textures/font_atlas.cpp
    src/textures/filter_mirror.cpp
)
target_include_directories(scrapbook_modules PUBLIC src)
target_compile_definitions(scrapbook_modules PUBLIC _USE_MATH_DEFINES)

# ---- Hardware OpenGL Renderer Library ----
add_library(opengl_renderer STATIC
    src/opengl/window.cpp
    src/opengl/shader.cpp
    src/opengl/renderer.cpp
    src/opengl/texture_renderer.cpp
)
target_include_directories(opengl_renderer PUBLIC src extern/glad/include)
target_link_libraries(opengl_renderer PUBLIC glad glfw OpenGL::GL scrapbook_modules)

# ---- Main Executable ----
add_executable(scrapbook src/main.cpp)
target_link_libraries(scrapbook PRIVATE opengl_renderer scrapbook_modules)

# ---- Test Executables ----
add_executable(polygons_test src/polygons/test_polygons.cpp)
target_link_libraries(polygons_test PRIVATE scrapbook_modules)

add_executable(clipping_colour_test src/clipping/test_clipping_colour.cpp)
target_link_libraries(clipping_colour_test PRIVATE scrapbook_modules)

add_executable(lines_circles_test src/circles/test_lines_circles.cpp)
target_link_libraries(lines_circles_test PRIVATE scrapbook_modules)

add_executable(animation_test src/animation/test_animation_core.cpp src/animation/exporter.cpp)
target_link_libraries(animation_test PRIVATE scrapbook_modules)

add_executable(sticker_test src/stickers/test_stickers.cpp)
target_link_libraries(sticker_test PRIVATE scrapbook_modules)

add_executable(easing_paths_test src/easing/test_easing_paths.cpp)
target_link_libraries(easing_paths_test PRIVATE scrapbook_modules)

add_executable(textures_test src/textures/test_textures.cpp)
target_link_libraries(textures_test PRIVATE scrapbook_modules)

add_executable(opengl_test src/opengl/test_opengl.cpp)
target_link_libraries(opengl_test PRIVATE opengl_renderer scrapbook_modules)

# ---- Testing & Targets ----
enable_testing()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output")

add_test(NAME polygons COMMAND polygons_test)
add_test(NAME clipping_colour COMMAND clipping_colour_test)
add_test(NAME lines_circles COMMAND lines_circles_test)
add_test(NAME animation COMMAND animation_test)
add_test(NAME stickers COMMAND sticker_test)
add_test(NAME easing_paths COMMAND easing_paths_test)
add_test(NAME textures COMMAND textures_test)
add_test(NAME opengl COMMAND opengl_test)

set_tests_properties(polygons clipping_colour lines_circles animation stickers easing_paths textures opengl PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_target(check
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS polygons_test clipping_colour_test lines_circles_test animation_test sticker_test easing_paths_test textures_test opengl_test
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

add_custom_target(run
    COMMAND $<TARGET_FILE:scrapbook>
    DEPENDS scrapbook
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
