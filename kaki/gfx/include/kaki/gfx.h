//
// Created by felix on 29-12-21.
//

#ifndef KAKI_GFX_H
#define KAKI_GFX_H

#include <flecs.h>
#include <vector>

namespace kaki {

    struct Camera {
        float fov;
    };

    struct MeshFilter {
        flecs::entity_t mesh;
        flecs::entity_t albedo;
        flecs::entity_t normal;
        flecs::entity_t metallicRoughness;
        flecs::entity_t ao;
    };

    namespace internal {
        struct MeshInstance {
            struct Primitive {
                uint32_t indexOffset;
                uint32_t vertexOffset;
                uint32_t indexCount;
            };
            std::vector<Primitive> primitives;
        };
    }

    struct gfx {
        explicit gfx(flecs::world& world);
    };

}

#endif //KAKI_GFX_H
