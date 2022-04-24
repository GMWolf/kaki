//
// Created by felix on 24/02/2022.
//

#pragma once

#include <flecs.h>
#include <imgui.h>
#include <kaki/ecs.h>
#include <kaki/ecs/childof.h>

namespace kaki {

    struct GuiSelectedEntity {
        flecs::entity entity;
    };

    static void guiSelectedEntity(GuiSelectedEntity& selected) {

        if (selected.entity.is_alive()) {
            ImGui::Begin("Inspector");

            const char* name = selected.entity.name().c_str();
            if (strlen(name) == 0) {
                name = "<unnamed>";
            }
            ImGui::Text("Name: %s", name);

            for (auto t: selected.entity.type().vector()) {
                auto te = selected.entity.world().entity(t);
                if (!te.has_role()) {
                    ImGui::Text("%s", te.name().c_str());
                } else if (te.is_pair()){
                    ImGui::Text("%s. %s", te.relation().name().c_str(), te.object().name().c_str());
                }
            }

            ImGui::End();
        }
    }

    void printEntity(flecs::entity entity) {

        auto selected = entity.world().get<GuiSelectedEntity>();
        bool isSelected = selected && selected->entity == entity;

        bool hasChildren = entity.world().filter_builder().term(flecs::ChildOf, entity).build().iter().is_true();

        const char* fmt = entity.name().size() > 0 ? entity.name().c_str() : "%lu";


        bool opened = ImGui::TreeNodeEx( (void*)entity.id() ,
                                          ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                          (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                                  (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf), fmt, entity.id());
        if (ImGui::IsItemClicked()) {
            entity.world().set(GuiSelectedEntity{entity});
        }

        if (opened)
        {
            entity.children(printEntity);
            ImGui::TreePop();
        };
    }

    void printEntity(ecs::Registry& registry, ecs::EntityId entity) {
        auto* iden = registry.get<ecs::Identifier>( entity );
        const char* name;
        if (iden) {
            name = iden->name.c_str();
        } else {
            name = "<%d>";
        }

        bool isSelected = false;
        bool hasChildren = true;

        bool opened = ImGui::TreeNodeEx( (void*)entity.id ,
                                         ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                         (isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
                                         (hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf), name, entity.id);

        if (opened)
        {
            for(auto& chunk : ecs::query(registry, ecs::Query().all(ecs::ComponentType(ecs::ComponentTrait<ecs::ChildOf>::id, entity))))
            {
                for (const auto& [id] : ecs::ChunkView<ecs::EntityId>( chunk )) {
                    printEntity(registry, id);
                }
            }

            //entity.children(printEntity);
            ImGui::TreePop();
        };
    }

    void entityTree(ecs::Registry& registry) {

        ImGui::Begin("Hierarchy");

        //world.filter_builder()
        //    .term(flecs::ChildOf, flecs::Wildcard).oper(flecs::Not)
        //    .build().each(printEntity);

        //auto q = ecs::Query().none<kaki::ecs::ChildOf>();
        auto q = ecs::Query().none<kaki::ecs::ChildOf>();
        auto qr = ecs::query(registry, q);

        for(auto& chunk : qr)
        {
            for (const auto& [id] : ecs::ChunkView<ecs::EntityId>( chunk )) {
                printEntity(registry, id);
            }
        }


        ImGui::End();

        //world.each(guiSelectedEntity);
    }

}