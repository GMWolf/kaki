//
// Created by felix on 03-01-22.
//

#ifndef KAKI_PIPELINE_H
#define KAKI_PIPELINE_H

#include <vulkan/vulkan.h>
#include <flecs.h>
#include "shader.h"

namespace kaki {

    struct Pipeline {
        VkPipelineLayout pipelineLayout {};
        VkPipeline pipeline {};
        std::vector<DescriptorSet> descriptorSets;
    };

    Pipeline createPipeline(VkDevice device, VkRenderPass renderpass, const kaki::ShaderModule* vertexModule, const kaki::ShaderModule* fragmentModule);
}

#endif //KAKI_PIPELINE_H
