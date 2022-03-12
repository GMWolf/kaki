//
// Created by felix on 03-01-22.
//

#ifndef KAKI_PIPELINE_H
#define KAKI_PIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <span>

namespace kaki {

    struct DescriptorSet {
        uint8_t index;
        std::vector<std::string> bindingNames;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        VkDescriptorSetLayout layout;
    };

    struct Pipeline {
        VkPipelineLayout pipelineLayout {};
        VkPipeline pipeline {};

        std::vector<DescriptorSet> descriptorSets;
        std::vector<VkPushConstantRange> pushConstantRanges;

        const DescriptorSet* getDescSet(uint32_t setIndex) const;
    };

    template<class Archive>
    void serialize(Archive& archive, DescriptorSet& descSet) {
        archive(descSet.index, descSet.bindingNames, descSet.bindings);
    }

    template<class Archive>
    void serialize(Archive& archive, Pipeline& pipeline) {
        archive(pipeline.descriptorSets, pipeline.pushConstantRanges);
    }


}

#endif //KAKI_PIPELINE_H
