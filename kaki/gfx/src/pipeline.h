//
// Created by felix on 03-01-22.
//

#ifndef KAKI_PIPELINE_H
#define KAKI_PIPELINE_H

#include "vk.h"
#include <flecs.h>
#include "shader.h"

namespace kaki {

    struct Pipeline {
        VkPipelineLayout pipelineLayout {};
        VkPipeline pipeline {};
        std::vector<DescriptorSet> descriptorSets;

        const DescriptorSet* getDescSet(uint32_t setIndex) const;

    };

    Pipeline createPipeline(const VkGlobals& vk, flecs::entity scope, std::span<uint8_t> pipelineData);
}

#endif //KAKI_PIPELINE_H
