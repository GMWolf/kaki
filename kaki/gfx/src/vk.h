//
// Created by felix on 30-12-21.
//

#ifndef KAKI_VK_H
#define KAKI_VK_H

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "render_graph.h"
#include "geometry_buffer.h"
#include <kaki/transform.h>

namespace kaki {

    struct DrawInfo {
        Transform transform;

        uint32_t indexOffset;
        uint32_t vertexOffset;
        uint32_t material;
        uint32_t pad;
    };

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

        GeometryBuffers geometry;

        uint64_t maxDrawCount = 2048;
        VkBuffer drawInfoBuffer[framesInFlight];
        VmaAllocation drawInfoAlloc[framesInFlight];
        DrawInfo* drawInfos[framesInFlight];

        GraphScript script;
        RenderGraph graph;
    };

}

#endif //KAKI_VK_H
