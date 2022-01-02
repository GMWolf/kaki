//
// Created by felix on 29-12-21.
//

#include "gfx.h"


#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include "vk.h"
#include "window.h"
#include <cstdio>
#include <vector>
#include <span>
#include <fstream>
#include "shader.h"
#include <cereal/archives/binary.hpp>

template<class T>
std::vector<T> loadBytes(const char* path)
{
    std::vector<T> ret;
    FILE* file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    size_t numBytes = ftell(file);
    size_t numElements = numBytes / sizeof(T);
    fseek(file, 0, SEEK_SET);
    ret.resize(numElements);
    fread(ret.data(), sizeof(T), numElements, file);
    return ret;
}

VkShaderModule createModule(VkDevice device, std::span<uint32_t> code)
{
    VkShaderModuleCreateInfo createInfo {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size_bytes(),
            .pCode = code.data(),
    };

    VkShaderModule shader_module;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shader_module) != VK_SUCCESS){
        return VK_NULL_HANDLE;
    }

    return shader_module;
}

static VkRenderPass createRenderPass(VkDevice device, VkFormat format) {

    VkAttachmentDescription attachment {
        .flags = 0,
        .format = format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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


static VkPipelineLayout createPipelineLayout(VkDevice device)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    VkPipelineLayout layout;
    vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &layout);
    return layout;
}

static kaki::ShaderModule loadShaderModule(VkDevice device, const char* path) {
    std::ifstream is(path);
    cereal::BinaryInputArchive archive(is);

    kaki::ShaderModule module;
    archive(module);

    module.module = createModule(device, module.code);

    // we no longer need to keep the code around
    module.code.clear();
    module.code.shrink_to_fit();

    return module;
}


static VkPipeline createPipeline(VkDevice device, VkRenderPass renderPass, VkPipelineLayout pipelineLayout) {
    auto vertexModule = loadShaderModule(device, "shader.vert.shd");
    auto fragmentModule = loadShaderModule(device, "shader.frag.shd");

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo[] {
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertexModule.module,
                .pName = "main"
            },
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragmentModule.module,
                .pName = "main"
            }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = false};

    // A dummy viewport and scissor. this needs to get set by cmd buffer
    VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = 1.0f,
            .height = 1.0f,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
    };

    VkRect2D scissor{
            .offset = {0, 0},
            .extent = {1, 1},
    };

    VkPipelineViewportStateCreateInfo viewportState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = false,
            .rasterizerDiscardEnable = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable = false,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = false,
            .minSampleShading = 1.0f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = false,
            .alphaToOneEnable = false,
    };


    VkPipelineColorBlendAttachmentState blendAttachment {
            .blendEnable = true,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                              VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = false,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &blendAttachment,
    };

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_ALWAYS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
            .front = {},
            .back = {},
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
    };

    VkGraphicsPipelineCreateInfo pipelineInfo{
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStageCreateInfo,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = renderPass,
            .subpass = 0,
            .basePipelineHandle = {},
            .basePipelineIndex = -1,
    };


    VkPipeline pipeline;
    vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &pipeline);
    return pipeline;
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
            .set_minimum_version(1,0)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::integrated)
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
    vk.pipelineLayout = createPipelineLayout(vk.device);
    vk.pipeline = createPipeline(vk.device, vk.renderPass, vk.pipelineLayout);

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
        }
    };

    vkCmdBeginRenderPass(vk.cmd[vk.currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(vk.cmd[vk.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline);
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
    vkCmdDraw(vk.cmd[vk.currentFrame], 3, 1, 0, 0);
    vkCmdEndRenderPass(vk.cmd[vk.currentFrame]);

    vkEndCommandBuffer(vk.cmd[vk.currentFrame]);

    VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

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
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .swapchainCount = 1,
            .pSwapchains = &vk.swapchain.swapchain,
            .pImageIndices = &imageIndex,
            .pResults = &presentResult,
    };

    vkQueuePresentKHR(vk.queue, &presentInfo);
    vk.currentFrame = (vk.currentFrame + 1) % kaki::VkGlobals::framesInFlight;
}


kaki::gfx::gfx(flecs::world &world) {
    world.module<gfx>();

    createGlobals(world);

    world.system<VkGlobals>().kind(flecs::OnStore).each(render);

}
