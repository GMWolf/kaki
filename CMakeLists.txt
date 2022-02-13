cmake_minimum_required(VERSION 3.21)
project(kaki)

set(CMAKE_CXX_STANDARD 23)

set(FLECS_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/flecs)

add_subdirectory(external/flatbuffers)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_USE_WAYLAND OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

add_subdirectory(external/vk-bootstrap)

add_subdirectory(external/VulkanMemoryAllocator)

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

add_library(spirv-reflect external/SPIRV-Reflect/spirv_reflect.h external/SPIRV-Reflect/spirv_reflect.c)
target_include_directories(spirv-reflect PUBLIC external/SPIRV-Reflect)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE external/rapidjson/include)

add_library(cereal INTERFACE kaki/common/include/overloaded.h)
target_include_directories(cereal INTERFACE external/cereal/include)

add_subdirectory(external/glm)
target_compile_definitions(glm INTERFACE GLM_FORCE_QUAT_DATA_XYZW)

add_subdirectory(external/KTX-Software)

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE external/cgltf)

add_subdirectory(kaki)
add_subdirectory(shader_compiler)
add_subdirectory(gltf_compiler)
add_subdirectory(packager)

add_executable(test main.cpp)
target_link_libraries(test PRIVATE kaki-gfx)
target_link_libraries(test PRIVATE flecs_static)

compile_shader(test-shader SOURCES shader.vert shader.frag)
compile_gltf(test-gltf SOURCES untitled.gltf SciFiHelmet/SciFiHelmet.gltf)
make_package(test-package SOURCES shader.vert.shd shader.frag.shd SciFiHelmet/SciFiHelmet.gltf.bin)
add_dependencies(test test-gltf test-package)
configure_file(pipeline.json pipeline.json COPYONLY)
configure_file(assets.json assets.json COPYONLY)

add_custom_command(
        OUTPUT kaki.ktx2
        COMMAND toktx --genmipmap kaki.ktx2 ${CMAKE_CURRENT_LIST_DIR}/kaki.png
        DEPENDS kaki.png
        USES_TERMINAL
)

add_custom_command(
        OUTPUT SciFiHelmet/SciFiHelmet_BaseColor.ktx2
        COMMAND toktx --genmipmap SciFiHelmet/SciFiHelmet_BaseColor.ktx2 ${CMAKE_CURRENT_LIST_DIR}/SciFiHelmet/SciFiHelmet_BaseColor.png
        DEPENDS SciFiHelmet/SciFiHelmet_BaseColor.png
        USES_TERMINAL
)

add_custom_target(textures DEPENDS kaki.ktx2 SciFiHelmet/SciFiHelmet_BaseColor.ktx2)
add_dependencies(test textures)