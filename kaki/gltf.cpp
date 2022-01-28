//
// Created by felix on 16/01/2022.
//

#include "gltf.h"
#include "cgltf.h"
#include <cstdio>
#include <span>
#include "vk.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <span>

namespace cereal {

    template<class Archive>
    void serialize(Archive &archive, glm::vec2 &vec) {
        archive(vec.x, vec.y);
    }

    template<class Archive>
    void serialize(Archive &archive, glm::vec3 &vec) {
        archive(vec.x, vec.y, vec.z);
    }

}

namespace kaki {


    template<class T>
    VkBuffer loadBuffer(const VkGlobals& vk, const std::span<T>& v) {

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = v.size_bytes();
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // TODO use a staging buffer

        VkBuffer buffer;
        VmaAllocation alloc;

        vmaCreateBuffer(vk.allocator, &bufferInfo, &vmaAllocInfo,
                        &buffer,
                        &alloc,
                        nullptr);

        void* bufferData;
        vmaMapMemory(vk.allocator, alloc, &bufferData);
        memcpy(bufferData, v.data(), v.size_bytes());
        vmaUnmapMemory(vk.allocator, alloc);

        return buffer;
    }


    void loadGltf(flecs::entity entity, const char *file) {

        std::ifstream is(file);
        cereal::BinaryInputArchive archive(is);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoord;
        std::vector<uint32_t> indices;

        std::vector<Mesh> meshes;

        archive(positions, normals, texcoord, indices, meshes);

        entity.scope([&]{
            for(auto& m : meshes) {
                entity.world().entity(m.name.c_str()).set<Mesh>(m);
            }
        });

        const VkGlobals& vk = *entity.world().get<VkGlobals>();

        Gltf gltf;

        gltf.positionBuffer = loadBuffer(vk, std::span(positions));
        gltf.normalBuffer = loadBuffer(vk, std::span(normals));
        gltf.uvBuffer = loadBuffer(vk, std::span(texcoord));
        gltf.indexBuffer = loadBuffer(vk, std::span(indices));

        entity.set<Gltf>(gltf);
    }

    void handleGltfLoads(flecs::iter iter, kaki::Asset *assets) {
        for(auto i : iter) {
            loadGltf(iter.entity(i), assets[i].path);
        }

    }
}