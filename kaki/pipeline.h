//
// Created by felix on 03-01-22.
//

#ifndef KAKI_PIPELINE_H
#define KAKI_PIPELINE_H

#include <vulkan/vulkan.h>

namespace kaki {
    struct Pipeline {
        VkPipelineLayout pipelineLayout {};
        VkPipeline pipeline {};
    };

    Pipeline createPipeline(VkDevice device, VkRenderPass renderpass, const char* vertexPath, const char* fragmentPath);
}

#endif //KAKI_PIPELINE_H
