//
// Created by felix on 12/01/2022.
//

#include "asset.h"
#include <rapidjson/document.h>
#include <fstream>
#include "shader.h"

static void processEntitiesForAsset(flecs::entity assetType, flecs::entity pack) {
    auto world = assetType.world();



    auto assetHandlers = world.filter_builder<kaki::AssetHandler>()
            .term(assetType)
            .build();

    assetHandlers.each([&](kaki::AssetHandler& handler) {
        //assetFilter.iter([&](flecs::iter iter, kaki::Asset* assets) {
        //    handler.load(iter, assets);
        //});
    });
}

void kaki::loadAssets(flecs::world& world, const char *path) {
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


    auto handlerFilter = world.query_builder<>()
            .term<kaki::AssetHandler>(flecs::Wildcard)
            .term<kaki::AssetHandler>(flecs::Wildcard).super(world.component<kaki::DependsOn>(),flecs::Cascade).oper(flecs::Optional)
            .build();

    handlerFilter.iter([pack](flecs::iter iter) {
       auto id = iter.term_id(1);
       auto assetType = id.object();
       auto assetName = assetType.name();
       auto assetFilter = iter.world().filter_builder<kaki::Asset>()
                .term(assetType)
                .term(flecs::ChildOf, pack)
                .build();

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

}
