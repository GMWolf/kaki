//
// Created by felix on 12/01/2022.
//

#pragma once

#include <flecs.h>
#include <span>
#include <string>
#include <vector>
#include "package.h"
#include <membuf.h>
#include <cereal/archives/binary.hpp>

namespace kaki {

    struct AssetData {
        std::span<uint8_t> data;
        std::span<flecs::entity_t> entityRefs;
    };

    typedef void (*asset_handle_fn)(flecs::iter, AssetData*, void*);
    struct DependsOn {};
    struct ComponentAssetHandler {
        asset_handle_fn load {};
    };

    template<class T>
    ComponentAssetHandler serializeComponentAssetHandler() {
        return ComponentAssetHandler {
            .load = [](flecs::iter iter, AssetData* data, void* pc) {
                auto* c = static_cast<T*>(pc);
                for(auto i : iter) {
                    membuf buf(data[i].data);
                    std::istream bufStream(&buf);
                    cereal::BinaryInputArchive archive(bufStream);
                    archive(c[i]);
                }
            },
        };
    };

    flecs::entity loadPackage(flecs::world& world, const char* path);

    flecs::entity instanciatePackage(flecs::world& world, const Package& package, std::span<uint8_t> data);
}