//
// Created by felix on 05/03/2022.
//

#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <optional>
#include <glm/glm.hpp>

namespace kaki {

    struct GeometryBuffers {

        VkBuffer indexBuffer;

        union {
            VkBuffer vertexBuffers[4];
            struct {
                VkBuffer positionBuffer;
                VkBuffer normalBuffer;
                VkBuffer tangentBuffer;
                VkBuffer texcoordBuffer;
            };
        };

        VmaAllocation indexAlloc;
        VmaAllocation positionAlloc;
        VmaAllocation normalAlloc;
        VmaAllocation tangentAlloc;
        VmaAllocation texcoordAlloc;

        uint32_t* indices;
        glm::vec3* positions;
        glm::vec3* normals;
        glm::vec4* tangents;
        glm::vec2* texcoords;

        size_t vertexCount;
        size_t indexCount;

        size_t vertexHead;
        size_t indexHead;
    };

    struct MeshAlloc {
        size_t indexOffset;
        size_t vertexOffset;
    };

    void allocGeometryBuffer(VmaAllocator allocator, GeometryBuffers& geometryBuffers, size_t vertexCount, size_t indexCount);

    std::optional<MeshAlloc> allocMesh(GeometryBuffers& geom, size_t vertexCount, size_t indexCount);
}