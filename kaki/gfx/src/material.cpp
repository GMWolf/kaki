//
// Created by felix on 12/03/2022.
//

#include "material.h"
#include "descriptors.h"
#include "image.h"

namespace kaki {

    void loadMaterials(flecs::iter iter, AssetData *data, void *pmaterial) {
        auto world = iter.world();

        auto materials = static_cast<Material*>(pmaterial);
        auto& vk = *iter.world().get_mut<VkGlobals>();

        auto pipeline = world.lookup("testpackage::shade").get<kaki::Pipeline>();

        auto descSet = pipeline->getDescSet(2);

        if (!descSet) {
            return;
        }

        std::vector<VkDescriptorSet> descSets(iter.count());

        {
            std::vector<VkDescriptorSetLayout> descSetLayouts(iter.count(), descSet->layout);

            VkDescriptorSetAllocateInfo descAlloc{
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .descriptorPool = vk.staticDescPool,
                    .descriptorSetCount = static_cast<uint32_t>(iter.count()),
                    .pSetLayouts = descSetLayouts.data(),
            };

            vkAllocateDescriptorSets(vk.device, &descAlloc, descSets.data());
        }


        DescSetWriteCtx descCtx;
        descCtx.reserve( iter.count() * descSet->bindings.size() );

        for(auto i : iter) {
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

            auto albedoImage = flecs::entity(world, materials[i].albedo).get<kaki::Image>();
            auto normalImage = flecs::entity(world, materials[i].normal).get<kaki::Image>();
            auto metallicRoughnessImage = flecs::entity(world, materials[i].metallicRoughness).get<kaki::Image>();
            auto aoImage = flecs::entity(world, materials[i].ao).get<kaki::Image>();
            auto emissiveImage = flecs::entity(world, materials[i].emissive).get<kaki::Image>();

            descCtx.add(descSets[i], *descSet, {
                    {"albedoTexture", ShaderInput::Image{albedoImage->view, vk.sampler}},
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
