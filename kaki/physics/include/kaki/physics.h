//
// Created by felix on 27/03/2022.
//

#pragma once

#include <flecs.h>
#include <glm/vec3.hpp>

namespace kaki {

    struct RigidBody {
        float mass;
        bool sphere;
        glm::vec3 extent;
    };

    struct Physics {
        explicit Physics(flecs::world& world);
    };

    struct Impulse {
        flecs::entity target;
        glm::vec3 impulse;
    };

    void initPhysics(flecs::world& world);

}