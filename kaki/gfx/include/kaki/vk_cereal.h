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