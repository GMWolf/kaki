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

add_subdirectory(kaki)

add_executable(test main.cpp)
target_link_libraries(test PRIVATE flecs glfw vk-bootstrap vulkan)
