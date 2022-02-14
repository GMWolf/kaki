//
// Created by felix on 01-01-22.
//

#include <cstdio>
#include <shaderc/shaderc.hpp>
#include <span>
#include <cstring>
#include <spirv_reflect.h>
#include <vector>
#include <kaki/shader.h>
#include <sstream>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>
#include <kaki/package.h>
#include <cereal_ext.h>

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

    std::string sourcePath = argv[1];
    fprintf(stdout, "Compiling %s\n", sourcePath.c_str());
    std::string targetPath = argv[2];
    auto binTargetPath = targetPath + ".bin";

    auto ext = sourcePath.substr(sourcePath.find_last_of('.') + 1);
    auto assetName = sourcePath.substr(sourcePath.find_last_of("/\\") + 1);
    std::replace(assetName.begin(), assetName.end(), '.', '_');


    shaderc_shader_kind shaderKind;

    if (ext == "vert") {
        shaderKind = shaderc_vertex_shader;
    } else if (ext == "frag") {
        shaderKind = shaderc_fragment_shader;
    } else {
        fprintf(stderr, "Unknown shader extension .%s\n", ext.c_str());
        return 1;
    }

    std::string source;
    {
        FILE *inFile = fopen(sourcePath.c_str(), "r");
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
    options.SetTargetSpirv(shaderc_spirv_version_1_0);

    auto preprocessed = compiler.PreprocessGlsl(source, shaderKind, sourcePath.c_str(), options);

    if (preprocessed.GetCompilationStatus() != shaderc_compilation_status_success) {
        fprintf(stderr, "%s", preprocessed.GetErrorMessage().c_str());
        return -1;
    }

    auto compiled = compiler.CompileGlslToSpv({preprocessed.cbegin(), preprocessed.cend()},
                                              shaderKind, sourcePath.c_str(), options);

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
            kaki::DescriptorSet& kakiDescSet = kakiModule.descSets.emplace_back();
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
            kakiModule.pushConstantRanges.push_back(VkPushConstantRange {
                .stageFlags = module.shader_stage,
                .offset = block->offset,
                .size = block->size,
            });
        }

        std::vector<uint32_t> code(compiledWords.begin(), compiledWords.end());



        std::ofstream outData(binTargetPath, std::ios::binary | std::ios::out);
        {
            cereal::BinaryOutputArchive dataArchive(outData);
            dataArchive(kakiModule, code);
        }

        std::ofstream os(targetPath);
        cereal::JSONOutputArchive archive( os );

        kaki::Package package{};
        package.dataFile = binTargetPath.substr(binTargetPath.find_last_of("/\\") + 1);
        package.entities = {{assetName}};
        package.tables = {kaki::Package::Table {
                .entityFirst = 0,
                .entityCount = 1,
                .types = {{"kaki::ShaderModule", {}}},
                .typeData = {
                        {0,static_cast<uint64_t>(outData.tellp())}
                },
        }};

        archive(package);
    }

    return 0;
}