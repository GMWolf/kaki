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

namespace kaki {

    struct Gltf {
        union {
            VkBuffer buffers[3];
            struct {
                VkBuffer positionBuffer;
                VkBuffer normalBuffer;
                VkBuffer uvBuffer;
            };
        };

        VkBuffer indexBuffer;
        VmaAllocation positionBufferAllocation;
        VmaAllocation normalBufferAllocation;
        VmaAllocation uvBufferAllocation;
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

    void handleGltfLoads(flecs::iter iter, kaki::Asset* assets);

    void* loadGltfs(flecs::world& world, size_t count, std::span<uint8_t> data);
}