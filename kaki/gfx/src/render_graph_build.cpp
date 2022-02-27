//
// Created by felix on 27/02/2022.
//

#include "render_graph.h"
#include "vk_mem_alloc.h"
#include <glm/glm.hpp>
#include "vk.h"

namespace kaki {


    static std::vector<VkImageUsageFlags> compileImageUsage(const RenderGraphBuilder &graph) {
        std::vector<VkImageUsageFlags> usage(graph.images.size(), 0);

        for (auto &pass: graph.passes) {
            if (pass.depthAttachment) {
                usage[pass.depthAttachment->image] |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }

            for (auto &attachment: pass.colorAttachments) {
                if (attachment.image != DISPLAY_IMAGE_INDEX) {
                    usage[attachment.image] |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                }
            }
        }
        return usage;
    }


    static void createImageView(VkGlobals& vk, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkImageView* imageView) {
        VkImageSubresourceRange imageRange {
                .aspectMask = aspect,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
        };

        VkImageViewCreateInfo viewInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image =  image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = VkComponentMapping {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = imageRange,
        };

        vkCreateImageView(vk.device, &viewInfo, nullptr, imageView);
    }

    static void createImages(VkGlobals& vk, const RenderGraphBuilder &graph, RenderGraph& rg) {
        rg.images.resize(graph.images.size());

        auto queueIndex = vk.device.get_queue_index(vkb::QueueType::graphics).value();

        auto usages = compileImageUsage(graph);

        for(uint32_t index = 0; index < graph.images.size(); index++) {
            auto& imageInfo = graph.images[index];
            auto& image = rg.images[index];
            VkImageCreateInfo createInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = imageInfo.format,
                .extent = {
                        .width = imageInfo.size.width,
                        .height = imageInfo.size.height,
                        .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = usages[index],
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &queueIndex,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            VmaAllocationCreateInfo allocInfo {
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .preferredFlags = 0,
            };

            vmaCreateImage(vk.allocator, &createInfo, &allocInfo, &image.image, &image.allocation, nullptr);

            if (usages[index] & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                createImageView(vk, image.image, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT, &image.colorView);
            } else {
                image.colorView = VK_NULL_HANDLE;
            }
            if (usages[index] & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
                createImageView(vk, image.image, imageInfo.format, VK_IMAGE_ASPECT_DEPTH_BIT, &image.depthView);
            } else {
                image.depthView = VK_NULL_HANDLE;
            }
        }
    }

    static VkImageView getColorImageView(VkGlobals& vk, RenderGraph& rg, uint32_t imageIndex, uint32_t swapchainIndex) {
        if (imageIndex == DISPLAY_IMAGE_INDEX) {
            return vk.imageViews[swapchainIndex];
        }
        return rg.images[imageIndex].colorView;
    }

    static VkExtent2D getFramebufferSize(VkGlobals& vk, const RenderGraphBuilder& graph, uint32_t passIndex) {
        auto& pass = graph.passes[passIndex];
        for(auto a : pass.colorAttachments) {
            if (a.image == DISPLAY_IMAGE_INDEX) {
                return vk.swapchain.extent;
            }
            auto& image = graph.images[a.image];
            return image.size;
        }
        if (pass.depthAttachment) {
            auto& image = graph.images[pass.depthAttachment->image];
            return image.size;
        }

        return {};
    }

    static VkFormat getImageFormat(VkGlobals& vk, const RenderGraphBuilder& graph, uint32_t imageIndex) {
        if (imageIndex == DISPLAY_IMAGE_INDEX) {
            return vk.swapchain.image_format;
        }
        return graph.images[imageIndex].format;
    }

    static VkAttachmentLoadOp getLoadOp(VkImageLayout layout, bool clear) {
        if (clear) {
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        }
        if (layout == VK_IMAGE_LAYOUT_UNDEFINED) {
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    static VkImageLayout getNextLayout(const RenderGraphBuilder& graph, uint32_t passIndex, uint32_t imageIndex) {
        for(; passIndex < graph.passes.size(); passIndex++) {
            for(auto attachment : graph.passes[passIndex].colorAttachments) {
                if (attachment.image == imageIndex) {
                    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            }
            if (graph.passes[passIndex].depthAttachment && graph.passes[passIndex].depthAttachment->image == imageIndex) {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
        }

        if (imageIndex == DISPLAY_IMAGE_INDEX) {
            return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        } else {
            return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    static void createRenderpasses(VkGlobals& vk, const RenderGraphBuilder& graph, RenderGraph& rg) {

        std::vector<VkImageLayout> imageLayouts(graph.images.size(), VK_IMAGE_LAYOUT_UNDEFINED);
        VkImageLayout displayLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        auto getImageLayout = [&](uint32_t imageIndex) -> VkImageLayout& {
            return imageIndex == DISPLAY_IMAGE_INDEX ? displayLayout : imageLayouts[imageIndex];
        };

        for(uint32_t passIndex = 0; passIndex < graph.passes.size(); passIndex++) {
            const RenderGraphBuilder::Pass& passInfo = graph.passes[passIndex];

            uint32_t attachmentCount = passInfo.colorAttachments.size() + (passInfo.depthAttachment.has_value() ? 1 : 0);
            std::vector<VkAttachmentDescription> attachments(attachmentCount);
            std::vector<VkAttachmentReference> attachmentReferences(attachmentCount);
            for(uint32_t attachmentIndex = 0; attachmentIndex < passInfo.colorAttachments.size(); attachmentIndex++) {
                auto& attachmentInfo = passInfo.colorAttachments[attachmentIndex];
                auto initialLayout = getImageLayout(attachmentInfo.image);
                auto nextLayout = getNextLayout(graph, passIndex + 1, attachmentInfo.image);
                auto finalLayout = nextLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : nextLayout;
                attachments[attachmentIndex] = VkAttachmentDescription {
                    .flags = 0,
                    .format = getImageFormat(vk, graph, attachmentInfo.image),
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp = getLoadOp(initialLayout, attachmentInfo.clear.has_value()),
                    .storeOp = nextLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
                    .initialLayout = initialLayout,
                    .finalLayout = finalLayout,
                };

                attachmentReferences[attachmentIndex] = VkAttachmentReference {
                    .attachment = attachmentIndex,
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };

                if (attachmentInfo.image == DISPLAY_IMAGE_INDEX){
                    displayLayout = finalLayout;
                } else {
                    imageLayouts[attachmentInfo.image] = finalLayout;
                }
            }

            if (passInfo.depthAttachment) {
                auto& attachmentInfo = passInfo.depthAttachment.value();
                auto initialLayout = getImageLayout(attachmentInfo.image);
                auto nextLayout = getNextLayout(graph, passIndex + 1, attachmentInfo.image);
                auto finalLayout = nextLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : nextLayout;
                attachments.back() = VkAttachmentDescription {
                        .flags = 0,
                        .format = getImageFormat(vk, graph, attachmentInfo.image),
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = getLoadOp(initialLayout, attachmentInfo.clear.has_value()),
                        .storeOp = nextLayout == VK_IMAGE_LAYOUT_UNDEFINED ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE,
                        .initialLayout = initialLayout,
                        .finalLayout = finalLayout,
                };

                attachmentReferences.back() = VkAttachmentReference {
                    .attachment = static_cast<uint32_t>(attachments.size() - 1),
                    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                };

                imageLayouts[attachmentInfo.image] = finalLayout;
            }

            VkSubpassDescription subpass {
                    .flags = 0,
                    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                    .colorAttachmentCount = static_cast<uint32_t>(passInfo.colorAttachments.size()),
                    .pColorAttachments = attachmentReferences.data(),
                    .pDepthStencilAttachment = passInfo.depthAttachment.has_value() ? &attachmentReferences.back() : nullptr,
            };


            VkRenderPassCreateInfo renderPassCreateInfo {
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                    .flags = 0,
                    .attachmentCount = static_cast<uint32_t>(attachments.size()),
                    .pAttachments = attachments.data(),
                    .subpassCount = 1,
                    .pSubpasses = &subpass,
            };

            vkCreateRenderPass(vk.device, &renderPassCreateInfo, nullptr, &rg.passes[passIndex].renderPass);
        }
    }

    static void createFramebuffers(VkGlobals& vk, const RenderGraphBuilder& graph, RenderGraph& rg) {

        for (uint32_t passIndex = 0; passIndex < graph.passes.size(); passIndex++){
            auto &passInfo = graph.passes[passIndex];
            auto& pass = rg.passes[passIndex];

            for(uint32_t swapchainIndex = 0; swapchainIndex < vk.swapchain.image_count; swapchainIndex++) {
                std::vector<VkImageView> views(
                        passInfo.colorAttachments.size() + (passInfo.depthAttachment.has_value() ? 1 : 0));

                for (auto attachmentIndex = 0; attachmentIndex < passInfo.colorAttachments.size(); attachmentIndex++) {
                    auto &attachment = passInfo.colorAttachments[attachmentIndex];
                    views[attachmentIndex] = getColorImageView(vk, rg, attachment.image, swapchainIndex);
                }

                if (passInfo.depthAttachment) {
                    views.back() = rg.images[passInfo.depthAttachment->image].depthView;
                }

                auto size = getFramebufferSize(vk, graph, passIndex);

                VkFramebufferCreateInfo createInfo {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass = rg.passes[passIndex].renderPass,
                    .attachmentCount = static_cast<uint32_t>(views.size()),
                    .pAttachments = views.data(),
                    .width = size.width,
                    .height = size.height,
                    .layers = 1,
                };

                vkCreateFramebuffer(vk.device, &createInfo, nullptr, &pass.framebuffer[swapchainIndex]);
            }
        }
    }

    static void createRenderPassBeginInfo(VkGlobals& vk, const RenderGraphBuilder& graph, RenderGraph& rg) {

        for(uint32_t passIndex = 0; passIndex < graph.passes.size(); passIndex++) {
            auto& pass = rg.passes[passIndex];
            auto& passInfo = graph.passes[passIndex];
            uint32_t attachmentCount = passInfo.colorAttachments.size() + (passInfo.depthAttachment.has_value() ? 1 : 0);

            for(uint32_t attachmentIndex = 0; attachmentIndex < passInfo.colorAttachments.size(); attachmentIndex++) {
                auto& attachment = passInfo.colorAttachments[attachmentIndex];
                if (attachment.clear.has_value()) {
                    pass.clearValues[attachmentIndex] = attachment.clear.value();
                }
            }

            if (passInfo.depthAttachment && passInfo.depthAttachment->clear.has_value()) {
                pass.clearValues[attachmentCount - 1] = passInfo.depthAttachment->clear.value();
            }

            for(uint32_t swapchainIndex = 0; swapchainIndex < vk.swapchain.image_count; swapchainIndex++) {
                pass.beginInfo[swapchainIndex] = VkRenderPassBeginInfo {
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .renderPass = pass.renderPass,
                    .framebuffer = pass.framebuffer[swapchainIndex],
                    .renderArea = {
                            .offset = {0, 0},
                            .extent = vk.swapchain.extent
                    },
                    .clearValueCount = attachmentCount,
                    .pClearValues = pass.clearValues,
                };
            }
        }

    }

    static void createPasses(VkGlobals& vk, const RenderGraphBuilder& graph, RenderGraph& rg) {
        rg.passes.resize(graph.passes.size());
        for(uint32_t passIndex = 0; passIndex < graph.passes.size(); passIndex++) {
            rg.passes[passIndex].callback = graph.passes[passIndex].callback;
        }
        createRenderpasses(vk, graph, rg);
        createFramebuffers(vk, graph, rg);
        createRenderPassBeginInfo(vk, graph, rg);
    }

    RenderGraph RenderGraphBuilder::build(VkGlobals& vk) const {
        RenderGraph rg;
        createImages(vk, *this, rg);
        createPasses(vk, *this, rg);
        return rg;
    }
}