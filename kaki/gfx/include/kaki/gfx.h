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
        uint32_t primitiveIndex;
        flecs::entity_t material;
    };

    struct Material {
        flecs::entity_t albedo;
        flecs::entity_t normal;
        flecs::entity_t metallicRoughness;
        flecs::entity_t ao;
        flecs::entity_t emissive;
    };

    namespace internal {
        struct MeshInstance {
            uint32_t indexOffset;
            uint32_t vertexOffset;
            uint32_t indexCount;
        };
    }

    struct gfx {
        explicit gfx(flecs::world& world);
    };

}

#endif //KAKI_GFX_H
