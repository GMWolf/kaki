//
// Created by felix on 30-12-21.
//

#ifndef KAKI_VK_H
#define KAKI_VK_H

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include "pipeline.h"
#include <vk_mem_alloc.h>
#include "render_graph.h"

namespace kaki {

    struct VkGlobals {
        VkInstance instance{};
        vkb::Device device{};
        vkb::Swapchain swapchain;
        std::vector<VkImageView> imageViews;

        VkQueue queue{};

        VmaAllocator allocator;

        static const size_t framesInFlight = 2;
        uint32_t currentFrame = 0;
        VkSemaphore imageAvailableSemaphore[framesInFlight] {};
        VkSemaphore renderCompleteSemaphores[framesInFlight] {};

        VkFence cmdBufFence[framesInFlight] {};
        VkCommandPool cmdPool {};
        VkCommandBuffer cmd[framesInFlight] {};

        VkDescriptorPool descriptorPools[framesInFlight];
        VkDescriptorPool imguiDescPool;
        VkSampler sampler;

        GraphScript script;
        RenderGraph graph;
    };

}

#endif //KAKI_VK_H
