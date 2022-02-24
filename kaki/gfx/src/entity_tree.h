//
// Created by felix on 24/02/2022.
//

#pragma once

#include <flecs.h>
#include <imgui.h>

namespace kaki {

    static void entityTreeChild(flecs::world& world, flecs::entity e) {
        world.filter_builder()
        .term(flecs::ChildOf, e)
        .build().each([&world](flecs::entity e) {
            if (ImGui::TreeNode(e.name().c_str() ? e.name().c_str() : "<unnamed>")) {

                entityTreeChild(world, e);

                ImGui::TreePop();
            }
        });

    }

    void entityTree(flecs::world& world) {

        world.filter_builder()
            .term(flecs::ChildOf, flecs::Wildcard).oper(flecs::Not)
            .build().each([&world](flecs::entity e) {

                if (ImGui::TreeNode(e.name().c_str() ? e.name().c_str() : "<unnamed>")) {

                    entityTreeChild(world, e);

                    ImGui::TreePop();
                }


            });


    }

}