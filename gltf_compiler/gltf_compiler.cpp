//
// Created by felix on 24/01/2022.
//

#include <cstdio>
#include <span>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <glm/glm.hpp>
#include <fstream>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <kaki/package.h>
#include <cereal_ext.h>

struct Buffers {
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec2> texcoord;
    std::vector<uint32_t> indices;
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

struct Image {
    std::string name;
    std::vector<uint8_t> ktxData;
};

template<class Archive>
void serialize(Archive& archive, Mesh::Primitive& primitive) {
    archive(primitive.indexOffset, primitive.vertexOffset, primitive.indexCount);
}

template<class Archive>
void serialize(Archive& archive, Mesh& mesh) {
    archive(mesh.name, mesh.primitives);
}

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


Mesh loadMeshData(cgltf_mesh* mesh, Buffers& buffers) {

    Mesh m;
    m.name = mesh->name;

    for(auto& primitive : std::span(mesh->primitives, mesh->primitives_count)) {

        m.primitives.push_back(Mesh::Primitive{
                .indexOffset = static_cast<uint32_t>(buffers.indices.size()),
                .vertexOffset = static_cast<uint32_t>(buffers.position.size()),
                .indexCount = static_cast<uint32_t>(primitive.indices->count),
        });

        buffers.indices.reserve(buffers.indices.size() + primitive.indices->count);
        for(uint64_t index = 0; index < primitive.indices->count; index++) {
            buffers.indices.push_back(cgltf_accessor_read_index(primitive.indices, index));
        }

        for(auto& attribute : std::span(primitive.attributes, primitive.attributes_count)) {
            if (attribute.type == cgltf_attribute_type_position) {
                size_t offset = buffers.position.size();
                buffers.position.resize(offset + attribute.data->count);
                cgltf_accessor_unpack_floats(attribute.data, &buffers.position[offset].x, 3 * attribute.data->count);
            } else if (attribute.type == cgltf_attribute_type_normal) {
                size_t offset = buffers.normal.size();
                buffers.normal.resize(offset + attribute.data->count);
                cgltf_accessor_unpack_floats(attribute.data, &buffers.normal[offset].x, 3 * attribute.data->count);
            } else if (attribute.type == cgltf_attribute_type_texcoord) {
                size_t offset = buffers.texcoord.size();
                buffers.texcoord.resize(offset + attribute.data->count);
                cgltf_accessor_unpack_floats(attribute.data, &buffers.texcoord[offset].x, 2 * attribute.data->count);
            }
        }
    }

    return m;
};

template<class T, class Archive>
void saveBuffer(const std::vector<T>& vec, Archive& archive)
{
    archive( cereal::make_size_tag( static_cast<size_t>( vec.size() ) ) );
    archive( cereal::binary_data( vec.data(), vec.size() * sizeof(T) ) );
}


void writeGltfEntity(kaki::Package& package, const std::string& name, Buffers& buffers) {
    std::string data;
    std::stringstream datastream(std::stringstream::binary | std::ios::in | std::ios::out);
    {
        cereal::BinaryOutputArchive archive(datastream);

        saveBuffer(buffers.position, archive);
        saveBuffer(buffers.normal, archive);
        saveBuffer(buffers.texcoord, archive);
        saveBuffer(buffers.indices, archive);

        data = datastream.str();
    }

    auto first = package.entities.size();
    package.entities.push_back({name});
    package.tables.push_back(kaki::Package::Table{
        .entityFirst = first,
        .entityCount = 1,
        .types = {
                {"kaki::Gltf", {}}
        },
        .typeData = {
                {{data.begin(), data.end()}},
        },

    });
}

void writeMeshes(kaki::Package& package, std::span<Mesh> meshes) {

    auto first = package.entities.size();

    std::string meshData;
    {
        std::stringstream meshDataStream(std::stringstream::binary | std::ios::in | std::ios::out);
        cereal::BinaryOutputArchive dataArchive(meshDataStream);
        for (auto& mesh: meshes) {
            package.entities.push_back({mesh.name});
            dataArchive(mesh);
        }
        meshData = meshDataStream.str();
    }

    auto childofType = kaki::Package::Type {
        .typeId = kaki::Package::TypeId::ChildOf,
        .object = 0u, //
    };

    package.tables.push_back(kaki::Package::Table{
        .entityFirst = first,
        .entityCount = meshes.size(),
        .types = {
                {kaki::Package::TypeId::ChildOf, 0u}, //Child of the gltf entity (entity 0 in package)
                {"kaki::gfx::Mesh", {}}
        },
        .typeData = {
                {},
                {{meshData.begin(), meshData.end()}},
        },
    });
}

void writeImages(kaki::Package& package, std::span<cgltf_image> images, const char* inputPath) {

    if (!images.empty()) {

        auto first = package.entities.size();

        std::string data;
        {
            std::stringstream meshDataStream(std::stringstream::binary | std::ios::in | std::ios::out);
            cereal::BinaryOutputArchive dataArchive(meshDataStream);
            for (auto &t: images) {
                package.entities.push_back({t.name});
                char path[1024];
                cgltf_combine_paths(path, inputPath, t.uri);
                printf("%s: %s\n", t.name, path);
                std::ifstream input(path, std::ios::binary);
                std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
                dataArchive(buffer);
            }
            data = meshDataStream.str();
        }

        kaki::Package::Table table{
                .entityFirst = first,
                .entityCount = images.size(),
                .types = {
                        {kaki::Package::TypeId::ChildOf, 0u}, //Child of the gltf entity (entity 0 in package)
                        {"kaki::gfx::Image",             {}}
                },
                .typeData = {
                        {},
                        {{data.begin(), data.end()}},
                },
        };

        package.tables.emplace_back(std::move(table));
    }
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Expected 2 arguments\n");
        return 1;
    }

    const std::string inputPath = argv[1];
    const char* outputPath = argv[2];

    auto assetName = inputPath.substr(inputPath.find_last_of("/\\") + 1);
    std::replace(assetName.begin(), assetName.end(), '.', '_');

    cgltf_options options = {};
    cgltf_data* data{};
    cgltf_result result = cgltf_parse_file(&options, inputPath.c_str(), &data);

    if (result != cgltf_result_success) {
        fprintf(stderr, "Failed to load gltf file: %s\n", inputPath.c_str());
        return 1;
    }

    result = cgltf_load_buffers(&options, data, inputPath.c_str());

    if (result != cgltf_result_success) {
        fprintf(stderr, "Failed to load gltf binary file for %s\n", inputPath.c_str());
        return 1;
    }

    Buffers buffers;
    std::vector<Mesh> meshes;
    for(auto& mesh : std::span(data->meshes, data->meshes_count)) {
        meshes.push_back(loadMeshData(&mesh, buffers));
    }

    for(auto& image : std::span(data->images, data->images_count)) {
        char path[1024];
        cgltf_combine_paths(path, inputPath.c_str(), image.uri);
    }

    kaki::Package package;

    writeGltfEntity(package, assetName, buffers);
    writeMeshes(package, meshes);
    writeImages(package, std::span(data->images, data->images_count), inputPath.c_str());


    std::ofstream os(outputPath);
    cereal::JSONOutputArchive archive( os );
    archive(package);

    return 0;
}