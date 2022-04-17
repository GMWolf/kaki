//
// Created by felix on 16/01/2022.
//

#include "gltf.h"
#include <cstdio>
#include <span>
#include "vk.h"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "membuf.h"
#include <fstream>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <span>
#include "geometry_buffer.h"

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
    void loadBuffer(T* ptr, size_t count, Archive& archive) {

        size_t numElements = 0;
        archive( cereal::make_size_tag( numElements ) );
        assert(numElements == count);
        archive( cereal::binary_data( ptr, count * sizeof(T) ) );
    }


    void loadGltfs(JobCtx ctx, flecs::world& world, size_t assetCount, AssetData* data, void* p_gltf) {

        VkGlobals& vk = *world.get_mut<VkGlobals>();

        GeometryBuffers& geomBuffer = vk.geometry;

        Gltf* gltfs = static_cast<Gltf*>(p_gltf);
        for(size_t i = 0; i < assetCount; i++) {
            membuf buf(data[i].data);
            std::istream bufStream(&buf);
            cereal::BinaryInputArchive archive(bufStream);

            size_t indexCount, vertexCount;
            archive(indexCount, vertexCount);

            auto meshAlloc = allocMesh(geomBuffer, vertexCount, indexCount);
            assert(meshAlloc.has_value());
            gltfs[i].meshAlloc = *meshAlloc;

            loadBuffer(geomBuffer.positions + meshAlloc->vertexOffset, vertexCount, archive);
            loadBuffer(geomBuffer.normals + meshAlloc->vertexOffset, vertexCount, archive);
            loadBuffer(geomBuffer.tangents + meshAlloc->vertexOffset, vertexCount, archive);
            loadBuffer(geomBuffer.texcoords + meshAlloc->vertexOffset, vertexCount, archive);
            loadBuffer(geomBuffer.indices + meshAlloc->indexOffset, indexCount, archive);
        }
    }
}