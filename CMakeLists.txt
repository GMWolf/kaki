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
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)

add_subdirectory(external/shaderc)

add_library(spirv-reflect external/SPIRV-Reflect/spirv_reflect.h external/SPIRV-Reflect/spirv_reflect.c)
target_include_directories(spirv-reflect PUBLIC external/SPIRV-Reflect)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE external/rapidjson/include)

add_library(cereal INTERFACE kaki/common/include/overloaded.h)
target_include_directories(cereal INTERFACE external/cereal/include)

add_subdirectory(external/glm)
target_compile_definitions(glm INTERFACE GLM_FORCE_LEFT_HANDED)
target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)

add_subdirectory(external/KTX-Software)


add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE external/cgltf)

add_library(imgui external/imgui/imgui.h
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/imgui_tables.cpp)
target_include_directories(imgui PUBLIC external/imgui)

add_library(imgui_glfw external/imgui/backends/imgui_impl_glfw.h external/imgui/backends/imgui_impl_glfw.cpp)
target_include_directories(imgui_glfw PUBLIC external/imgui/backends)
target_link_libraries(imgui_glfw PRIVATE imgui)

add_library(imgui_vk external/imgui/backends/imgui_impl_vulkan.h external/imgui/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui_vk PUBLIC external/imgui/backends)
target_link_libraries(imgui_vk PRIVATE imgui)

add_library(vk_mini_libs INTERFACE
        external/vulkan-mini-libs-2/include/vk_error_code.hpp
        external/vulkan-mini-libs-2/include/vk_result_to_string.h
        external/vulkan-mini-libs-2/include/vk_struct_cleanup.h
        external/vulkan-mini-libs-2/include/vk_struct_compare.h
        external/vulkan-mini-libs-2/include/vk_value_serialization.hpp)
target_include_directories(vk_mini_libs INTERFACE external/vulkan-mini-libs-2/include)

set(USE_GRAPHICAL_BENCHMARK OFF)
set(BUILD_CPU_DEMOS OFF)
set(BUILD_OPENGL3_DEMOS OFF)
set(BUILD_BULLET2_DEMOS OFF)
set(BUILD_EXTRAS OFF)
set(BUILD_UNIT_TESTS OFF)
add_subdirectory(external/bullet3)

add_subdirectory(kaki)
add_subdirectory(tools)

add_executable(test main.cpp)
target_link_libraries(test PRIVATE kaki-gfx kaki-physics)
target_link_libraries(test PRIVATE flecs_static)

compile_gltf(test-gltf SOURCES SciFiHelmet/SciFiHelmet.gltf)
compile_gltf(test-sponza SOURCES sponza/Sponza.gltf)
compile_gltf(test-dh SOURCES DamagedHelmet/DamagedHelmet.gltf)
compile_pipeline(vispipeline shaders/visibility.json)
compile_pipeline(shadepipeline shaders/shade.json)
compile_pipeline(materialpipeline shaders/materialPass.json)
#add_image(sponzaDiffuseIrradiance sponza/SponzaDiffuseIrradiance.ktx2)
#add_image(sponzaSpecular sponza/sponzaSpecular.ktx2)
add_image(sponzaDiffuseIrradiance SunTemple/templeIrradiance.ktx2)
add_image(sponzaSpecular SunTemple/templeSpec.ktx2)
add_image(brdf_lut brdf_lut.ktx2)
compile_gltf(suntemple SOURCES SunTemple/SunTemple.gltf)
compile_gltf(objects SOURCES physicsAssets/objects.gltf)
make_package(testpackage SOURCES
        SciFiHelmet/SciFiHelmet.gltf.json
        sponza/Sponza.gltf.json
        DamagedHelmet/DamagedHelmet.gltf.json
        vispipeline.json
        shadepipeline.json
        materialpipeline.json
        sponzaDiffuseIrradiance.json
        sponzaSpecular.json
        brdf_lut.json
        #SunTemple/SunTemple.gltf.json
        physicsAssets/objects.gltf.json
        )
add_dependencies(test testpackage)