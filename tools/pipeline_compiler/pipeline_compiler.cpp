//
// Created by felix on 15/02/2022.
//
#define CEREAL_RAPIDJSON_NAMESPACE cereal_rapidjson

#include <fstream>
#include <rapidjson/document.h>
#include <kaki/package.h>
#include <filesystem>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#define VK_VALUE_SERIALIZATION_CONFIG_MAIN
#include <vk_value_serialization.hpp>
#include <kaki/vk_cereal.h>
#include <kaki/pipeline.h>
#include "shader.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    fs::path inputPath = argv[1];

    auto assetName = inputPath.stem();

    fs::path outputFile = argv[2];
    auto outputDataFile = fs::path(outputFile).replace_extension("bin");

    rapidjson::Document doc;
    std::ifstream inputStream(inputPath);
    std::string str((std::istreambuf_iterator<char>(inputStream)),
                    std::istreambuf_iterator<char>());
    doc.ParseInsitu(str.data());

    std::ofstream outputData(outputDataFile, std::ios::binary);
    {
        cereal::BinaryOutputArchive dataArchive(outputData);
        std::string vertexSrcPath = doc["vertex"].GetString();
        std::string fragmentSrcPath = doc["fragment"].GetString();

        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        std::vector<VkFormat> colorFormats;
        for(auto& attachment : doc["attachments"].GetArray()) {
            auto& format = colorFormats.emplace_back();
            auto& blend = colorBlendAttachments.emplace_back();
            vk_parse("VkFormat", attachment["format"].GetString(), &format);
            blend.blendEnable = attachment["blend_enable"].GetBool();
            vk_parse("VkBlendFactor", attachment["src_blend_factor"].GetString(), &blend.srcColorBlendFactor);
            vk_parse("VkBlendFactor", attachment["dst_blend_factor"].GetString(), &blend.dstColorBlendFactor);
            vk_parse("VkBlendOp", attachment["color_blend_op"].GetString(), &blend.colorBlendOp);
            vk_parse("VkBlendFactor", attachment["src_alpha_factor"].GetString(), &blend.srcAlphaBlendFactor);
            vk_parse("VkBlendFactor", attachment["dst_alpha_factor"].GetString(), &blend.dstAlphaBlendFactor);
            vk_parse("VkBlendOp", attachment["alpha_blend_op"].GetString(), &blend.alphaBlendOp);
            blend.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                                   VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
        }

        std::optional<VkFormat> depthFormat;
        if (doc.HasMember("depthAttachment")) {
            VkFormat format;
            vk_parse("VkFormat", doc["depthAttachment"]["format"].GetString(), &format);
            depthFormat = format;
        }

        VkCullModeFlags cullMode;
        vk_parse("VkCullModeFlags", doc["cullMode"].GetString(), &cullMode);
        VkCompareOp depthCompareOp;
        vk_parse("VkCompareOp", doc["depthCompare"].GetString(), &depthCompareOp);
        bool depthTestEnable = doc["depthTest"].GetBool();
        bool depthWriteEnable = doc["depthWrite"].GetBool();


        auto vertexModule = compileModule((inputPath.parent_path() / vertexSrcPath).c_str(), shaderc_vertex_shader, {});
        if (!vertexModule) {
            fprintf(stderr, "Failed compiling %s\n", vertexSrcPath.c_str());
            return -1;
        }

        auto fragmentModule = compileModule((inputPath.parent_path() / fragmentSrcPath).c_str(), shaderc_fragment_shader, {});
        if (!fragmentModule) {
            fprintf(stderr, "Failed compiling %s\n", fragmentSrcPath.c_str());
            return -1;
        }

        std::vector<VkPushConstantRange> pushConstants;
        pushConstants.insert(pushConstants.end(), vertexModule->pushConstantRanges.begin(), vertexModule->pushConstantRanges.end());
        pushConstants.insert(pushConstants.end(), fragmentModule->pushConstantRanges.begin(), fragmentModule->pushConstantRanges.end());

        std::vector<kaki::DescriptorSet> descSets;
        descSets.insert(descSets.end(), vertexModule->descSets.begin(), vertexModule->descSets.end());
        descSets.insert(descSets.end(), fragmentModule->descSets.begin(), fragmentModule->descSets.end());

        dataArchive(colorBlendAttachments, colorFormats, depthFormat, cullMode, depthTestEnable, depthCompareOp, depthWriteEnable);
        dataArchive(pushConstants, descSets);
        dataArchive(vertexModule->code);
        dataArchive(fragmentModule->code);

    }

    kaki::Package package {
            .entities = {{assetName}},
            .tables = {kaki::Package::Table{
                    .entityFirst = 0,
                    .entityCount = 1,
                    .components = { {
                        .type = {"kaki::gfx::Pipeline"},
                        .data = {{0, static_cast<uint64_t>(outputData.tellp())}},
                    }
                    },
            }
            },
            .dataFile = outputDataFile.filename(),
    };

    std::ofstream outputStream(outputFile);
    cereal::JSONOutputArchive archive(outputStream);
    archive(package);
    return 0;
}