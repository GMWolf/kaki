//
// Created by felix on 02-01-22.
//

#ifndef KAKI_SHADER_H
#define KAKI_SHADER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <span>

namespace kaki {

    struct DescriptorSet {
        uint8_t index;
        std::vector<std::string> bindingNames;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        VkDescriptorSetLayout layout;
    };

    struct ShaderModule {
        std::vector<VkPushConstantRange> pushConstantRanges;
        std::vector<DescriptorSet> descSets;

        VkShaderModule module;
    };

    template<class Archive>
    void serialize(Archive& archive, ShaderModule& module) {
        archive(module.pushConstantRanges, module.descSets);
    }

    template<class Archive>
    void serialize(Archive& archive, DescriptorSet& descSet) {
        archive(descSet.index, descSet.bindingNames, descSet.bindings);
    }

    ShaderModule loadShaderModule(VkDevice device, cereal::BinaryInputArchive& archive);
}

template<class Archive>
void serialize(Archive& archive, VkPushConstantRange& pc) {
    archive(pc.stageFlags, pc.offset, pc.size);
}

template<class Archive>
void serialize(Archive& archive, VkDescriptorSetLayoutBinding& b) {
    archive(b.binding, b.descriptorType, b.descriptorCount, b.stageFlags);
}

#endif //KAKI_SHADER_H
