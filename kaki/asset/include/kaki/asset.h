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

    struct AssetData {
        std::span<uint8_t> data;
    };

    typedef void (*asset_handle_fn)(flecs::iter, AssetData*, void*);
    struct DependsOn {};
    struct ComponentAssetHandler {
        asset_handle_fn load {};
    };

    flecs::entity loadPackage(flecs::world& world, const char* path);

    flecs::entity instanciatePackage(flecs::world& world, const Package& package, std::span<uint8_t> data);
}