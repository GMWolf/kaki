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
#include <kaki/asset.h>
#include "image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <span>
#include <algorithm>
#include <numeric>
#include "gltf.h"
#include <kaki/transform.h>
#include <membuf.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "imgui_theme.h"

namespace kaki::internal {
    struct MeshInstance {
        uint32_t indexOffset;
        uint32_t vertexOffset;
        uint32_t indexCount;
    };
}


flecs::query<kaki::MeshFilter> meshQuery;


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

    VkAttachmentDescription depthAttachment {
            .flags = 0,
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription attachments[] = {
            attachment, depthAttachment
    };

    VkAttachmentReference attachmentReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentReference {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentReference,
        .pDepthStencilAttachment = &depthAttachmentReference,
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .flags = 0,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkRenderPass renderPass;
    vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
    return renderPass;
}

VkFramebuffer createFrameBuffer(VkDevice device, VkRenderPass pass, VkImageView image, VkImageView depth, uint32_t width, uint32_t height) {

    VkImageView views[] { image, depth };

    VkFramebufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = pass,
        .attachmentCount = static_cast<uint32_t>((depth == VK_NULL_HANDLE) ? 1 : 2),
        .pAttachments = views,
        .width = width,
        .height = height,
        .layers = 1,
    };

    VkFramebuffer framebuffer;
    vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer);
    return framebuffer;
}


static bool createSwapChain(kaki::VkGlobals& vk) {
    vkb::SwapchainBuilder swapchainBuilder { vk.device };
    auto swap_ret = swapchainBuilder
            .set_old_swapchain(vk.swapchain)
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            .build();

    if (!swap_ret) {
        fprintf(stderr, "Failed to create swapchain.\n");
        return false;
    }

    vkb::destroy_swapchain(vk.swapchain);
    vk.swapchain = swap_ret.value();
    vk.imageViews = vk.swapchain.get_image_views().value();

    return true;
}

static void createFrameBuffer(kaki::VkGlobals& vk) {
    for(int i = 0; i < vk.swapchain.image_count; i++) {
        vk.framebuffer[i] = createFrameBuffer(vk.device, vk.renderPass, vk.imageViews[i], vk.depthBufferView, vk.swapchain.extent.width, vk.swapchain.extent.height);
        vk.imguiFrameBuffer[i] = createFrameBuffer(vk.device, vk.imguiRenderPass, vk.imageViews[i], VK_NULL_HANDLE, vk.swapchain.extent.width, vk.swapchain.extent.height);
    }
}

static void createDepthBuffer(kaki::VkGlobals& vk) {
    {
        uint32_t queueIndex = vk.device.get_queue_index(vkb::QueueType::graphics).value();

        VkImageCreateInfo imageCreateInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_D32_SFLOAT,
                .extent = {
                        .width = vk.swapchain.extent.width,
                        .height = vk.swapchain.extent.height,
                        .depth = 1,
                },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples =VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = &queueIndex,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VmaAllocationCreateInfo allocInfo {
                .flags = 0,
                .usage = VMA_MEMORY_USAGE_GPU_ONLY,
                .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .preferredFlags = 0,
        };

        vmaCreateImage(vk.allocator, &imageCreateInfo, &allocInfo, &vk.depthBuffer, &vk.depthBufferAlloc, nullptr);

        VkImageSubresourceRange imageRange {
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
        };

        VkImageViewCreateInfo viewInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = vk.depthBuffer,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_D32_SFLOAT,
                .components = VkComponentMapping {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = imageRange,
        };

        VkImageView view;
        vkCreateImageView(vk.device, &viewInfo, nullptr, &vk.depthBufferView);

    }
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


    auto window = world.lookup("window").get<kaki::Window>();
    VkSurfaceKHR surface = window->createSurface(vkb_inst);

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

    VkPhysicalDeviceRobustness2FeaturesEXT rbfeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
        .nullDescriptor = VK_TRUE,
    };

    deviceBuilder.add_pNext(&rbfeatures);

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

    kaki::VkGlobals vk {};
    vk.instance = vkb_inst.instance;
    vk.device = vkbDevice;
    vk.queue = queue;

    if (!createSwapChain(vk)) {
        return false;
    }


    VmaAllocatorCreateInfo allocatorInfo = {
            .physicalDevice = vk.device.physical_device,
            .device = vk.device,
            .instance = vk.instance
    };
    vmaCreateAllocator(&allocatorInfo, &vk.allocator);


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

    for(auto & i : vk.cmdBufFence) {
        vkCreateFence(vk.device, &fenceCreateInfo, nullptr, &i);
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

    createDepthBuffer(vk);

    // Create the imgui Render Pass
    {
        VkAttachmentDescription attachment = {};
        attachment.format = vk.swapchain.image_format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference color_attachment = {};
        color_attachment.attachment = 0;
        color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment;
        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &attachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;
        VkResult err = vkCreateRenderPass(vk.device, &info, nullptr, &vk.imguiRenderPass);
    }


    createFrameBuffer(vk);

    {
        VkDescriptorPoolSize descriptorPoolSize[]{
                VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 1024,
                }
        };

        VkDescriptorPoolCreateInfo descPoolInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1024,
                .poolSizeCount = 1,
                .pPoolSizes = descriptorPoolSize,
        };

        for(auto & descriptorPool : vk.descriptorPools) {
            vkCreateDescriptorPool(vk.device, &descPoolInfo, nullptr, &descriptorPool);
        }
    }

    {
        VkSamplerCreateInfo samplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .minLod = 0,
            .maxLod = VK_LOD_CLAMP_NONE,
        };

        vkCreateSampler(vk.device, &samplerCreateInfo, nullptr, &vk.sampler);
    }


    {
        VkDescriptorPoolSize pool_sizes[] =
                {
                        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
                };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        vkCreateDescriptorPool(vk.device, &pool_info, nullptr, &vk.imguiDescPool);
    }


    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImguiTheme();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->handle, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vk.instance;
    init_info.PhysicalDevice = vk.device.physical_device;
    init_info.Device = vk.device.device;
    init_info.QueueFamily = vk.device.get_queue_index(vkb::QueueType::graphics).value();
    init_info.Queue = vk.queue;
    init_info.PipelineCache = nullptr;
    init_info.DescriptorPool = vk.imguiDescPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = vk.swapchain.image_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;

    ImGui_ImplVulkan_Init(&init_info, vk.imguiRenderPass);

    {
        // Use any command queue
        VkCommandBuffer command_buffer = vk.cmd[0];

        vkResetCommandBuffer(command_buffer, 0);

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &command_buffer;
        vkEndCommandBuffer(command_buffer);

        vkQueueSubmit(vk.queue, 1, &end_info, VK_NULL_HANDLE);

        vkDeviceWaitIdle(vk.device);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    world.set<kaki::VkGlobals>(vk);
    return true;
}


struct ShaderInput {
    const char* name;
    VkImageView imageView;
};

void fillDescSetWrites(kaki::VkGlobals& vk, VkDescriptorSet vkSet, kaki::DescriptorSet& descSetInfo, std::span<ShaderInput> shaderInputs, std::span<VkWriteDescriptorSet> writes, std::span<VkDescriptorImageInfo> imageInfos ) {

    for(int i = 0; i < descSetInfo.bindings.size(); i++) {
        const auto& name = descSetInfo.bindingNames[i];
        auto input = std::ranges::find_if(shaderInputs, [name](ShaderInput& input) { return input.name == name; });
        assert(input != shaderInputs.end());

        imageInfos[i] = {
                .sampler = vk.sampler,
                .imageView = input->imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        writes[i] = VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = vkSet,
                .dstBinding = descSetInfo.bindings[i].binding,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = descSetInfo.bindings[i].descriptorType,
                .pImageInfo = &imageInfos[i],
        };
    }
}


void updateDescSets( kaki::VkGlobals& vk, std::span<VkDescriptorSet> descSets, const kaki::Pipeline& pipeline, std::span<ShaderInput> shaderInputs) {

    size_t writeCount = std::accumulate(pipeline.descriptorSets.begin(), pipeline.descriptorSets.end(), 0, [](size_t a, const kaki::DescriptorSet& set) {
        return a + set.bindings.size();
    });;

    std::vector<VkWriteDescriptorSet> descSetWrites(writeCount);
    std::vector<VkDescriptorImageInfo> imageInfos(writeCount);

    std::span<VkWriteDescriptorSet> descSetWriteRange = descSetWrites;
    std::span<VkDescriptorImageInfo> imageInfoRange = imageInfos;

    for(int i = 0; i < pipeline.descriptorSets.size(); i++)
    {
        auto vkSet = descSets[i];
        auto set = pipeline.descriptorSets[i];

        fillDescSetWrites(vk, vkSet, set, shaderInputs, descSetWriteRange, imageInfoRange);
        descSetWriteRange = descSetWriteRange.subspan(set.bindings.size(), descSetWriteRange.size() - set.bindings.size());
        imageInfoRange = imageInfoRange.subspan(set.bindings.size(), imageInfoRange.size() - set.bindings.size());
    }

    vkUpdateDescriptorSets(vk.device, writeCount, descSetWrites.data(), 0, nullptr);
}

template<class T>
std::optional<flecs::column<T>> selectColumn(flecs::iter iter, std::initializer_list<size_t> indices) {
    for(auto i : indices) {
        if (iter.is_set(i)) {
            return iter.term<T>(i);
        }
    }
    return std::nullopt;
}

static glm::mat4 perspective(float vertical_fov, float aspect_ratio, float n, float f, glm::mat4 *inverse = nullptr)
{
    float fov_rad = vertical_fov;
    float focal_length = 1.0f / std::tan(fov_rad / 2.0f);

    float x  =  focal_length / aspect_ratio;
    float y  = -focal_length;
    float A  = n / (f - n);
    float B  = f * A;

    glm::mat4 projection({
                                x,    0.0f,  0.0f, 0.0f,
                                0.0f,    y,  0.0f, 0.0f,
                                0.0f, 0.0f,     A,    B,
                                0.0f, 0.0f, 1.0f, 0.0f,
                        });

    if (inverse)
    {
        *inverse = glm::mat4({
                                    1/x,  0.0f, 0.0f,  0.0f,
                                    0.0f,  1/y, 0.0f,  0.0f,
                                    0.0f, 0.0f, 0.0f, -1.0f,
                                    0.0f, 0.0f,  1/B,   A/B,
                            });
    }

    return glm::transpose( projection );
}



void recreate_swapchain(kaki::VkGlobals& vk) {

    vkDeviceWaitIdle(vk.device);

    for(auto framebuffer : std::span(vk.framebuffer, vk.swapchain.image_count)) {
        vkDestroyFramebuffer(vk.device, framebuffer, nullptr);
    }

    for(auto framebuffer : std::span(vk.imguiFrameBuffer, vk.swapchain.image_count)) {
        vkDestroyFramebuffer(vk.device, framebuffer, nullptr);
    }


    vk.swapchain.destroy_image_views(vk.imageViews);

    vkDestroyImageView(vk.device, vk.depthBufferView, nullptr);
    vmaDestroyImage(vk.allocator, vk.depthBuffer, vk.depthBufferAlloc);


    createSwapChain(vk);
    createDepthBuffer(vk);
    createFrameBuffer(vk);
}


static void render(const flecs::entity& entity, kaki::VkGlobals& vk) {

    auto world = entity.world();

    vkWaitForFences(vk.device, 1, &vk.cmdBufFence[vk.currentFrame], true, UINT64_MAX);

    vkResetDescriptorPool(vk.device, vk.descriptorPools[vk.currentFrame], 0);
    vkResetCommandBuffer(vk.cmd[vk.currentFrame], 0);


    uint32_t imageIndex;
    auto acquire = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore[vk.currentFrame], nullptr, &imageIndex);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR ){
        recreate_swapchain (vk);
        return;
    } else if (acquire != VK_SUCCESS) {
        fprintf(stderr, "Error acquiring swapchain image.\n");
        return;
    }

    static float time = 0;
    time += entity.world().delta_time();

    VkClearColorValue clearColor = { 0, 0, 0, 1.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkClearDepthStencilValue clearDepth {0.0f, 0};
    VkClearValue depthClearValue {
        .depthStencil = clearDepth,
    };

    VkClearValue clearValues[] {clearValue, depthClearValue};

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;

    vkBeginCommandBuffer(vk.cmd[vk.currentFrame], &beginInfo);

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
        .clearValueCount = 2,
        .pClearValues = clearValues,
    };

    vkCmdBeginRenderPass(vk.cmd[vk.currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto pipeline = entity.world().lookup("testpackage::pipeline").get<kaki::Pipeline>();

    std::vector<VkDescriptorSetLayout> descSetLayouts;
    for(auto& descSet : pipeline->descriptorSets) {
        descSetLayouts.push_back(descSet.layout);
    }

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

    //auto cameraFilter = world.filter_builder<const kaki::Camera, const kaki::Transform>()
    //        .arg(2).id<kaki::Transform, kaki::WorldSpace>()
    //        .build();

    world.each([&](const kaki::Camera& camera, const kaki::Transform& cameraTransform) {

        float aspect = (float)vk.swapchain.extent.width / (float)vk.swapchain.extent.height;
        glm::mat4 proj = perspective(camera.fov, aspect, 0.01f, 100.0f);
        glm::mat4 view = cameraTransform.inverse().matrix();
        glm::mat4 viewProj = proj * view;

        meshQuery.iter([&](flecs::iter& it, kaki::MeshFilter* filters) {

            auto transforms = it.term<kaki::Transform>(3);

            auto meshInstances = it.term<kaki::internal::MeshInstance>(2);
            auto gltfE = it.pair(2).object();
            auto gltf = gltfE.get<kaki::Gltf>();

            VkDeviceSize offsets[4]{0, 0, 0, 0};
            vkCmdBindVertexBuffers(vk.cmd[vk.currentFrame], 0, 4, gltf->buffers, offsets);
            vkCmdBindIndexBuffer(vk.cmd[vk.currentFrame], gltf->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            for(auto i : it) {
                kaki::internal::MeshInstance& mesh = meshInstances[i];
                auto albedoImage = flecs::entity(world, filters[i].albedo).get<kaki::Image>();
                auto normalImage = flecs::entity(world, filters[i].normal).get<kaki::Image>();
                auto metallicRoughnessImage = flecs::entity(world, filters[i].metallicRoughness).get<kaki::Image>();
                auto aoImage = flecs::entity(world, filters[i].ao).get<kaki::Image>();
                auto emissiveImage = flecs::entity(world, filters[i].emissive).get<kaki::Image>();
                auto& transform = transforms[i];

                if (!descSetLayouts.empty()) {
                    VkDescriptorSetAllocateInfo descAlloc{
                            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                            .pNext = nullptr,
                            .descriptorPool = vk.descriptorPools[vk.currentFrame],
                            .descriptorSetCount = static_cast<uint32_t>(descSetLayouts.size()),
                            .pSetLayouts = descSetLayouts.data(),
                    };

                    std::vector<VkDescriptorSet> descriptorSets(descSetLayouts.size());
                    vkAllocateDescriptorSets(vk.device, &descAlloc, descriptorSets.data());

                    ShaderInput inputs[]{
                        {"albedoTexture", albedoImage->view},
                        {"normalTexture", normalImage ? normalImage->view : VK_NULL_HANDLE},
                        {"metallicRoughnessTexture", metallicRoughnessImage ? metallicRoughnessImage->view : VK_NULL_HANDLE},
                        {"aoTexture", aoImage ? aoImage->view : VK_NULL_HANDLE},
                        {"emissiveTexture", emissiveImage? emissiveImage->view : VK_NULL_HANDLE},
                    };

                    updateDescSets(vk, descriptorSets, *pipeline, inputs);

                    vkCmdBindDescriptorSets(vk.cmd[vk.currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            pipeline->pipelineLayout,
                                            0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
                }

                struct Pc {
                    glm::mat4 proj;
                    glm::vec3 viewPos; float pad0;
                    kaki::Transform transform;
                    glm::vec3 light; float pad1;
                } pc;

                pc.proj = viewProj;
                pc.viewPos = cameraTransform.position;
                pc.transform = transform;
                pc.light = -glm::normalize(glm::vec3(0, -1, 0.5));

                vkCmdPushConstants(vk.cmd[vk.currentFrame], pipeline->pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(Pc), &pc);


                vkCmdDrawIndexed(vk.cmd[vk.currentFrame], mesh.indexCount, 1, mesh.indexOffset, mesh.vertexOffset, 0);

            }

        });
    });

    vkCmdEndRenderPass(vk.cmd[vk.currentFrame]);

    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("hi");

        ImGui::End();

        ImGui::Begin("hello");

        ImGui::End();

        ImGui::Render();

        VkRenderPassBeginInfo renderPassBeginInfo2{
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = vk.imguiRenderPass,
                .framebuffer = vk.imguiFrameBuffer[imageIndex],
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
                .pClearValues = clearValues,
        };

        vkCmdBeginRenderPass(vk.cmd[vk.currentFrame], &renderPassBeginInfo2, VK_SUBPASS_CONTENTS_INLINE);


        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk.cmd[vk.currentFrame]);

        vkCmdEndRenderPass(vk.cmd[vk.currentFrame]);
    }
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



static void UpdateMeshInstance(flecs::iter iter, kaki::MeshFilter* meshFilters) {
    for(auto i : iter) {

        auto mesh =  flecs::entity(iter.world(), meshFilters[i].mesh).get<kaki::Mesh>();
        auto gltf = flecs::entity(iter.world(), meshFilters[i].mesh).get_object(flecs::ChildOf);

        iter.entity(i).set<kaki::internal::MeshInstance>(gltf, kaki::internal::MeshInstance {
            .indexOffset = mesh->primitives[meshFilters[i].primitiveIndex].indexOffset,
            .vertexOffset = mesh->primitives[meshFilters[i].primitiveIndex].vertexOffset,
            .indexCount = mesh->primitives[meshFilters[i].primitiveIndex].indexCount,
        } );
    }
}


kaki::gfx::gfx(flecs::world &world) {
    auto This = world.module<gfx>();
    flecs::doc::set_brief(This, "Rendering module for kaki");

    world.component<kaki::Mesh>();
    world.component<kaki::Image>();
    world.component<kaki::Pipeline>();
    world.component<kaki::Gltf>();
    world.component<kaki::ShaderModule>();
    world.component<kaki::MeshFilter>();

    world.component<kaki::Camera>()
            .member<float>("yfov");

    world.component<kaki::DependsOn>().add(flecs::Acyclic);

    world.entity("GltfLoader").set<ComponentAssetHandler, Gltf>({
        .load = loadGltfs,
    });

    auto shaderModuleLoader = world.entity("ShaderLoader").set<ComponentAssetHandler, ShaderModule>({
        .load = [](flecs::iter iter, AssetData* data, void* pmodule){
            auto* modules = static_cast<ShaderModule*>(pmodule);
            auto device = iter.world().get<VkGlobals>()->device.device;

            for(auto i : iter) {
                membuf buf(data[i].data);
                std::istream bufStream(&buf);
                cereal::BinaryInputArchive archive(bufStream);
                modules[i] = loadShaderModule(device, archive);
            }
        }
    });

    world.entity("MeshLoader").set<ComponentAssetHandler, Mesh>({
        .load = [](flecs::iter iter, AssetData* data, void* pmeshes) {
            auto* meshes = static_cast<Mesh*>(pmeshes);
            for(auto i : iter) {
                membuf buf(data[i].data);
                std::istream bufStream(&buf);
                cereal::BinaryInputArchive archive(bufStream);
                archive(meshes[i]);
            }
        }
    });

    world.entity("ImageLoader").set<ComponentAssetHandler, Image>({
        .load = loadImages,
    });

    world.entity("PipelineLoader").set<ComponentAssetHandler, Pipeline>({
        .load = [](flecs::iter iter, AssetData* data, void* ppipelines) {

            const VkGlobals& vk = *iter.world().get<VkGlobals>();

            auto pipelines = static_cast<Pipeline*>(ppipelines);

            for(auto i : iter) {
                membuf buf(data[i].data);
                std::istream bufStream(&buf);
                cereal::BinaryInputArchive archive(bufStream);

                std::string vertexName;
                std::string fragmentName;
                archive(vertexName, fragmentName);

                auto parent = iter.entity(i).get_object(flecs::ChildOf);

                auto vertexEntity = parent.lookup(vertexName.c_str());
                auto fragmentEntity = parent.lookup(fragmentName.c_str());

                auto vertexModule = vertexEntity.get<kaki::ShaderModule>();
                auto fragmentModule = fragmentEntity.get<kaki::ShaderModule>();

                pipelines[i] = kaki::createPipeline(vk.device, vk.renderPass, vertexModule, fragmentModule);
            }

        }
    }).add<DependsOn>(shaderModuleLoader);

    world.entity("MeshFilterLoader").set<ComponentAssetHandler, MeshFilter>({
        .load = [](flecs::iter iter, AssetData* data, void* pfilters) {
            auto filters = static_cast<MeshFilter*>(pfilters);

            for(auto i : iter) {
                membuf buf(data[i].data);
                std::istream bufStream(&buf);
                cereal::BinaryInputArchive archive(bufStream);

                uint32_t primitiveIndex;
                uint64_t meshRef, albedoRef, normalRef, mrRef, aoRef, emissiveRef;
                archive(meshRef, primitiveIndex, albedoRef, normalRef, mrRef, aoRef, emissiveRef);

                filters[i].mesh = data->entityRefs[meshRef];
                filters[i].primitiveIndex = primitiveIndex;
                filters[i].albedo = data->entityRefs[albedoRef];
                filters[i].normal = data->entityRefs[normalRef];
                filters[i].metallicRoughness = data->entityRefs[mrRef];
                filters[i].ao = data->entityRefs[aoRef];
                filters[i].emissive = data->entityRefs[emissiveRef];
            }

        }
    });

    world.entity("CameraLoader").set<ComponentAssetHandler, Camera>({
        .load = [](flecs::iter iter, AssetData* data, void* pfilters) {
            auto cameras = static_cast<Camera*>(pfilters);

            for(auto i : iter) {
                membuf buf(data[i].data);
                std::istream bufStream(&buf);
                cereal::BinaryInputArchive archive(bufStream);

                archive(cameras[i].fov);
            }

        }
    });

    world.component<MeshFilter>()
            .member("mesh", ecs_id(ecs_entity_t))
            .member<uint32_t>("index")
            .member("albedo", ecs_id(ecs_entity_t))
            .member("normal", ecs_id(ecs_entity_t))
            .member("metallicRoughness", ecs_id(ecs_entity_t))
            .member("ao", ecs_id(ecs_entity_t))
            .member("emissive", ecs_id(ecs_entity_t));

    createGlobals(world);


    //TODO: support world space queries
    meshQuery = world.query_builder<kaki::MeshFilter>()
            .term<kaki::internal::MeshInstance>(flecs::Wildcard)
            .term<kaki::Transform>().set(flecs::Self | flecs::SuperSet, flecs::ChildOf)
            .build();

    world.system<VkGlobals>("Render system").kind(flecs::OnStore).each(render);

    world.trigger<MeshFilter>().event(flecs::OnSet).iter(UpdateMeshInstance);
}
