add_library(kaki-physics src/physics.cpp include/kaki/physics.h)
target_link_libraries(kaki-physics PUBLIC flecs_static BulletDynamics BulletCollision Bullet3Common LinearMath glm kaki-core)
target_include_directories(kaki-physics
        PUBLIC include
        PRIVATE include/kaki
        )
target_include_directories(kaki-physics
        PRIVATE ${BULLET_PHYSICS_SOURCE_DIR}/src
        )