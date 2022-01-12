//
// Created by felix on 29-12-21.
//

#include "gfx.h"

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include "vk.h"
#include "window.h"
#include <cstdio>
#include "pipeline.h"
#include "shader.h"
#include "asset.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rapidjson/document.h>
#include <fstream>


static VkRenderPass createRenderPass(VkDevice device, VkFormat format) {
    VkAttachmentDescription attachment {
        .flags = 0,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference attachmentReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentReference,
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkRenderPass renderPass;
    vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    return renderPass;
}

VkFramebuffer createFrameBuffer(VkDevice device, VkRenderPass pass, VkImageView image, uint32_t width, uint32_t height) {

    VkFramebufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pass,
        .attachmentCount = 1,
        .pAttachments = &image,
        .width = width,
        .height = height,
        .layers = 1,
    };

    VkFramebuffer framebuffer;
    vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer);
    return framebuffer;
}


static bool createGlobals(flecs::world& world) {
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("Kaki application")
            .request_validation_layers()
            .use_default_debug_messenger()
            .build();

    if (!inst_ret) {
        fprintf(stderr, "Failed to initialize vulkan instance.\n");
        return false;
    }

    vkb::Instance vkb_inst = inst_ret.value();

    VkSurfaceKHR surface = world.get<kaki::Window>()->createSurface(vkb_inst);

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto phys_ret = selector.set_surface(surface)
            .set_minimum_version(1,2)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select();

    if (!phys_ret) {
        fprintf(stderr, "Failed to select physical device.\n");
        return false;
    }

    vkb::DeviceBuilder deviceBuilder{phys_ret.value()};

    auto dev_ret = deviceBuilder.build();

    if (!dev_ret) {
        fprintf(stderr, "Failed to crate device.\n");
        return false;
    }

    vkb::Device vkbDevice = dev_ret.value();

    auto graphics_queue_ret = vkbDevice.get_queue(vkb::QueueType::graphics);

    if(!graphics_queue_ret) {
        fprintf(stderr, "Failed to get graphics queue.\n");
        return false;
    }

    VkQueue queue = graphics_queue_ret.value();

    vkb::SwapchainBuilder swapchainBuilder { vkbDevice };
    auto swap_ret = swapchainBuilder
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .build();

    if (!swap_ret) {
        fprintf(stderr, "Failed to create swapchain.\n");
        return false;
    }

    kaki::VkGlobals vk;
    vk.swapchain = swap_ret.value();
    vk.instance = vkb_inst.instance;
    vk.device = vkbDevice;
    vk.queue = queue;

    VkSemaphoreCreateInfo semaphoreCreateInfo {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0
    };

    for(int i = 0; i < kaki::VkGlobals::framesInFlight; i++) {
        vkCreateSemaphore(vk.device, &semaphoreCreateInfo, nullptr, &vk.imageAvailableSemaphore[i]);
        vkCreateSemaphore(vk.device, &semaphoreCreateInfo, nullptr, &vk.renderCompleteSemaphores[i]);
    }

    VkFenceCreateInfo fenceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for(int i = 0; i < kaki::VkGlobals::framesInFlight; i++) {
        vkCreateFence(vk.device, &fenceCreateInfo, nullptr, &vk.cmdBufFence[i]);
    }

    VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = vk.device.get_queue_index(vkb::QueueType::graphics).value(),
    };

    vkCreateCommandPool(vk.device, &poolInfo, nullptr, &vk.cmdPool);

    VkCommandBufferAllocateInfo cmdAllocInfo {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = vk.cmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(kaki::VkGlobals::framesInFlight),
    };

    vkAllocateCommandBuffers(vk.device, &cmdAllocInfo, vk.cmd);

    vk.renderPass = createRenderPass(vk.device, vk.swapchain.image_format);

    for(int i = 0; i < vk.swapchain.image_count; i++) {
        vk.framebuffer[i] = createFrameBuffer(vk.device, vk.renderPass, vk.swapchain.get_image_views()->at(i), vk.swapchain.extent.width, vk.swapchain.extent.height);
    }

    world.set<kaki::VkGlobals>(vk);

    return true;
}


static void render(const flecs::entity& entity, kaki::VkGlobals& vk) {
    vkWaitForFences(vk.device, 1, &vk.cmdBufFence[vk.currentFrame], true, UINT64_MAX);

    uint32_t imageIndex;
    auto acquire = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore[vk.currentFrame], nullptr, &imageIndex);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR ){

    } else if (acquire != VK_SUCCESS) {
        fprintf(stderr, "Error acquiring swapchain image.\n");
    }

    static float time = 0;
    time += entity.world().delta_time();

    float clearColorV = time / (10.0f);

    VkClearColorValue clearColor = { clearColorV, clearColorV, clearColorV, 0.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;

    vkResetCommandBuffer(vk.cmd[vk.currentFrame], 0);
    vkBeginCommandBuffer(vk.cmd[vk.currentFrame], &beginInfo);

    //vkCmdClearColorImage(vk.cmd[vk.currentFrame], vk.swapchain.get_images()->at(imageIndex), VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &imageRange);
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk.renderPass,
        .framebuffer = vk.framebuffer[imageIndex],
        .renderArea = {
                .offset = {
                        .x = 0,
                        .y = 0,
                },
                .extent = {
                        .width = vk.swapchain.extent.width,
                        .height = vk.swapchain.extent.height,
                }
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue,
    };

    vkCmdBeginRenderPass(vk.cmd[vk.currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto pipeline = entity.world().lookup("pipeline").get<kaki::Pipeline>();

    vkCmdBindPipeline(vk.cmd[vk.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    VkViewport viewport {
        .x = 0,
        .y = 0,
        .width = (float)vk.swapchain.extent.width,
        .height = (float)vk.swapchain.extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor {
            .offset = {0, 0},
            .extent = vk.swapchain.extent
    };
    vkCmdSetViewport(vk.cmd[vk.currentFrame], 0, 1, &viewport);
    vkCmdSetScissor(vk.cmd[vk.currentFrame], 0, 1, &scissor);

    entity.world().each([&](const kaki::Camera& camera) {
        glm::mat4 proj = glm::ortho(camera.x, camera.x + camera.width, camera.y, camera.y + camera.height);
        vkCmdPushConstants(vk.cmd[vk.currentFrame], pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(proj), &proj);

        entity.world().each([&](const kaki::Rectangle& rect) {
            vkCmdPushConstants(vk.cmd[vk.currentFrame], pipeline->pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(proj), sizeof(rect.pos), &rect.pos);
            vkCmdPushConstants(vk.cmd[vk.currentFrame], pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(proj) + sizeof(rect.pos) + sizeof(glm::vec2), sizeof(rect.color), &rect.color);
            vkCmdDraw(vk.cmd[vk.currentFrame], 6, 1, 0, 0);
        });
    });

    vkCmdEndRenderPass(vk.cmd[vk.currentFrame]);

    vkEndCommandBuffer(vk.cmd[vk.currentFrame]);

    VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    vkResetFences(vk.device, 1, &vk.cmdBufFence[vk.currentFrame]);

    VkSubmitInfo submitInfo {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &vk.imageAvailableSemaphore[vk.currentFrame],
            .pWaitDstStageMask = waitStages,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk.cmd[vk.currentFrame],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &vk.renderCompleteSemaphores[vk.currentFrame],
    };

    vkQueueSubmit(vk.queue, 1, &submitInfo, vk.cmdBufFence[vk.currentFrame]);


    VkResult presentResult;
    VkPresentInfoKHR presentInfo {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &vk.renderCompleteSemaphores[vk.currentFrame],
            .swapchainCount = 1,
            .pSwapchains = &vk.swapchain.swapchain,
            .pImageIndices = &imageIndex,
            .pResults = &presentResult,
    };

    vkQueuePresentKHR(vk.queue, &presentInfo);
    vk.currentFrame = (vk.currentFrame + 1) % kaki::VkGlobals::framesInFlight;
}


void handleShaderModuleLoads(flecs::iter iter, kaki::Asset* assets, kaki::asset::Shader*) {
    for(auto i : iter) {
        iter.entity(i).set<kaki::ShaderModule>(kaki::loadShaderModule(iter.world().get<kaki::VkGlobals>()->device, assets[i].path));
    }
}

void handlePipelineLoads(flecs::iter iter, kaki::Asset* assets, kaki::asset::Pipeline*) {
    auto vk = iter.world().get<kaki::VkGlobals>();

    for(auto i : iter) {

        rapidjson::Document doc;
        std::ifstream t(assets->path);
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        doc.ParseInsitu(str.data());

        auto vertexModule = iter.world().lookup(doc["vertex"].GetString()).get<kaki::ShaderModule>();
        auto fragmentModule = iter.world().lookup(doc["fragment"].GetString()).get<kaki::ShaderModule>();

        iter.entity(i).set<kaki::Pipeline>(kaki::createPipeline(vk->device, vk->renderPass, vertexModule, fragmentModule));
    }
}


kaki::gfx::gfx(flecs::world &world) {
    world.module<gfx>();

    world.observer<kaki::Asset, kaki::asset::Shader>().event(flecs::OnAdd).iter(handleShaderModuleLoads);

    world.observer<kaki::Asset, kaki::asset::Pipeline>().event(flecs::OnAdd).iter(handlePipelineLoads);

    createGlobals(world);

    world.system<VkGlobals>().kind(flecs::OnStore).each(render);

}
