//
// Created by felix on 29/01/2022.
//

#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>
#include <glm/vec2.hpp>

namespace kaki {

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
        };

        std::vector<Pass> passes;
        std::vector<Image> images;

        uint32_t image(Image image) {
            images.push_back(image);
            return images.size() - 1;
        }

        Pass& pass(){
            return passes.emplace_back();
        }

    };

}