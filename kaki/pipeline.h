//
// Created by felix on 03-01-22.
//

#ifndef KAKI_PIPELINE_H
#define KAKI_PIPELINE_H

#include <vulkan/vulkan.h>
#include <flecs.h>
#include "shader.h"

namespace kaki {

    namespace asset {
        struct Pipeline {};
    }

    struct Pipeline {
        VkPipelineLayout pipelineLayout {};
        VkPipeline pipeline {};
    };

    Pipeline createPipeline(VkDevice device, VkRenderPass renderpass, const kaki::ShaderModule* vertexModule, const kaki::ShaderModule* fragmentModule);
}

#endif //KAKI_PIPELINE_H
