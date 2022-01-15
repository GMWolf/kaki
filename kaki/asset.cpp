//
// Created by felix on 12/01/2022.
//

#include "asset.h"
#include <rapidjson/document.h>
#include <fstream>

void kaki::loadAssets(flecs::world& world, const char *path) {
    rapidjson::Document doc;
    std::ifstream t(path);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    doc.ParseInsitu(str.data());


    auto packName = doc["name"].GetString();
    auto assetList = doc["assets"].GetArray();

    for(auto& assetEntry : assetList) {
        auto assetName = assetEntry["name"].GetString();
        auto assetPath = assetEntry["path"].GetString();
        auto assetTypeName = assetEntry["type"].GetString();
        auto assetType = world.lookup(assetTypeName);

        world.entity(assetName).set<kaki::Asset>({assetPath}).add(assetType);
    }


}
