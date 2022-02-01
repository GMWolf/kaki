add_library(kaki-core
        src/transform.cpp
        )

target_sources(kaki-core PUBLIC
        include/kaki/transform.h
        )

target_link_libraries(kaki-core PUBLIC glm)
target_include_directories(kaki-core
        PUBLIC include
        PRIVATE include/kaki)