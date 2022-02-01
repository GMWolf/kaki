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

flecs::entity kaki::loadAssets(flecs::world& world, const char *path) {
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
        auto assetType = iter.term_id(1).object();
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

namespace kaki {

    template<class Archive>
    void serialize(Archive &archive, Package::Entity &entity) {
        archive(entity.name);
    }

    template<class Archive>
    void serialize(Archive &archive, Package::ComponentEntry &entry) {
        archive(entry.entity, entry.dataOffset, entry.dataSize);
    }

    template<class Archive>
    void serialize(Archive &archive, Package::ComponentGroup &group) {
        archive(group.type, group.componentBegin, group.componentEnd);
    }

    template<class Archive>
    void serialize(Archive &archive, Package &package) {
        archive(package.name, package.data, package.entities, package.componentEntries, package.componentGroups);
    }

    flecs::entity loadPackage(flecs::world &world, const char *path) {

        std::ifstream is(path);
        cereal::BinaryInputArchive archive(is);

        Package package;
        archive(package);

        auto packageEntity = world.entity(package.name);

        std::vector<flecs::entity> entities;

        packageEntity.scope([&]{
            for(const Package::Entity& entity : package.entities) {
                entities.push_back(world.entity(entity.name));
            }
        });

        for(const Package::ComponentGroup& group : package.componentGroups) {
            flecs::entity componentType = world.lookup(group.type);
            auto filter = world.filter_builder().term<ComponentLoader>(componentType).build();
            component_deserialize_fn fn;
            filter.each([&](ComponentLoader& loader) {
                fn = loader.deserialize;
            });

            for(auto c = group.componentBegin; c != group.componentEnd; c++) {
                const Package::ComponentEntry& entry = package.componentEntries[c];
                fn(entities[entry.entity], std::span(package.data + entry.dataOffset, entry.dataSize));
            }
        }

    }
}
