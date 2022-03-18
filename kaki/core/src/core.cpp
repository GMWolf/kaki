//
// Created by felix on 19/02/2022.
//

#include "core.h"

#include <transform.h>
#include <cereal/archives/binary.hpp>
#include <kaki/asset.h>
#include <kaki/vec_cereal.h>
#include <optional>

template <class T>
using optionalColumn = std::optional<flecs::column<T>>;


void applyTransformHeirarchy(flecs::iter iter, flecs::column<kaki::Transform> worldTransform, flecs::column<kaki::Transform> transform, optionalColumn<kaki::Transform> parentTransform) {

    if (parentTransform) {
        for (auto i: iter) {
            worldTransform[i] = parentTransform.value()[i].apply(transform[i]);
        }
    } else {
        for (auto i: iter) {
            worldTransform[i] = transform[i];
        }
    }
}

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
            .member(vec3, "position")
            .member(scale, "scale")
            .member(quat, "orientation");

    //world.component<Transform>()
    //        .add(flecs::With, world.pair<Transform, WorldSpace>());

    world.entity("TransformLoader").set<ComponentAssetHandler, Transform>(serializeComponentAssetHandler<Transform>());

    world.system("TransformSystem").kind(flecs::PostUpdate)
        .term<Transform, WorldSpace>().inout(flecs::Out)
        .term<Transform>().inout(flecs::In)
        .term<Transform>().super(flecs::ChildOf, flecs::Cascade).oper(flecs::Optional).inout(flecs::In)
        .iter([](flecs::iter iter) {
            optionalColumn<Transform> parentTransform = iter.is_set(3) ? iter.term<Transform>(3) : optionalColumn<Transform>();
            applyTransformHeirarchy(iter, iter.term<Transform>(1), iter.term<Transform>(2), parentTransform);
        });

}

