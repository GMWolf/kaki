//
// Created by felix on 16/01/2022.
//

#include "gltf.h"
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
    
    template<class T, class Archive>
    VkBuffer loadBuffer(const VkGlobals& vk, Archive& archive) {

        size_t numElements = 0;
        archive( cereal::make_size_tag( numElements ) );

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = numElements * sizeof(T);
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
        archive( cereal::binary_data( bufferData, numElements * sizeof(T) ) );
        vmaUnmapMemory(vk.allocator, alloc);

        return buffer;
    }


    void loadGltf(flecs::entity entity, const char *file) {

        const VkGlobals& vk = *entity.world().get<VkGlobals>();

        std::ifstream is(file);
        cereal::BinaryInputArchive archive(is);

        Gltf gltf {};
        gltf.positionBuffer = loadBuffer<glm::vec3>(vk, archive);
        gltf.normalBuffer = loadBuffer<glm::vec3>(vk, archive);
        gltf.uvBuffer = loadBuffer<glm::vec2>(vk, archive);
        gltf.indexBuffer = loadBuffer<uint32_t>(vk, archive);

        std::vector<Mesh> meshes;

        archive(meshes);

        entity.scope([&]{
            for(auto& m : meshes) {
                entity.world().entity(m.name.c_str()).set<Mesh>(m);
            }
        });

        entity.set<Gltf>(gltf);
    }

    void handleGltfLoads(flecs::iter iter, kaki::Asset *assets) {
        for(auto i : iter) {
            loadGltf(iter.entity(i), assets[i].path);
        }

    }
}