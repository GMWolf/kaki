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
#include <ranges>
#include <span>
#include <algorithm>
#include <iterator>
#include <filesystem>

namespace fs = std::filesystem;

struct Buffers {
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec4> tangents;
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
    m.name = mesh->name ? mesh->name : "";

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
            } else if (attribute.type == cgltf_attribute_type_tangent) {
                size_t offset = buffers.tangents.size();
                buffers.tangents.resize(offset + attribute.data->count);
                cgltf_accessor_unpack_floats(attribute.data, &buffers.tangents[offset].x, 4 * attribute.data->count);
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


void writeGltfEntity(kaki::Package& package, const std::string& name, Buffers& buffers, std::ostream& outData) {

    auto dataOffset = outData.tellp();
    {
        cereal::BinaryOutputArchive archive(outData);

        saveBuffer(buffers.position, archive);
        saveBuffer(buffers.normal, archive);
        saveBuffer(buffers.tangents, archive);
        saveBuffer(buffers.texcoord, archive);
        saveBuffer(buffers.indices, archive);

    }

    auto first = package.entities.size();
    package.entities.push_back({name});
    package.tables.push_back(kaki::Package::Table{
        .entityFirst = first,
        .entityCount = 1,
        .components = {kaki::Package::Component{
          .type = {"kaki::gfx::Gltf"},
          .data = {{static_cast<uint64_t>(dataOffset), static_cast<uint64_t>(outData.tellp() - dataOffset)}},
        }},
    });
}

void writeMeshes(kaki::Package& package, std::span<Mesh> meshes, std::ostream& outData) {

    auto first = package.entities.size();
    auto dataOffset = outData.tellp();

    auto& table = package.tables.emplace_back(kaki::Package::Table{
            .entityFirst = first,
            .entityCount = meshes.size(),
            .components = {
                    kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, 0u}},
                    kaki::Package::Component{.type = {"kaki::gfx::Mesh", {}}}
            }
    });

    {
        cereal::BinaryOutputArchive dataArchive(outData);
        for (auto& mesh: meshes) {
            package.entities.push_back({mesh.name});

            auto& data = table.components[1].data.emplace_back();
            data.offset = static_cast<uint64_t>(outData.tellp());
            dataArchive(mesh);
            data.size = static_cast<uint64_t>(outData.tellp()) - data.offset;
        }
    }
}

void writeImages(kaki::Package& package, std::span<cgltf_image> images, const char* inputPath, std::ostream& outData) {

    if (!images.empty()) {
        auto first = package.entities.size();
        auto dataStart = outData.tellp();

        auto& table = package.tables.emplace_back(kaki::Package::Table{
                .entityFirst = first,
                .entityCount = images.size(),
                .components = {
                        kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, 0u}},
                        kaki::Package::Component{.type = {"kaki::gfx::Image", {}}}
                }
        });

        {
            for (auto &t: images) {
                package.entities.push_back({t.name ? t.name : ""});
                char path[1024];
                cgltf_combine_paths(path, inputPath, t.uri);
                printf("%s: %s\n", t.name, path);

                std::ifstream input(path, std::ios::binary);

                auto& data = table.components[1].data.emplace_back();
                data.offset = static_cast<uint64_t>(outData.tellp());

                const size_t len = 4096;
                char buf[len];
                while(!input.eof()) {
                    input.read(buf, len);
                    int nBytesRead = input.gcount();
                    if(nBytesRead <= 0)
                        break;
                    outData.write(buf, nBytesRead);
                }
                input.close();
                data.size = static_cast<uint64_t>(outData.tellp()) - data.offset;
            }
        }
    }
}

kaki::Package::Data writeTransform(const cgltf_node* node, std::ostream& outData) {

    cereal::BinaryOutputArchive dataArchive(outData);

    kaki::Package::Data data{};
    data.offset = outData.tellp();
    dataArchive(node->translation[0], node->translation[1], node->translation[2]);
    dataArchive(node->scale[0]);
    dataArchive(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
    data.size = (uint64_t)outData.tellp() - data.offset;

    return data;
}

kaki::Package::Data writeTransform(std::ostream& outData) {

    cereal::BinaryOutputArchive dataArchive(outData);

    kaki::Package::Data data{};
    data.offset = outData.tellp();

    float zero = 0;
    float one = 1;

    dataArchive(zero, zero, zero);
    dataArchive(one);
    dataArchive(zero, zero, zero, one);
    data.size = (uint64_t)outData.tellp() - data.offset;

    return data;
}


kaki::Package::Data writeMeshFilter(cgltf_mesh* mesh, cgltf_primitive& primitive, uint64_t firstMesh, uint64_t firstImage, cgltf_data* cgltfData, std::ostream& outData)
{
    cereal::BinaryOutputArchive dataArchive(outData);
    kaki::Package::Data md{};

    md.offset = outData.tellp();
    uint64_t meshEntity = firstMesh + std::distance(cgltfData->meshes, mesh);
    auto albedoImage = primitive.material->pbr_metallic_roughness.base_color_texture.texture->image;
    auto normalImage = primitive.material->normal_texture.texture ? primitive.material->normal_texture.texture->image : nullptr;
    auto mrImage = primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture ? primitive.material->pbr_metallic_roughness.metallic_roughness_texture.texture->image : nullptr;
    auto aoImage = primitive.material->occlusion_texture.texture ? primitive.material->occlusion_texture.texture->image : nullptr;

    uint32_t primitiveIndex = std::distance(mesh->primitives, &primitive);
    uint64_t albedoEntity = firstImage + std::distance(cgltfData->images, albedoImage);
    uint64_t normalEntity = normalImage ? firstImage + std::distance(cgltfData->images, normalImage) : 0;
    uint64_t mrEntity = mrImage ? firstImage + std::distance(cgltfData->images, mrImage) : 0;
    uint64_t aoEntity = aoImage ? firstImage + std::distance(cgltfData->images, aoImage) : 0;

    dataArchive(meshEntity, primitiveIndex, albedoEntity, normalEntity, mrEntity, aoEntity);
    md.size = (uint64_t)outData.tellp() - md.offset;

    return md;
}

template<std::ranges::range Range>
void writeSinglePrimitiveMeshNodes(kaki::Package& package, uint64_t parentEntity, uint64_t firstMesh, uint64_t firstImage, cgltf_data* cgltfData, Range&& nodes, std::ostream& outData) {
    auto nodeCount = std::ranges::distance(nodes);

    if (nodeCount == 0) {
        return;
    }

    cereal::BinaryOutputArchive dataArchive(outData);
    kaki::Package::Table table {
            .entityFirst = package.entities.size(),
            .entityCount = nodeCount,
            .referenceOffset = 0,
            .components = {
                    kaki::Package::Component{.type = {"flecs::core::Prefab"}},
                    kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, parentEntity}},
                    kaki::Package::Component{.type = {"kaki::core::Transform", {}}},
                    kaki::Package::Component{.type = {"kaki::core::Transform", {}, true}},
                    kaki::Package::Component{.type = {"kaki::gfx::MeshFilter", {}}},
            },
    };

    auto& transformData = table.components[2].data;
    auto& meshData = table.components[4].data;

    for(cgltf_node* node : nodes) {
        package.entities.push_back({node->name});

        transformData.emplace_back(writeTransform(node, outData));
        meshData.emplace_back(writeMeshFilter(node->mesh, node->mesh->primitives[0], firstMesh, firstImage, cgltfData, outData));
    }

    package.tables.emplace_back(std::move(table));
}

void writeMultiPrimitiveMeshNodesPrimitives(kaki::Package& package, uint64_t parentEntity, uint64_t firstMesh, uint64_t firstImage, cgltf_data* cgltfData, cgltf_mesh* mesh, std::ostream& outData) {

    cereal::BinaryOutputArchive dataArchive(outData);
    kaki::Package::Table table {
            .entityFirst = package.entities.size(),
            .entityCount = mesh->primitives_count,
            .referenceOffset = 0,
            .components = {
                    kaki::Package::Component{.type = {"flecs::core::Prefab"}},
                    kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, parentEntity}},
                    kaki::Package::Component{.type = {"kaki::gfx::MeshFilter", {}}},
            },
    };

    auto& meshData = table.components[2].data;

    for(cgltf_primitive& primitive : std::span(mesh->primitives, mesh->primitives_count)) {
        package.entities.push_back({""});

        meshData.emplace_back(writeMeshFilter(mesh, primitive, firstMesh, firstImage, cgltfData, outData));
    }

    package.tables.emplace_back(std::move(table));
}

template<std::ranges::range Range>
void writeMultiPrimitiveMeshNodes(kaki::Package& package, uint64_t parentEntity, uint64_t firstMesh, uint64_t firstImage, cgltf_data* cgltfData, Range&& nodes, std::ostream& outData) {
    auto nodeCount = std::ranges::distance(nodes);
    if (nodeCount == 0) {
        return;
    }

    cereal::BinaryOutputArchive dataArchive(outData);

    kaki::Package::Table table {
            .entityFirst = package.entities.size(),
            .entityCount = nodeCount,
            .referenceOffset = 0,
            .components = {
                    kaki::Package::Component{.type = {"flecs::core::Prefab"}},
                    kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, parentEntity}},
                    kaki::Package::Component{.type = {"kaki::core::Transform", {}}},
                    kaki::Package::Component{.type = {"kaki::core::Transform", {}, true}},
            },
    };


    auto& transformData = table.components[2].data;

    for(cgltf_node* node : nodes) {
        package.entities.push_back({node->name ? node->name : ""});
        transformData.emplace_back(writeTransform(node, outData));
    }

    uint64_t it = table.entityFirst;

    package.tables.emplace_back(std::move(table));

    for(cgltf_node* node : nodes) {
        writeMultiPrimitiveMeshNodesPrimitives(package, it, firstMesh, firstImage, cgltfData, node->mesh, outData);
        it ++;
    }

}


template<std::ranges::range Range>
void writeMeshNodes(kaki::Package& package, uint64_t parentEntity, uint64_t firstMesh, uint64_t firstImage, cgltf_data* cgltfData, Range&& nodes, std::ostream& outData) {

    auto nodeCount = std::ranges::distance(nodes);

    if (nodeCount == 0) {
        return;
    }

    writeSinglePrimitiveMeshNodes(package, parentEntity, firstMesh, firstImage, cgltfData, nodes | std::views::filter([](cgltf_node* node){return node->mesh->primitives_count == 1;}), outData);
    writeMultiPrimitiveMeshNodes(package, parentEntity, firstMesh, firstImage, cgltfData, nodes | std::views::filter([](cgltf_node* node){return node->mesh->primitives_count > 1;}), outData);
}

template<std::ranges::range Range>
void writeCameraNodes(kaki::Package& package, uint64_t parentEntity, Range&& nodes, std::ostream& outData) {

    auto nodeCount = std::ranges::distance(nodes);

    if (nodeCount == 0) {
        return;
    }

    cereal::BinaryOutputArchive dataArchive(outData);

    kaki::Package::Table table {
            .entityFirst = package.entities.size(),
            .entityCount = nodeCount,
            .referenceOffset = 0,
            .components = {
                    kaki::Package::Component{.type = {"flecs::core::Prefab"}},
                    kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, parentEntity}},
                    kaki::Package::Component{.type = {"kaki::core::Transform", {}}},
                    kaki::Package::Component{.type = {"kaki::core::Transform", {}, true}},
                    kaki::Package::Component{.type = {"kaki::gfx::Camera", {}}},
            },
    };

    auto& transformData = table.components[2].data;
    auto& cameraData = table.components[4].data;

    for(cgltf_node* node : nodes) {
        package.entities.push_back({node->name});
        transformData.emplace_back(writeTransform(node, outData));

        kaki::Package::Data& cd = cameraData.emplace_back();
        cd.offset = outData.tellp();
        dataArchive(node->camera->data.perspective.yfov);
        cd.size = (uint64_t)outData.tellp() - cd.offset;
    }

    package.tables.emplace_back(std::move(table));

}


static bool hasMesh(cgltf_node* node) {
    return node->mesh;
}

static bool hasCamera(cgltf_node* node) {
    return node->camera;
}

void writeNodes(kaki::Package& package, uint64_t parent, uint64_t firstMesh, uint64_t firstImage, cgltf_data* cgltfData, std::span<cgltf_node*> nodes, std::ostream& outData) {

    writeMeshNodes(package, parent, firstMesh, firstImage, cgltfData, nodes | std::views::filter(hasMesh), outData);
    writeCameraNodes(package, parent, nodes | std::views::filter(hasCamera), outData);

}


void writeScene(kaki::Package& package, uint64_t firstMesh, uint64_t firstImage, cgltf_data* data, cgltf_scene* scene, std::ostream& outData) {

    kaki::Package::Table table {
            .entityFirst = package.entities.size(),
            .entityCount = 1,
            .referenceOffset = 0,
            .components = {
                    kaki::Package::Component{.type = {kaki::Package::TypeId::ChildOf, 0u}},
                    kaki::Package::Component{.type = {"flecs::core::Prefab"}},
            },
    };

    uint64_t id = table.entityFirst;

    package.entities.push_back({scene->name ? scene->name : ""});

    package.tables.push_back(std::move(table));

    writeNodes(package, id, firstMesh, firstImage, data, std::span(scene->nodes, scene->nodes_count), outData);
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Expected 2 arguments\n");
        return 1;
    }

    const fs::path inputPath = argv[1];
    const std::string outputPath = argv[2];
    const std::string binOutputPath = outputPath + ".bin";

    auto assetName = inputPath.stem();

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
    package.dataFile = binOutputPath.substr(binOutputPath.find_last_of("/\\") + 1);

    std::ofstream outData(binOutputPath, std::ios::binary | std::ios::out);

    writeGltfEntity(package, assetName, buffers, outData);
    auto meshRefStart = package.entities.size();
    writeMeshes(package, meshes, outData);
    auto imageRefStart = package.entities.size();
    writeImages(package, std::span(data->images, data->images_count), inputPath.c_str(), outData);
    writeScene(package, meshRefStart, imageRefStart, data, data->scene, outData);

    std::ofstream os(outputPath);
    cereal::JSONOutputArchive archive( os );
    archive(package);

    return 0;
}