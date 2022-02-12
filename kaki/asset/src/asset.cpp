//
// Created by felix on 12/01/2022.
//

#include "asset.h"
#include <rapidjson/document.h>
#include <fstream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <span>
#include <overloaded.h>

flecs::entity kaki::loadAssets(flecs::world& world, const char *path) {

    world.component<DependsOn>().add(flecs::Acyclic);

    rapidjson::Document doc;
    std::ifstream t(path);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    doc.ParseInsitu(str.data());

    auto packName = doc["name"].GetString();
    auto assetList = doc["assets"].GetArray();

    flecs::entity pack = world.entity(packName);

    pack.scope([&](){
        for(auto& assetEntry : assetList) {
            auto assetName = assetEntry["name"].GetString();
            auto assetPath = assetEntry["path"].GetString();
            auto assetTypeName = assetEntry["type"].GetString();
            auto assetType = world.lookup(assetTypeName);
            auto e = world.entity(assetName).set<kaki::Asset>({assetPath}).add(assetType);
            assert(e.get_object(flecs::ChildOf) == pack);
        }
    });

    // Create a query that iterates asset handles in dependency order
    auto handlerQuery = world.query_builder<>()
            .term<kaki::AssetHandler>(flecs::Wildcard)
            .term<kaki::AssetHandler>(flecs::Wildcard).super(world.component<kaki::DependsOn>(),flecs::Cascade).oper(flecs::Optional)
            .build();

    // Iterate over each handler
    handlerQuery.iter([pack](flecs::iter iter) {

        // Create a filter over assets in the pack of that handles asset type
        auto assetType = iter.pair(1).object();
        auto assetFilter = iter.world().filter_builder<kaki::Asset>()
                .term(assetType)
                .term(flecs::ChildOf, pack)
                .build();

       // Iterate the handlers
       auto handlers = iter.term<AssetHandler>(1);

       for(auto i : iter) {
           auto& handler = handlers[i];

           iter.world().defer_begin();
           assetFilter.iter([handler](flecs::iter iter, kaki::Asset* assets){
               handler.load(iter, assets);
           });
           iter.world().defer_end();
       }

    });

    return pack;
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

flecs::entity kaki::instanciatePackage(flecs::world &world, const kaki::Package &package) {

    std::vector<ecs_entity_t> entities;
    {
        auto entityCount = package.entities.size();
        ecs_bulk_desc_t entitiesBulkDesc{};
        entitiesBulkDesc.count = static_cast<int32_t >(entityCount);
        auto entitiesTmp = ecs_bulk_init(world.c_ptr(), &entitiesBulkDesc);
        entities.insert(entities.end(), entitiesTmp, entitiesTmp + entityCount);

        for(int i = 0; i < entityCount; i++) {
            flecs::entity(world, entities[i]).set_name(package.entities[i].name.c_str());
        }
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

            loaders[i] = e.get<ComponentLoader>();
            if (loaders[i]) {
                bulkDesc.data[i] = loaders[i]->deserialize(table.entityCount, table.typeData[i]);
            }
        }

        ecs_bulk_init( world.c_ptr(), &bulkDesc );

        for(int i = 0; i < table.types.size(); i++) {
            if (loaders[i]) {
                loaders[i]->free(table.entityCount, bulkDesc.data[i]);
            }
        }
    }

    return entities.empty() ? flecs::entity{} : flecs::entity(world, entities[0]);
}