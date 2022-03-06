//
// Created by felix on 05/03/2022.
//

#include "geometry_buffer.h"
#include <glm/glm.hpp>

namespace kaki {

    static void* allocBuffer(VmaAllocator allocator, size_t size, VkBuffer& buffer, VmaAllocation& alloc) {

        VkBufferCreateInfo bufferInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };

        VmaAllocationCreateInfo vmaAllocInfo {
            .usage = VMA_MEMORY_USAGE_CPU_ONLY,
        };

        vmaCreateBuffer(allocator, &bufferInfo, &vmaAllocInfo, &buffer, &alloc, nullptr);

        void* map;
        vmaMapMemory(allocator, alloc, &map);

        return map;
    }

    void allocGeometryBuffer(VmaAllocator allocator, GeometryBuffers &geometryBuffers, size_t vertexCount, size_t indexCount) {
        geometryBuffers.indices = static_cast<uint32_t *>(allocBuffer(allocator, indexCount * sizeof(uint32_t),
                                                                      geometryBuffers.indexBuffer,
                                                                      geometryBuffers.indexAlloc));
        geometryBuffers.positions = static_cast<glm::vec3 *>(allocBuffer(allocator, vertexCount * sizeof(glm::vec3),
                                                                         geometryBuffers.positionBuffer,
                                                                         geometryBuffers.positionAlloc));
        geometryBuffers.normals = static_cast<glm::vec3 *>(allocBuffer(allocator, vertexCount * sizeof(glm::vec3),
                                                                       geometryBuffers.normalBuffer,
                                                                       geometryBuffers.normalAlloc));
        geometryBuffers.tangents = static_cast<glm::vec4 *>(allocBuffer(allocator, vertexCount * sizeof(glm::vec4),
                                                                        geometryBuffers.tangentBuffer,
                                                                        geometryBuffers.tangentAlloc));
        geometryBuffers.texcoords = static_cast<glm::vec2 *>(allocBuffer(allocator, vertexCount * sizeof(glm::vec2),
                                                                         geometryBuffers.texcoordBuffer,
                                                                         geometryBuffers.texcoordAlloc));

        geometryBuffers.vertexCount = vertexCount;
        geometryBuffers.indexCount = indexCount;
        geometryBuffers.vertexHead = 0;
        geometryBuffers.indexHead = 0;
    }

    std::optional<MeshAlloc> allocMesh(GeometryBuffers &geom, size_t vertexCount, size_t indexCount) {

        MeshAlloc alloc {
            .indexOffset = geom.indexHead,
            .vertexOffset = geom.vertexHead,
        };

        if (geom.vertexHead + vertexCount <= geom.vertexCount && geom.indexHead +indexCount <= geom.indexCount) {
            geom.vertexHead += vertexCount;
            geom.indexHead += indexCount;
            return alloc;
        } else {
            return {};
        }
    }

}