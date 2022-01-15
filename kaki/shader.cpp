//
// Created by felix on 03-01-22.
//

#include "shader.h"
#include <fstream>
#include <span>
#include <cereal/archives/binary.hpp>

static VkShaderModule createModule(VkDevice device, std::span<uint32_t> code)
{
    VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size_bytes(),
            .pCode = code.data(),
    };


    VkShaderModule shader_module;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shader_module) != VK_SUCCESS){
        return VK_NULL_HANDLE;
    }

    return shader_module;
}

kaki::ShaderModule kaki::loadShaderModule(VkDevice device, const char* path) {
    std::ifstream is(path);
    cereal::BinaryInputArchive archive(is);

    kaki::ShaderModule module;
    archive(module);

    std::vector<uint32_t> code;
    archive(code);

    module.module = createModule(device, code);

    for(auto& descSet : module.descSets) {

        VkDescriptorSetLayoutCreateInfo descSetLayoutInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = static_cast<uint32_t>(descSet.bindings.size()),
                .pBindings = descSet.bindings.data(),
        };

        vkCreateDescriptorSetLayout(device, &descSetLayoutInfo, nullptr, &descSet.layout);
    }

    return module;
}