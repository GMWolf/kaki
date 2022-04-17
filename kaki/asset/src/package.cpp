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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define alloca _malloca
#endif

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

    if (type.override) {
        r |= ECS_OVERRIDE;
    }

    if (type.object) {
        auto o = resolveEntity(world, entities, type.object.value());
        return world.pair(r, o);
    } else {
        return r;
    }
}


static void callAssetHandlers(flecs::entity root, kaki::JobCtx& ctx) {

    std::atomic<uint64_t> jobCounter = 0;

    auto world = root.world();

    // Create a query that iterates asset handles in dependency order
    auto handlerQuery = world.query_builder<>()
            .term<kaki::ComponentAssetHandler>(flecs::Wildcard)
            .term<kaki::ComponentAssetHandler>(flecs::Wildcard).super(world.component<kaki::DependsOn>(), flecs::Cascade).oper(flecs::Optional)
            .build();

    // Iterate over each handler
    handlerQuery.iter([&](flecs::iter iter) {

        // Create a filter over assets in the pack of that handles asset type
        auto assetType = iter.pair(1).object();
        auto name = assetType.name();
        auto assetFilter = iter.world().filter_builder()
                .term(assetType)
                .term<kaki::AssetData>(assetType)
                .term(flecs::ChildOf, root).set(flecs::Self | flecs::SuperSet, flecs::ChildOf)
                .term(flecs::Prefab).oper(flecs::Optional)
                .build();

        // Iterate the handlers
        auto handlers = iter.term<kaki::ComponentAssetHandler>(1);

        for(auto i : iter) {
            auto& handler = handlers[i];

            assetFilter.iter([&jobCounter, &ctx, &world, handler](flecs::iter iter) {
                jobCounter++;
                kaki::AssetData* data = &iter.term<kaki::AssetData>(2)[0];
                void* assets = iter.term(1)[0];
                ctx.schedule([&jobCounter, &world, fn = handler.load, count = iter.count(), data, assets](kaki::JobCtx ctx){
                    fn(ctx, world, count, data, assets);
                    jobCounter--;
                });
                //handler.load(ctx, iter, data, assets);
            });
        }
    });

    ctx.wait(jobCounter, 0);
}

static flecs::entity instanciatePackage(flecs::world &world, const kaki::Package &package, std::span<uint8_t> dataFile, kaki::JobCtx& ctx) {

    std::vector<ecs_entity_t> entities;
    std::vector<flecs::Identifier> entityNames;

    auto nameIdentifier = world.pair<flecs::Identifier>(flecs::Name);

    // Instanciate the entities
    {
        auto entityCount = package.entities.size();
        ecs_bulk_desc_t entitiesBulkDesc{};
        entitiesBulkDesc.count = static_cast<int32_t >(entityCount);
        auto entitiesTmp = ecs_bulk_init(world.c_ptr(), &entitiesBulkDesc);
        entities.insert(entities.end(), entitiesTmp, entitiesTmp + entityCount);
    }

    entityNames.resize(package.entities.size());
    for(size_t i = 0; i < package.entities.size(); i++) {
        if (package.entities[i].name.length() > 0) {
            entityNames[i].value = ecs_os_strdup(package.entities[i].name.c_str());
        } else {
            char name[64];
            sprintf(name, "<%zu>", i);
            entityNames[i].value = ecs_os_strdup(name);
        }
    }

    // Move entities to tables, including AssetData components
    for(auto& table : package.tables) {
        ecs_bulk_desc_t bulkDesc{};
        bulkDesc.count = static_cast<int32_t>(table.entityCount);
        bulkDesc.entities = entities.data() + table.entityFirst;

        uint componentId = 0;

        void* data[ECS_ID_CACHE_SIZE]{};
        bulkDesc.data = data;

        for (auto& component : table.components) {

            assert(table.entityFirst + table.entityCount <= entities.size());
            auto t = resolveComponentType(world, entities, component.type);
            auto e = flecs::entity(world, t);

            bulkDesc.ids[componentId] = e;
            componentId++;

            if (!component.data.empty()) {
                assert(component.data.size() == table.entityCount);
                bulkDesc.ids[componentId] = world.pair(world.component<kaki::AssetData>(), e);
                auto* assetData = static_cast<kaki::AssetData*>(malloc(sizeof(kaki::AssetData) * table.entityCount));
                for(int i = 0; i < table.entityCount; i++) {
                    auto& cdata = component.data[i];
                    assetData[i].data = dataFile.subspan(cdata.offset, cdata.size);
                    assetData[i].entityRefs = std::span(entities).subspan(table.referenceOffset);
                }
                bulkDesc.data[componentId] = assetData;

                componentId++;
            }
        }

        {
            bulkDesc.ids[componentId] = nameIdentifier;
            bulkDesc.data[componentId] = entityNames.data() + table.entityFirst;
            componentId++;
        }

        ecs_bulk_init( world.c_ptr(), &bulkDesc );

        for(auto i = 0; i < ECS_ID_CACHE_SIZE; i++) {
            if (bulkDesc.data[i] && bulkDesc.ids[i] != nameIdentifier) {
                free(bulkDesc.data[i]);
            }

        }
    }

    // free entity names
    for(auto& i : entityNames) {
        ecs_os_free(i.value);
    }

    // Call asset handles
    if (!entities.empty()) {
        callAssetHandlers(flecs::entity(world, entities[0]), ctx);
    }

    // Remove AssetData components
    world.remove_all<kaki::AssetData>();


    return entities.empty() ? flecs::entity{} : flecs::entity(world, entities[0]);
}

void kaki::loadPackage(flecs::world &world, const char *path, JobCtx ctx) {
    std::ifstream input(path);
    cereal::JSONInputArchive archive(input);

    Package package;
    archive(package);

    auto data = mapFile(package.dataFile.c_str());

    auto pe = instanciatePackage(world, package, data, ctx);
    //unmapFile(data);
}
