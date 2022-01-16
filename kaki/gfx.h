//
// Created by felix on 29-12-21.
//

#ifndef KAKI_GFX_H
#define KAKI_GFX_H

#include <flecs.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace kaki {

    struct Camera {
        float x, y , width, height;
    };

    struct Rectangle {
        glm::vec2 pos;
        glm::vec3 color;
        flecs::entity image;
    };

    struct MeshFilter {
        flecs::entity mesh;
    };

    struct gfx {
        explicit gfx(flecs::world& world);
    };

}

#endif //KAKI_GFX_H
