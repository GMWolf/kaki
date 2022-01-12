//
// Created by felix on 12/01/2022.
//

#pragma once

#include <flecs.h>

namespace kaki {

    struct Asset {
        const char* path;
    };

    typedef void (*asset_handle_fn)(flecs::iter, Asset*);

    struct AssetHandler {
        const char* type;
        asset_handle_fn load;
    };


    void importAssetManager(flecs::world& world);
}