//
// Created by felix on 16/01/2022.
//

#pragma once
#include <flecs.h>
#include <vector>
#include "asset.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
namespace kaki {

    struct Gltf {
        std::vector<std::pair<VkBuffer, VmaAllocation>> buffers;
    };

    struct Mesh {
        struct Primitive {
            uint32_t indexOffset;
            uint32_t vertexOffset;
            uint32_t indexCount;
        };

        std::vector<Primitive> primitives;
    };


    void handleGltfLoads(flecs::iter iter, kaki::Asset* assets);

}