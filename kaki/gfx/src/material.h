//
// Created by felix on 12/03/2022.
//

#pragma once

#include <vulkan/vulkan.h>
#include <flecs.h>
#include <kaki/asset.h>

namespace kaki {

    struct Material {
        flecs::entity_t albedo;
        flecs::entity_t normal;
        flecs::entity_t metallicRoughness;
        flecs::entity_t ao;
        flecs::entity_t emissive;

        VkDescriptorSet descriptorSet;
    };


    void loadMaterials(JobCtx ctx, flecs::world& world, size_t assetCount, AssetData* data, void* pmaterial);

}