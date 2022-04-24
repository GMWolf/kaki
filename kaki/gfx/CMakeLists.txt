
add_library(kaki-gfx-interface INTERFACE
        include/kaki/gfx.h
        include/kaki/window.h
        include/kaki/vk_cereal.h)

target_include_directories(kaki-gfx-interface INTERFACE include)

add_library(kaki-gfx
        src/gfx.cpp
        src/gltf.cpp
        src/gltf.h
        src/image.cpp
        src/image.h
        src/pipeline.cpp
        src/render_graph.cpp
        src/render_graph.h
        src/rg_default.h
        src/vk.h
        src/vma_impl.cpp
        src/window.cpp
        src/imgui_theme.h
        src/entity_tree.h
        src/render_graph_build.cpp
        src/graphScript.h
        src/descriptors.h
        src/descriptors.cpp
        src/renderpass.h
        src/geometry_buffer.h
        src/geometry_buffer.cpp
        src/material.h
        src/material.cpp
        src/probe.h
        src/allocator.h
        src/allocator.cpp
        src/upload.h src/upload.cpp)

target_link_libraries(kaki-gfx PUBLIC kaki-gfx-interface kaki-core kaki-asset kaki-ecs)
target_link_libraries(kaki-gfx PUBLIC flecs_static cereal glm)
target_link_libraries(kaki-gfx PRIVATE vulkan vk-bootstrap glfw VulkanMemoryAllocator rapidjson ktx_read imgui imgui_glfw imgui_vk)

target_compile_options(ktx_read PRIVATE -O3)

target_include_directories(kaki-gfx
        PUBLIC include
        PRIVATE include/kaki)