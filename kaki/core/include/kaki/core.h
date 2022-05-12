//
// Created by felix on 19/02/2022.
//

#pragma once

#include <flecs.h>
#include <kaki/ecs.h>

namespace kaki {

    struct core {
        explicit core(flecs::world& world);
    };

    void registerCoreModule(kaki::ecs::Registry& registry);

}