//
// Created by felix on 06/03/2022.
//

#pragma once

#include <vulkan/vulkan.h>

template<class Archive>
void serialize(Archive& archive, VkPipelineColorBlendAttachmentState& blend) {
    archive(blend.blendEnable);
    archive(blend.srcColorBlendFactor, blend.dstColorBlendFactor, blend.colorBlendOp);
    archive(blend.srcAlphaBlendFactor, blend.dstAlphaBlendFactor, blend.alphaBlendOp);
    archive(blend.colorWriteMask);
}

template<class Archive>
void serialize(Archive& archive, VkPushConstantRange& pc) {
    archive(pc.stageFlags, pc.offset, pc.size);
}

template<class Archive>
void serialize(Archive& archive, VkDescriptorSetLayoutBinding& b) {
    archive(b.binding, b.descriptorType, b.descriptorCount, b.stageFlags);
}