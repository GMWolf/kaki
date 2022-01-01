cmake_minimum_required(VERSION 3.21)
project(kaki)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(external/flecs)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_USE_WAYLAND OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

add_subdirectory(external/vk-bootstrap)

add_subdirectory(external/SPIRV-Headers)

set(SPIRV_SKIP_EXECUTABLES ON)
set(SPIRV_WERROR OFF)
add_subdirectory(external/SPIRV-Tools)


set(ENABLE_SPVREMAPPER OFF)
set(ENABLE_GLSLANG_BINARIES OFF)
set(ENABLE_GLSLANG_JS OFF)
set(ENABLE_OPT OFF)
set(ENABLE_CTEST OFF)
add_subdirectory(external/glslang)

set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
add_subdirectory(external/shaderc)

add_subdirectory(kaki)
add_subdirectory(shader_compiler)

add_executable(test main.cpp)
target_link_libraries(test PRIVATE kaki-gfx)
target_link_libraries(test PRIVATE flecs)

compile_shader(test-shader SOURCES shader.vert shader.frag)
add_dependencies(test test-shader)
