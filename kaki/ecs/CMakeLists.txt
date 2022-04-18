add_library(kaki-ecs
        include/kaki/ecs.h
        include/kaki/ecs/component.h
        include/kaki/ecs/registry.h
        src/registry.cpp
        include/kaki/ecs/type.h
        include/kaki/ecs/query.h
        src/query.cpp include/kaki/ecs/view.h include/kaki/ecs/identifier.h)

target_include_directories(kaki-ecs
        PUBLIC include
        PRIVATE include/kaki include/kaki/ecs)