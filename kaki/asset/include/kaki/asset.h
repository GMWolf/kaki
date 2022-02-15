//
// Created by felix on 12/01/2022.
//

#pragma once

#include <flecs.h>
#include <span>
#include <string>
#include <vector>
#include "package.h"

namespace kaki {

    struct Asset {
        const char* path;
    };

    typedef void* (*component_deserialize_fn)(flecs::entity& parent, size_t count, std::span<uint8_t> data);
    typedef void (*component_free_fn)(flecs::world& world, size_t count, void* data);

    struct ComponentLoader {
        component_deserialize_fn deserialize;
        component_free_fn free;
    };

    typedef void (*asset_handle_fn)(flecs::iter, Asset*);

    struct DependsOn {};

    struct AssetHandler {
        asset_handle_fn load;
    };

    flecs::entity loadPackage(flecs::world& world, const char* path);

    flecs::entity instanciatePackage(flecs::world& world, const Package& package, std::span<uint8_t> data);

    flecs::entity loadAssets(flecs::world& world, const char* path);
}