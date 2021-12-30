//
// Created by felix on 30-12-21.
//

#ifndef KAKI_VK_H
#define KAKI_VK_H

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

namespace kaki {



    struct VkGlobals {
        VkInstance instance{};
        vkb::Device device{};
        vkb::Swapchain swapchain;
        VkQueue queue{};

        static const size_t framesInFlight = 2;
        uint32_t currentFrame = 0;
        VkSemaphore imageAvailableSemaphore[framesInFlight];
        VkSemaphore renderCompleteSemaphores[framesInFlight];

        VkFence cmdBufFence[framesInFlight];
        VkCommandPool cmdPool;
        VkCommandBuffer cmd[framesInFlight];

    };

}

#endif //KAKI_VK_H
