#include <cstdio>

#include <flecs.h>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

static void glfwErrorCallback(int error, const char *description) {
    fprintf(stderr, "GLFW error: %s\n", description);
}

int main() {

    flecs::world world;

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize glfw.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "window", nullptr, nullptr);

    if (!window) {
        fprintf(stderr, "Failed to create glfw window.\n");
        return 1;
    }

    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Kaki application")
            .request_validation_layers()
            .use_default_debug_messenger()
            .build();

    if (!inst_ret) {
        fprintf(stderr, "Failed to initialize vulkan instance.\n");
        return 1;
    }

    vkb::Instance vkb_inst = inst_ret.value();

    VkSurfaceKHR surface{};
    glfwCreateWindowSurface(vkb_inst.instance, window, nullptr, &surface);

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto phys_ret = selector.set_surface(surface)
            .set_minimum_version(1,0)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::integrated)
            .select();

    if (!phys_ret) {
        fprintf(stderr, "Failed to select physical device.\n");
        return 1;
    }

    vkb::DeviceBuilder deviceBuilder{phys_ret.value()};

    auto dev_ret = deviceBuilder.build();

    if (!dev_ret) {
        fprintf(stderr, "Failed to crate device.\n");
        return 1;
    }

    vkb::Device vkbDevice = dev_ret.value();

    VkDevice device = vkbDevice.device;


    auto graphics_queue_ret = vkbDevice.get_queue(vkb::QueueType::graphics);

    if(!graphics_queue_ret) {
        fprintf(stderr, "Failed to get graphics queue.\n");
        return -1;
    }

    VkQueue queue = graphics_queue_ret.value();


    vkb::SwapchainBuilder swapchainBuilder { vkbDevice };
    auto swap_ret = swapchainBuilder.build();

    if (!swap_ret) {
        fprintf(stderr, "Failed to create swapchain.\n");
        return 1;
    }

    VkSwapchainKHR swapchain = swap_ret->swapchain;

    double lastFrameTime = glfwGetTime();

    uint32_t currentFrame = 0;
    VkSemaphore imageAvailableSemaphore[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderCompleteSemaphores[MAX_FRAMES_IN_FLIGHT];

    VkSemaphoreCreateInfo semaphoreCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphores[i]);
    }

    VkCommandPool commandPool;
    VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value(),
    };

    vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo cmdAllocInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = swap_ret->image_count,//static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    };



    VkCommandBuffer cmd[16];
    vkAllocateCommandBuffers(device, &cmdAllocInfo, cmd);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkClearColorValue clearColor = { 164.0f/256.0f, 30.0f/256.0f, 34.0f/256.0f, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;

    for(int i = 0; i < swap_ret->image_count; i++) {
        vkBeginCommandBuffer(cmd[i], &beginInfo);
        vkCmdClearColorImage(cmd[i], swap_ret->get_images()->at(i), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);
        vkEndCommandBuffer(cmd[i]);
    }


    while(!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        world.progress(static_cast<float>(deltaTime));

        uint32_t imageIndex;
        auto acquire = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore[currentFrame], nullptr, &imageIndex);

        if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR ){

        } else if (acquire != VK_SUCCESS) {
            fprintf(stderr, "Error acquiring swapchain image.\n");
        }


        // clear
        VkPipelineStageFlags waitStages[] = {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &imageAvailableSemaphore[currentFrame],
                .pWaitDstStageMask = waitStages,
                .commandBufferCount = 1,
                .pCommandBuffers = &cmd[imageIndex],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores = &renderCompleteSemaphores[currentFrame],
        };

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

        // present


        VkResult presentResult;

        VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &imageIndex,
            .pResults = &presentResult,
        };


        vkQueuePresentKHR(queue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
