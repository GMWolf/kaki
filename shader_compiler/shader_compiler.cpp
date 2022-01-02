//
// Created by felix on 01-01-22.
//

#include <cstdio>
#include <shaderc/shaderc.hpp>
#include <span>
#include <cstring>
#include <spirv_reflect.h>
#include <vector>
#include "../kaki/shader.h"
#include <cereal/archives/binary.hpp>
#include <fstream>

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Wrong number of arguments. Expects 2, got %d\n", argc - 1);
        return 1;
    }

    auto sourcePath = argv[1];
    fprintf(stdout, "Compiling %s\n", sourcePath);
    auto targetPath = argv[2];

    auto ext = get_filename_ext(sourcePath);

    shaderc_shader_kind shaderKind;

    if (strcmp(ext, "vert") == 0) {
        shaderKind = shaderc_vertex_shader;
    } else if (strcmp(ext, "frag") == 0) {
        shaderKind = shaderc_fragment_shader;
    } else {
        fprintf(stderr, "Unknown shader extension .%s\n", ext);
        return 1;
    }

    std::string source;
    {
        FILE *inFile = fopen(sourcePath, "r");
        fseek(inFile, 0, SEEK_END);
        size_t fileSize = ftell(inFile);
        fseek(inFile, 0, SEEK_SET);
        source.resize(fileSize);
        fread(source.data(), 1, fileSize, inFile);
        fclose(inFile);
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    auto preprocessed = compiler.PreprocessGlsl(source, shaderKind, sourcePath, options);

    if (preprocessed.GetCompilationStatus() != shaderc_compilation_status_success) {
        fprintf(stderr, "%s", preprocessed.GetErrorMessage().c_str());
        return -1;
    }

    auto compiled = compiler.CompileGlslToSpv({preprocessed.cbegin(), preprocessed.cend()},
                                              shaderKind, sourcePath, options);

    if (compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
        fprintf(stderr, "%s", compiled.GetErrorMessage().c_str());
        return -1;
    }

    auto compiledWords = std::span(compiled.cbegin(), compiled.cend());

    // do reflection
    {
        kaki::ShaderModule kakiModule;

        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(compiledWords.size_bytes(), compiledWords.data(), &module);
        if (result != SPV_REFLECT_RESULT_SUCCESS) {
            fprintf(stderr, "Failed to reflect shader");
            return 1;
        }

        uint32_t descriptorSetCount;
        spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, nullptr);
        std::vector<SpvReflectDescriptorSet*> sets(descriptorSetCount);
        spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, sets.data());

        for(SpvReflectDescriptorSet* set : sets) {
            for(SpvReflectDescriptorBinding* binding : std::span(set->bindings, set->binding_count)) {

            }
        }


        uint32_t pushConstantCount;
        spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, nullptr);
        std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
        spvReflectEnumeratePushConstantBlocks(&module, &pushConstantCount, pushConstants.data());


        for(SpvReflectBlockVariable* block : pushConstants) {
            kakiModule.pushConstantRanges.push_back(VkPushConstantRange {
                .stageFlags = module.shader_stage,
                .offset = block->offset,
                .size = block->size,
            });
        }

        kakiModule.code.assign(compiledWords.begin(), compiledWords.end());

        std::ofstream os(std::string(targetPath), std::ios::binary);
        cereal::BinaryOutputArchive archive( os );
        archive( kakiModule );
    }

    return 0;
}