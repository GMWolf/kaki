add_library(kaki-gfx
        gfx.h
        gfx.cpp
        vk.h
        window.h
        window.cpp
        shader.h shader.cpp pipeline.h pipeline.cpp)

target_link_libraries(kaki-gfx PUBLIC flecs cereal)
target_link_libraries(kaki-gfx PRIVATE vulkan vk-bootstrap glfw)
