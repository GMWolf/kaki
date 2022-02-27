//
// Created by felix on 29/01/2022.
//

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <glm/vec2.hpp>
#include "vk.h"
#include <functional>
#include <flecs.h>

namespace kaki {

    struct RenderGraph {
        constexpr static uint32_t maxSwapchainSize = 8;

        struct Image {
            VkImage image;
            VkImageView colorView;
            VkImageView depthView;
            VmaAllocation allocation;
        };

        struct Pass {
            VkRenderPass renderPass;
            VkFramebuffer framebuffer[maxSwapchainSize];
            VkRenderPassBeginInfo beginInfo[maxSwapchainSize];
            std::function<void(flecs::world&)> callback;
        };

        std::vector<Image> images;
        std::vector<Pass> passes;
    };

    constexpr static uint32_t DISPLAY_IMAGE_INDEX = UINT32_MAX;

    struct RenderGraphBuilder {

        struct Image {
            glm::uvec2 size;
            VkFormat format;
        };

        struct Pass {
            struct Attachment {
                uint32_t image;
                std::optional<VkClearValue> clear;
            };

            std::vector<Attachment> colorAttachments;
            std::optional<Attachment> depthAttachment;

            Pass& color(uint32_t image);
            Pass& colorClear(uint32_t image, const VkClearColorValue& clearValue);
            Pass& depth(uint32_t image);
            Pass& depthClear(uint32_t image, const VkClearDepthStencilValue& clearValue);

            std::function<void(flecs::world&)> callback;
        };

        std::vector<Pass> passes;
        std::vector<Image> images;

        uint32_t image(Image image);

        Pass& pass(std::function<void(flecs::world&)>&& callback);

        RenderGraph build(VkGlobals& vk) const;

    };


}