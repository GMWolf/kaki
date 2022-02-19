add_library(kaki-core
        src/transform.cpp
        include/kaki/core.h
        src/core.cpp)

target_sources(kaki-core PUBLIC
        include/kaki/transform.h
        )

target_link_libraries(kaki-core PUBLIC glm flecs_static)
target_include_directories(kaki-core
        PUBLIC include
        PRIVATE include/kaki)