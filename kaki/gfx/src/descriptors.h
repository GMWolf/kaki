//
// Created by felix on 27/02/2022.
//

#pragma once
#include "vk.h"
#include "pipeline.h"
#include <variant>
namespace kaki {
    struct ShaderInput {
        struct Image {
            VkImageView view;
            VkSampler sampler;
        };

        const char* name;
        std::variant<Image, VkBuffer> view;
    };

    struct DescSetWriteCtx {
        std::vector<VkWriteDescriptorSet> descSetWrites;
        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkDescriptorBufferInfo> bufferInfos;

        void reserve(size_t count);
    };

    void addDescSetWrites(DescSetWriteCtx& ctx, VkDescriptorSet vkSet, const kaki::DescriptorSet &descSetInfo, std::span<ShaderInput> shaderInputs);

    void updateDescSets( kaki::VkGlobals& vk, std::span<VkDescriptorSet> descSets, const kaki::Pipeline& pipeline, std::span<ShaderInput> shaderInputs);

    void updateDescSets( kaki::VkGlobals& vk, DescSetWriteCtx& ctx);
}