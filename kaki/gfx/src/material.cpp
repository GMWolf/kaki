//
// Created by felix on 12/03/2022.
//

#include "material.h"
#include "descriptors.h"
#include "image.h"

namespace kaki {

    void loadMaterials(kaki::ecs::Registry& registry, size_t assetCount, AssetData *data, void *pmaterial) {
        auto materials = static_cast<Material*>(pmaterial);

        auto& vk = *registry.get<VkGlobals>(registry.lookup("Renderer"));

        auto pipeline = registry.get<kaki::Pipeline>(registry.lookup("testpackage::shade"));

        auto descSet = pipeline->getDescSet(2);

        if (!descSet) {
            return;
        }

        std::vector<VkDescriptorSet> descSets(assetCount);

        {
            std::vector<VkDescriptorSetLayout> descSetLayouts(assetCount, descSet->layout);

            VkDescriptorSetAllocateInfo descAlloc{
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .descriptorPool = vk.staticDescPool,
                    .descriptorSetCount = static_cast<uint32_t>(assetCount),
                    .pSetLayouts = descSetLayouts.data(),
            };

            vkAllocateDescriptorSets(vk.device, &descAlloc, descSets.data());
        }


        DescSetWriteCtx descCtx;
        descCtx.reserve( assetCount * descSet->bindings.size() );

        for(size_t i = 0; i < assetCount; i++) {
            membuf buf(data[i].data);

            std::istream bufStream(&buf);
            cereal::BinaryInputArchive archive(bufStream);

            uint64_t albedoRef, normalRef, mrRef, aoRef, emissiveRef;
            archive(albedoRef, normalRef, mrRef, aoRef, emissiveRef);

            size_t matBufferSize;
            archive(cereal::make_size_tag(matBufferSize));
            auto* matBuffer = (uint8_t*)malloc(matBufferSize);
            archive(cereal::binary_data(matBuffer, matBufferSize));

            materials[i].albedo = data->entityRefs[albedoRef];
            materials[i].normal = data->entityRefs[normalRef];
            materials[i].metallicRoughness = data->entityRefs[mrRef];
            materials[i].ao = data->entityRefs[aoRef];
            materials[i].emissive = data->entityRefs[emissiveRef];

            materials[i].descriptorSet = descSets[i];

            auto albedoImage = registry.get<kaki::Image>(materials[i].albedo);  // flecs::entity(world, materials[i].albedo).get<kaki::Image>();
            auto normalImage = registry.get<kaki::Image>(materials[i].normal); // flecs::entity(world, materials[i].normal).get<kaki::Image>();
            auto metallicRoughnessImage = registry.get<kaki::Image>(materials[i].metallicRoughness); //  flecs::entity(world, materials[i].metallicRoughness).get<kaki::Image>();
            auto aoImage = registry.get<kaki::Image>(materials[i].ao); // flecs::entity(world, materials[i].ao).get<kaki::Image>();
            auto emissiveImage = registry.get<kaki::Image>(materials[i].emissive); // flecs::entity(world, materials[i].emissive).get<kaki::Image>();

            descCtx.add(descSets[i], *descSet, {
                    {"albedoTexture", ShaderInput::Image{albedoImage ? albedoImage->view : VK_NULL_HANDLE, vk.sampler}},
                    {"normalTexture", ShaderInput::Image{normalImage ? normalImage->view : VK_NULL_HANDLE, vk.sampler}},
                    {"metallicRoughnessTexture", ShaderInput::Image{metallicRoughnessImage ? metallicRoughnessImage->view : VK_NULL_HANDLE, vk.sampler}},
                    {"aoTexture", ShaderInput::Image{aoImage ? aoImage->view : VK_NULL_HANDLE, vk.sampler}},
                    {"emissiveTexture", ShaderInput::Image{emissiveImage? emissiveImage->view : VK_NULL_HANDLE, vk.sampler}},
                    {"material", std::span<uint8_t>(matBuffer, matBufferSize)},
            });
        }

        descCtx.submit(vk);
    }

}
