//
// Created by felix on 29/01/2022.
//

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <glm/vec2.hpp>
#include <functional>
#include <flecs.h>
#include <vk_mem_alloc.h>
#include <span>
#include <kaki/ecs.h>

namespace kaki {

    struct VkGlobals;

    struct RenderGraph {
        constexpr static uint32_t maxSwapchainSize = 8;
        constexpr static uint32_t maxAttachmentCount = 8;

        struct Image {
            VkImage image;
            VkImageView colorView;
            VkImageView depthView;
            VmaAllocation allocation;
        };

        struct Pass {
            VkRenderPass renderPass;
            VkFramebuffer framebuffer[maxSwapchainSize];
            std::vector<VkClearValue> clearValues;
            VkRenderPassBeginInfo beginInfo[maxSwapchainSize];
            std::vector<VkImageView> images;

            std::function<void(kaki::ecs::Registry&, VkCommandBuffer, std::span<VkImageView> images)> callback;
        };

        std::vector<Image> images;
        std::vector<Pass> passes;
    };

    constexpr static uint32_t DISPLAY_IMAGE_INDEX = UINT32_MAX;

    struct RenderGraphBuilder {

        struct Image {
            VkExtent2D size;
            VkFormat format;
        };

        struct Pass {
            struct Attachment {
                uint32_t image;
                std::optional<VkClearValue> clear;
            };

            std::vector<Attachment> colorAttachments;
            std::optional<Attachment> depthAttachment;
            std::vector<uint32_t> imageReads;

            Pass& color(uint32_t image);
            Pass& colorClear(uint32_t image, const VkClearColorValue& clearValue);
            Pass& depth(uint32_t image);
            Pass& depthClear(uint32_t image, const VkClearDepthStencilValue& clearValue);
            Pass& imageRead(uint32_t image);

            std::function<void(ecs::Registry&, VkCommandBuffer, std::span<VkImageView> images)> callback;
        };

        std::vector<Pass> passes;
        std::vector<Image> images;

        uint32_t image(Image image);

        Pass& pass(std::function<void(ecs::Registry&, VkCommandBuffer, std::span<VkImageView> images)>&& callback);

        RenderGraph build(VkGlobals& vk) const;
    };

    void destroyGraph(kaki::VkGlobals& vk, RenderGraph& graph);

    void runGraph(kaki::ecs::Registry& registry, kaki::ecs::EntityId renderer, RenderGraph& graph, uint32_t swapchainIndex, VkCommandBuffer cmd);

    using GraphScript = RenderGraph(*)(VkGlobals& vk);
}