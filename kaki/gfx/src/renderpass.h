//
// Created by felix on 27/02/2022.
//

#pragma once

#include <vulkan/vulkan.h>
#include <span>
#include <optional>
#include <vector>

inline VkRenderPass createCompatRenderPass(VkDevice device, std::span<VkFormat> colorFormats, std::optional<VkFormat> depthFormat) {

    uint32_t attachmentCount = colorFormats.size() + (depthFormat.has_value() ? 1 : 0);
    std::vector<VkAttachmentDescription> attachments(attachmentCount);
    std::vector<VkAttachmentReference> attachmentReferences(attachmentCount);

    for(uint32_t index = 0; index < colorFormats.size(); index++) {
        attachments[index] = VkAttachmentDescription {
                .flags = 0,
                .format = colorFormats[index],
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        attachmentReferences[index] = VkAttachmentReference {
                .attachment = index,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
    }

    if (depthFormat) {
        attachments.back() = VkAttachmentDescription {
                .flags = 0,
                .format = depthFormat.value(),
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        attachmentReferences.back() = VkAttachmentReference {
                .attachment = static_cast<uint32_t>(attachments.size() - 1),
                .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    VkSubpassDescription subpass {
            .flags = 0,
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachments = attachmentReferences.data(),
            .pDepthStencilAttachment = depthFormat.has_value() ? &attachmentReferences.back() : nullptr,
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .flags = 0,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .subpassCount = 1,
            .pSubpasses = &subpass,
    };

    VkRenderPass renderPass;
    vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    return renderPass;
}