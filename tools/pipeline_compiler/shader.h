//
// Created by felix on 12/03/2022.
//

#pragma once

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include <cstdio>
#include <vector>
#include <vulkan/vulkan.h>
#include <span>
#include <optional>
#include <kaki/pipeline.h>

std::string readFile(const char* path) {

    std::string ret;

    FILE *inFile = fopen(path, "r");
    fseek(inFile, 0, SEEK_END);
    size_t fileSize = ftell(inFile);
    fseek(inFile, 0, SEEK_SET);
    ret.resize(fileSize);
    fread(ret.data(), 1, fileSize, inFile);
    fclose(inFile);

    return ret;
}

struct CompileDefinition {
    std::string name;
    std::string value;
};

struct ShaderModule {
    std::vector<VkPushConstantRange> pushConstantRanges;
    std::vector<kaki::DescriptorSet> descSets;
    std::vector<uint32_t> code;
};

std::optional<ShaderModule> compileModule(const char* sourcePath, shaderc_shader_kind shaderKind, std::span<CompileDefinition> defs) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetTargetSpirv(shaderc_spirv_version_1_0);
    auto source = readFile(sourcePath);

    for(auto& def : defs) {
        options.AddMacroDefinition(def.name, def.value);
    }
    auto preprocessed = compiler.PreprocessGlsl(source, shaderKind, sourcePath, options);

    if (preprocessed.GetCompilationStatus() != shaderc_compilation_status_success) {
        fprintf(stderr, "%s", preprocessed.GetErrorMessage().c_str());
        return {};
    }

    auto compiled = compiler.CompileGlslToSpv({preprocessed.cbegin(), preprocessed.cend()},
                                              shaderKind, sourcePath, options);

    if (compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
        fprintf(stderr, "%s", compiled.GetErrorMessage().c_str());
        return {};
    }

    auto compiledWords = std::span(compiled.cbegin(), compiled.cend());

    ShaderModule shdmodule;

    shdmodule.code = std::vector(compiledWords.begin(), compiledWords.end());

    {
        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(compiledWords.size_bytes(), compiledWords.data(), &module);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to reflect shader");
            return {};
        }

        uint32_t descriptorSetCount;
        spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, nullptr);
        std::vector<SpvReflectDescriptorSet*> sets(descriptorSetCount);
        spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, sets.data());

        for(SpvReflectDescriptorSet* set : sets) {
            kaki::DescriptorSet& kakiDescSet = shdmodule.descSets.emplace_back();
            kakiDescSet.index = set->set;
            for(SpvReflectDescriptorBinding* binding : std::span(set->bindings, set->binding_count)) {
                kakiDescSet.bindingNames.emplace_back(binding->name);
                kakiDescSet.bindings.push_back(VkDescriptorSetLayoutBinding{
                        .binding = binding->binding,
                        .descriptorType = static_cast<VkDescriptorType>(binding->descriptor_type),
                        .descriptorCount = binding->count,
                        .stageFlags = module.shader_stage,
                });
            }
        }

        uint32_t pushConstantCount;
        spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr);
        std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
        spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstants.data());


        for(SpvReflectBlockVariable* block : pushConstants) {
            shdmodule.pushConstantRanges.push_back(VkPushConstantRange {
                    .stageFlags = module.shader_stage,
                    .offset = block->offset,
                    .size = block->size,
            });
        }
    }

    return shdmodule;
}