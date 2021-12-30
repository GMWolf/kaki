add_library(kaki-gfx
        gfx.h
        gfx.cpp
        vk.h
        window.h
        window.cpp)

target_link_libraries(kaki-gfx PUBLIC flecs)
target_link_libraries(kaki-gfx PRIVATE vulkan vk-bootstrap glfw)

