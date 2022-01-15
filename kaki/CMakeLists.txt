add_library(kaki-gfx
        gfx.h
        gfx.cpp
        vk.h
        window.h
        window.cpp
        shader.h shader.cpp pipeline.h pipeline.cpp image.h image.cpp asset.h asset.cpp)

target_link_libraries(kaki-gfx PUBLIC flecs cereal glm)
target_link_libraries(kaki-gfx PRIVATE vulkan vk-bootstrap glfw VulkanMemoryAllocator rapidjson ktx_read)
