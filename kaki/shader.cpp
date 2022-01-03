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

    module.module = createModule(device, module.code);

    // we no longer need to keep the code around
    module.code.clear();
    module.code.shrink_to_fit();

    return module;
}