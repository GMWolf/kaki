//
// Created by felix on 27/02/2022.
//

#pragma once
#include "vk.h"

namespace kaki {
    struct ShaderInput {
        const char* name;
        VkImageView imageView;
    };

    void fillDescSetWrites(kaki::VkGlobals& vk, VkDescriptorSet vkSet, kaki::DescriptorSet& descSetInfo, std::span<ShaderInput> shaderInputs, std::span<VkWriteDescriptorSet> writes, std::span<VkDescriptorImageInfo> imageInfos );

    void updateDescSets( kaki::VkGlobals& vk, std::span<VkDescriptorSet> descSets, const kaki::Pipeline& pipeline, std::span<ShaderInput> shaderInputs);
}