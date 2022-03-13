//
// Created by felix on 03-01-22.
//

#include "pipeline.h"
#include <span>
#include "renderpass.h"
#include "membuf.h"
#include <ranges>
#include <vk_cereal.h>
#include <cereal/types/optional.hpp>
#include <algorithm>
#include "vk.h"
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>

static VkPipelineLayout createPipelineLayout(VkDevice device, std::span<VkPushConstantRange> pushConstantRanges, std::span<kaki::DescriptorSet> descSets)
{
    std::sort(descSets.begin(), descSets.end(), [](const kaki::DescriptorSet& a, const kaki::DescriptorSet& b) {
        return a.index < b.index;
    });

    std::vector<VkDescriptorSetLayout> descSetLayouts(descSets.size());
    for(auto i : std::views::iota(0u, descSets.size())) {

        VkDescriptorSetLayoutCreateInfo descSetLayoutInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(descSets[i].bindings.size()),
            .pBindings = descSets[i].bindings.data(),
        };

        vkCreateDescriptorSetLayout(device, &descSetLayoutInfo, nullptr, &descSets[i].layout);


        descSetLayouts[i] = descSets[i].layout;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(descSetLayouts.size()),
            .pSetLayouts = descSetLayouts.data(),
            .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
            .pPushConstantRanges = pushConstantRanges.data(),
    };

    VkPipelineLayout layout;
    vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout);
    return layout;
}

namespace kaki {

    static VkShaderModule createModule(const VkGlobals &vk, std::span<uint32_t> code) {
        VkShaderModuleCreateInfo createInfo {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = code.size_bytes(),
                .pCode = code.data(),
        };


        VkShaderModule shader_module;
        if(vkCreateShaderModule(vk.device, &createInfo, nullptr, &shader_module) != VK_SUCCESS){
            return VK_NULL_HANDLE;
        }

        return shader_module;
    }

    Pipeline createPipeline(const VkGlobals &vk, flecs::entity scope, std::span<uint8_t> pipelineData) {

        Pipeline pipeline;

        membuf buf(pipelineData);
        std::istream bufStream(&buf);
        cereal::BinaryInputArchive archive(bufStream);

        std::string vertexName;
        std::string fragmentName;
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
        std::vector<VkFormat> colorFormats;
        std::optional<VkFormat> depthFormat;
        VkCullModeFlags cullMode;
        VkCompareOp depthCompareOp;
        bool depthTestEnable;
        bool depthWriteEnable;

        archive(colorBlendAttachments, colorFormats, depthFormat, cullMode, depthTestEnable, depthCompareOp, depthWriteEnable);
        archive(pipeline.pushConstantRanges, pipeline.descriptorSets);

        VkShaderModule vertexModule, fragmentModule;

        {
            std::vector<uint32_t> code;
            archive(code);
            vertexModule = createModule(vk, code);
            archive(code);
            fragmentModule = createModule(vk, code);
        }

        for (auto &format: colorFormats) {
            if (format == VK_FORMAT_UNDEFINED) {
                format = vk.swapchain.image_format;
            }
        }

        auto renderpass = createCompatRenderPass(vk.device, colorFormats, depthFormat);

        auto pipelineLayout = createPipelineLayout(vk.device, pipeline.pushConstantRanges, pipeline.descriptorSets);

        VkPipelineShaderStageCreateInfo shaderStageCreateInfo[]{
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = vertexModule,
                        .pName = "main"
                },
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = fragmentModule,
                        .pName = "main"
                }
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = false};

        // A dummy viewport and scissor. this needs to get set by cmd buffer
        VkViewport viewport{
                .x = 0.0f,
                .y = 0.0f,
                .width = 1.0f,
                .height = 1.0f,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };

        VkRect2D scissor{
                .offset = {0, 0},
                .extent = {1, 1},
        };

        VkPipelineViewportStateCreateInfo viewportState{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                .viewportCount = 1,
                .pViewports = &viewport,
                .scissorCount = 1,
                .pScissors = &scissor,
        };

        VkPipelineRasterizationStateCreateInfo rasterizer{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = false,
                .rasterizerDiscardEnable = false,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = cullMode,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = false,
                .depthBiasConstantFactor = 0.0f,
                .depthBiasClamp = 0.0f,
                .depthBiasSlopeFactor = 0.0f,
                .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = false,
                .minSampleShading = 1.0f,
                .pSampleMask = nullptr,
                .alphaToCoverageEnable = false,
                .alphaToOneEnable = false,
        };

        VkPipelineColorBlendStateCreateInfo colorBlending{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = false,
                .logicOp = VK_LOGIC_OP_COPY,
                .attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
                .pAttachments = colorBlendAttachments.data(),
        };

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = 2,
                .pDynamicStates = dynamicStates,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = depthTestEnable,
                .depthWriteEnable = depthWriteEnable,
                .depthCompareOp = depthCompareOp,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE,
                .front = {},
                .back = {},
                .minDepthBounds = 0.0f,
                .maxDepthBounds = 1.0f,
        };

        VkGraphicsPipelineCreateInfo pipelineInfo{
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = 2,
                .pStages = shaderStageCreateInfo,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pDepthStencilState = &depthStencil,
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicState,
                .layout = pipelineLayout,
                .renderPass = renderpass,
                .subpass = 0,
                .basePipelineHandle = {},
                .basePipelineIndex = -1,
        };

        vkCreateGraphicsPipelines(vk.device, nullptr, 1, &pipelineInfo, nullptr, &pipeline.pipeline);

        pipeline.pipelineLayout = pipelineLayout;
        vkDestroyRenderPass(vk.device, renderpass, nullptr);
        vkDestroyShaderModule(vk.device, vertexModule, nullptr);
        vkDestroyShaderModule(vk.device, fragmentModule, nullptr);

        return pipeline;
    }
}

const kaki::DescriptorSet* kaki::Pipeline::getDescSet(uint32_t setIndex) const {
    for(const auto& set : descriptorSets) {
        if (set.index == setIndex) {
            return &set;
        }
    }
    return nullptr;
}
