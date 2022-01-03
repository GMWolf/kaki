//
// Created by felix on 02-01-22.
//

#ifndef KAKI_SHADER_H
#define KAKI_SHADER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <cereal/types/vector.hpp>

namespace kaki {

    struct ShaderModule {
        VkShaderModule module;
        std::vector<uint32_t> code;
        std::vector<VkPushConstantRange> pushConstantRanges;
    };

    template<class Archive>
    void serialize(Archive& archive, ShaderModule& module) {
        archive(module.code, module.pushConstantRanges);
    }

    kaki::ShaderModule loadShaderModule(VkDevice device, const char* path);
}

template<class Archive>
void serialize(Archive& archive, VkPushConstantRange& pc) {
    archive(pc.stageFlags, pc.offset, pc.size);
}

#endif //KAKI_SHADER_H
