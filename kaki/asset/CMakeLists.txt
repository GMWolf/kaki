add_library(kaki-asset
        src/asset.cpp
        include/kaki/package.h)

target_sources(kaki-asset PUBLIC
        include/kaki/asset.h
        )

target_link_libraries(kaki-asset PUBLIC flecs_static kaki-common)
target_link_libraries(kaki-asset PRIVATE rapidjson cereal)

target_include_directories(kaki-asset
        PUBLIC include
        PRIVATE include/kaki)