//
// Created by felix on 12/01/2022.
//

#pragma once

#include <flecs.h>
#include <span>

namespace kaki {

    struct Package {

        struct Entity {
            std::string name;
        };

        struct ComponentEntry {
            size_t entity;
            size_t dataOffset;
            size_t dataSize;
        };

        struct ComponentGroup {
            std::string type;
            size_t componentBegin;
            size_t componentEnd;
        };

        std::string name;
        std::vector<uint8_t> data;
        std::vector<Entity> entities;
        std::vector<ComponentEntry> componentEntries;
        std::vector<ComponentGroup> componentGroups;
    };

    struct Asset {
        const char* path;
    };

    typedef void (*component_deserialize_fn)(flecs::entity entity, std::span<uint8_t> data);

    struct ComponentLoader {
        component_deserialize_fn deserialize;
    };

    typedef void (*asset_handle_fn)(flecs::iter, Asset*);

    struct DependsOn {};

    struct AssetHandler {
        asset_handle_fn load;
    };

    flecs::entity loadPackage(flecs::world& world, const char* path);

    flecs::entity loadAssets(flecs::world& world, const char* path);
}