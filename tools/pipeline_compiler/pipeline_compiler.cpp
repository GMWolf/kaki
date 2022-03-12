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
        std::string vertex = doc["vertex"].GetString();
        std::string fragment = doc["fragment"].GetString();

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

        dataArchive(vertex, fragment, colorBlendAttachments, colorFormats, depthFormat, cullMode, depthTestEnable, depthCompareOp, depthWriteEnable);
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