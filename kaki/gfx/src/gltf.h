//
// Created by felix on 16/01/2022.
//

#pragma once
#include <flecs.h>
#include <vector>
#include <kaki/asset.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cereal/types/vector.hpp>
#include "geometry_buffer.h"

namespace kaki {

    struct Gltf {
        MeshAlloc meshAlloc;
    };

    struct Mesh {
        std::string name;

        struct Primitive {
            uint32_t indexOffset;
            uint32_t vertexOffset;
            uint32_t indexCount;
        };

        std::vector<Primitive> primitives;
    };

    template<class Archive>
    void serialize(Archive& archive, Mesh::Primitive& primitive) {
        archive(primitive.indexOffset, primitive.vertexOffset, primitive.indexCount);
    }

    template<class Archive>
    void serialize(Archive& archive, Mesh& mesh) {
        archive(mesh.name, mesh.primitives);
    }

    void loadGltfs(JobCtx ctx, flecs::world& world, size_t count, AssetData* data, void* p_gltf);
}