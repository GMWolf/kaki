//
// Created by felix on 12/02/2022.
//

#include "package.h"
#include "asset.h"
#include <cereal/cereal.hpp>
#include <fstream>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <span>
#include <overloaded.h>
#include <cereal_ext.h>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

static std::span<uint8_t> mapFile(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return {};
    }

    struct stat sb{};
    if (fstat(fd, &sb) == -1) {
        return {};
    }

    size_t length = sb.st_size;
    const void *ptr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0u);

    close(fd);

    if (ptr == MAP_FAILED) {
        return {};
    }
    return {(uint8_t *) ptr, length};
}

static void unmapFile(std::span<uint8_t> data) {
    munmap((void *) data.data(), data.size_bytes());
}

static flecs::entity_t resolveEntity(flecs::world& world, std::span<flecs::entity_t> entities,
                                     const kaki::Package::EntityRef& ref) {
    return std::visit(overloaded{
            [&world](const kaki::Package::TypeId& id) {
                switch (id) {
                    case kaki::Package::TypeId::ChildOf:
                        return flecs::ChildOf;
                        break;
                };
                return flecs::entity_t{};
            },
            [&entities](const uint64_t& index) {
                return entities[index];
            },
            [&world](const std::string& str) {
                return world.lookup(str.c_str()).id();
            },
    }, ref);
}

static flecs::id_t resolveComponentType(flecs::world& world, std::span<flecs::entity_t> entities,
                                        const kaki::Package::Type& type) {
    auto r = resolveEntity(world, entities, type.typeId);

    if (type.object) {
        auto o = resolveEntity(world, entities, type.object.value());
        return world.pair(r, o);
    } else {
        return r;
    }
}

flecs::entity kaki::instanciatePackage(flecs::world &world, const kaki::Package &package, std::span<uint8_t> dataFile) {

    std::vector<ecs_entity_t> entities;
    {
        auto entityCount = package.entities.size();
        ecs_bulk_desc_t entitiesBulkDesc{};
        entitiesBulkDesc.count = static_cast<int32_t >(entityCount);
        auto entitiesTmp = ecs_bulk_init(world.c_ptr(), &entitiesBulkDesc);
        entities.insert(entities.end(), entitiesTmp, entitiesTmp + entityCount);
    }

    for(auto table : package.tables) {
        ecs_bulk_desc_t bulkDesc{};
        bulkDesc.count = static_cast<int32_t>(table.entityCount);
        bulkDesc.entities = entities.data() + table.entityFirst;

        const ComponentLoader* loaders[ECS_MAX_ADD_REMOVE] {};
        void* data[ECS_MAX_ADD_REMOVE]{};
        bulkDesc.data = data;

        for (int i = 0; i < table.types.size(); i++) {
            assert(table.entityFirst + table.entityCount <= entities.size());
            auto t = resolveComponentType(world, entities, table.types[i]);
            auto e = flecs::entity(world, t);
            bulkDesc.ids[i] = e;

            if (e.is_pair()) {
                loaders[i] = e.relation().get<ComponentLoader>();
            } else {
                loaders[i] = e.get<ComponentLoader>();
            }

            if (loaders[i]) {
                std::span<uint8_t> cdata = {dataFile.subspan(table.typeData[i].offset, table.typeData[i].size)};
                bulkDesc.data[i] = loaders[i]->deserialize(world, table.entityCount, cdata);
            }
        }

        ecs_bulk_init( world.c_ptr(), &bulkDesc );

        for(int i = 0; i < table.types.size(); i++) {
            if (loaders[i]) {
                loaders[i]->free(world, table.entityCount, bulkDesc.data[i]);
            }
        }
    }

    for(int i = 0; i < package.entities.size(); i++) {
        flecs::entity(world, entities[i]).set_name(package.entities[i].name.c_str());
    }

    return entities.empty() ? flecs::entity{} : flecs::entity(world, entities[0]);
}

flecs::entity kaki::loadPackage(flecs::world &world, const char *path) {
    std::ifstream input(path);
    cereal::JSONInputArchive archive(input);

    Package package;
    archive(package);

    auto data = mapFile(package.dataFile.c_str());
    auto pe = instanciatePackage(world, package, data);
    unmapFile(data);
    return pe;
}
