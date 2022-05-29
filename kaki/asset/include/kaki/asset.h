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
#include <kaki/job.h>
#include <kaki/ecs.h>

namespace kaki {

    struct AssetData {
        std::span<uint8_t> data;
        std::span<kaki::ecs::EntityId> entityRefs;
    };

    typedef void (*asset_handle_fn)(kaki::ecs::Registry& registry, size_t assetCount, AssetData*, void*);
    struct DependsOn {};
    struct ComponentAssetHandler {
        asset_handle_fn load {};
    };

    template<class T>
    ComponentAssetHandler serializeComponentAssetHandler() {
        return ComponentAssetHandler {
            .load = [](kaki::ecs::Registry& registry, size_t assetCount, AssetData* data, void* pc) {
                auto* c = static_cast<T*>(pc);
                for(size_t i = 0; i < assetCount; i++) {
                    membuf buf(data[i].data);
                    std::istream bufStream(&buf);
                    cereal::BinaryInputArchive archive(bufStream);
                    archive(c[i]);
                }
            },
        };
    };

    void loadPackage(flecs::world& world, const char* path, JobCtx ctx);
    void loadPackage(kaki::ecs::Registry& registry, const char* path);

    void registerAssetComponents(kaki::ecs::Registry& registry);
}