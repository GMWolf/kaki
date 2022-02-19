//
// Created by felix on 19/02/2022.
//

#include "core.h"

#include <transform.h>
#include <cereal/archives/binary.hpp>
#include <kaki/asset.h>
#include <kaki/vec_cereal.h>

kaki::core::core(flecs::world &world) {

    world.module<kaki::core>();

    auto vec3 = world.component("vec3")
            .member<float>("x")
            .member<float>("y")
            .member<float>("z");

    auto quat = world.component("quat")
            .member<float>("x")
            .member<float>("y")
            .member<float>("z")
            .member<float>("w");

    auto scale = world.component("scale")
            .member<float>("scale");

    world.component<Transform>()
            .member("position", vec3)
            .member("scale", scale)
            .member("orientation", quat);


    world.entity("TransformLoader").set<ComponentAssetHandler, Transform>(serializeComponentAssetHandler<Transform>());
}

