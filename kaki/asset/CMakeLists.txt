add_library(kaki-asset-interface INTERFACE
        include/kaki/asset.h
        include/kaki/package.h)
target_include_directories(kaki-asset-interface INTERFACE include)

add_library(kaki-asset src/asset.cpp src/package.cpp)

target_link_libraries(kaki-asset PUBLIC kaki-asset-interface flecs_static kaki-common)
target_link_libraries(kaki-asset PRIVATE rapidjson cereal)

target_include_directories(kaki-asset
        PRIVATE include/kaki)