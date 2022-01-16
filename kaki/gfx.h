//
// Created by felix on 29-12-21.
//

#ifndef KAKI_GFX_H
#define KAKI_GFX_H

#include <flecs.h>


namespace kaki {

    struct Camera {
        float fov;
    };

    struct MeshFilter {
        flecs::entity mesh;
        flecs::entity image;
    };

    struct gfx {
        explicit gfx(flecs::world& world);
    };

}

#endif //KAKI_GFX_H
