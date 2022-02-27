//
// Created by felix on 29/01/2022.
//

#include <cassert>
#include "render_graph.h"
#include "vk.h"

kaki::RenderGraphBuilder::Pass &kaki::RenderGraphBuilder::Pass::color(uint32_t image) {
    colorAttachments.push_back(Attachment{
            .image = image,
            .clear = {},
    });
    return *this;
}

kaki::RenderGraphBuilder::Pass &
kaki::RenderGraphBuilder::Pass::colorClear(uint32_t image, const VkClearColorValue &clearValue) {
    colorAttachments.push_back(Attachment{
            .image = image,
            .clear = VkClearValue {.color = clearValue},
    });
    return *this;
}

kaki::RenderGraphBuilder::Pass &kaki::RenderGraphBuilder::Pass::depth(uint32_t image) {
    assert(!depthAttachment.has_value());
    depthAttachment = Attachment{
        .image = image,
        .clear = {},
    };
    return *this;
}

kaki::RenderGraphBuilder::Pass &
kaki::RenderGraphBuilder::Pass::depthClear(uint32_t image, const VkClearDepthStencilValue &clearValue) {
    assert(!depthAttachment.has_value());
    depthAttachment = Attachment{
            .image = image,
            .clear = VkClearValue{.depthStencil = clearValue},
    };
    return *this;
}

uint32_t kaki::RenderGraphBuilder::image(kaki::RenderGraphBuilder::Image image) {
    images.push_back(image);
    return images.size() - 1;
}

kaki::RenderGraphBuilder::Pass &kaki::RenderGraphBuilder::pass(std::function<void(flecs::world&, VkCommandBuffer)>&& callback) {
    return passes.emplace_back(Pass{
        .callback = callback,
    });
}

void kaki::destroyGraph(kaki::VkGlobals& vk, kaki::RenderGraph &graph) {
    for(auto& pass : graph.passes) {
        for(uint32_t index = 0; index < vk.swapchain.image_count; index++) {
            vkDestroyFramebuffer(vk.device, pass.framebuffer[index], nullptr);
        }
        vkDestroyRenderPass(vk.device, pass.renderPass, nullptr);
    }
    graph.passes.clear();

    for(auto& image : graph.images) {
        vkDestroyImageView(vk.device, image.colorView, nullptr);
        vkDestroyImageView(vk.device, image.depthView, nullptr);
        vmaDestroyImage(vk.allocator, image.image, image.allocation);
    }

    graph.images.clear();
}

void kaki::runGraph(flecs::world &world, RenderGraph &graph, uint32_t swapchainIndex, VkCommandBuffer cmd) {

    auto& vk = *world.get_mut<VkGlobals>();

    for(auto& pass : graph.passes) {
        vkCmdBeginRenderPass(cmd, &pass.beginInfo[swapchainIndex], VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport {
                .x = 0,
                .y = 0,
                .width = (float)pass.beginInfo->renderArea.extent.width,
                .height = (float)pass.beginInfo->renderArea.extent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };

        VkRect2D scissor {
                .offset = {0, 0},
                .extent = pass.beginInfo->renderArea.extent
        };
        vkCmdSetViewport(vk.cmd[vk.currentFrame], 0, 1, &viewport);
        vkCmdSetScissor(vk.cmd[vk.currentFrame], 0, 1, &scissor);

        pass.callback(world, cmd);
        vkCmdEndRenderPass(cmd);
    }

}