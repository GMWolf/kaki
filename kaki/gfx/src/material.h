//
// Created by felix on 12/03/2022.
//

#pragma once

#include <vulkan/vulkan.h>
#include <flecs.h>
#include <kaki/asset.h>

namespace kaki {

    struct Material {
        kaki::ecs::EntityId albedo;
        kaki::ecs::EntityId normal;
        kaki::ecs::EntityId metallicRoughness;
        kaki::ecs::EntityId ao;
        kaki::ecs::EntityId emissive;

        VkDescriptorSet descriptorSet;
    };


    void loadMaterials(JobCtx ctx, flecs::world& world, size_t assetCount, AssetData* data, void* pmaterial);

}