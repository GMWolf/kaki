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
        .attachmentCount = 2,
        .pAttachments = views,
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

    VkSurfaceKHR surface = world.lookup("window").get<kaki::Window>()->createSurface(vkb_inst);

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

    {
        uint32_t queueIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

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

    for(int i = 0; i < vk.swapchain.image_count; i++) {
        vk.framebuffer[i] = createFrameBuffer(vk.device, vk.renderPass, vk.swapchain.get_image_views()->at(i), vk.depthBufferView, vk.swapchain.extent.width, vk.swapchain.extent.height);
    }

    {
        VkDescriptorPoolSize descriptorPoolSize[]{
                VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 32,
                }
        };

        VkDescriptorPoolCreateInfo descPoolInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 32,
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


static void render(const flecs::entity& entity, kaki::VkGlobals& vk) {

    auto world = entity.world();

    vkWaitForFences(vk.device, 1, &vk.cmdBufFence[vk.currentFrame], true, UINT64_MAX);

    vkResetDescriptorPool(vk.device, vk.descriptorPools[vk.currentFrame], 0);
    vkResetCommandBuffer(vk.cmd[vk.currentFrame], 0);


    uint32_t imageIndex;
    auto acquire = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore[vk.currentFrame], nullptr, &imageIndex);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR ){

    } else if (acquire != VK_SUCCESS) {
        fprintf(stderr, "Error acquiring swapchain image.\n");
    }

    static float time = 0;
    time += entity.world().delta_time();

    VkClearColorValue clearColor = { 0, 0, 0, 1.0f };
    VkClearValue clearValue = {};
    clearValue.color = clearColor;

    VkClearDepthStencilValue clearDepth {1.0f, 0};
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

    entity.world().each([&](const kaki::Camera& camera, const kaki::Transform& cameraTransform) {

        float aspect = (float)vk.swapchain.extent.width / (float)vk.swapchain.extent.height;
        glm::mat4 proj = glm::perspective(camera.fov, aspect, 0.01f, 1000.0f);
        glm::mat4 view = cameraTransform.inverse().matrix();
        glm::mat4 viewProj = proj * view;


        entity.world().filter_builder<kaki::Transform, kaki::MeshFilter>().term<kaki::internal::MeshInstance>(flecs::Wildcard).build()
        .iter([&](flecs::iter& it, kaki::Transform* transforms, kaki::MeshFilter* filters) {

            auto meshInstances = it.term<kaki::internal::MeshInstance>(3);
            auto gltfE = it.pair(3).object();
            auto gltf = gltfE.get<kaki::Gltf>();

            VkDeviceSize offsets[4]{0, 0, 0, 0};
            vkCmdBindVertexBuffers(vk.cmd[vk.currentFrame], 0, 4, gltf->buffers, offsets);
            vkCmdBindIndexBuffer(vk.cmd[vk.currentFrame], gltf->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            for(auto i : it) {
                auto& mesh = meshInstances[i];
                auto albedoImage = flecs::entity(world, filters[i].albedo).get<kaki::Image>();
                auto normalImage = flecs::entity(world, filters[i].normal).get<kaki::Image>();
                auto metallicRoughnessImage = flecs::entity(world, filters[i].metallicRoughness).get<kaki::Image>();
                auto aoImage = flecs::entity(world, filters[i].ao).get<kaki::Image>();
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
                        {"normalTexture", normalImage->view},
                        {"metallicRoughnessTexture", metallicRoughnessImage->view},
                        {"aoTexture", aoImage->view},
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
                pc.light = glm::normalize(glm::vec3(1, -0.5, -1));

                vkCmdPushConstants(vk.cmd[vk.currentFrame], pipeline->pipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                   sizeof(Pc), &pc);

                for (auto &prim: mesh.primitives) {
                    vkCmdDrawIndexed(vk.cmd[vk.currentFrame], prim.indexCount, 1, prim.indexOffset, prim.vertexOffset,
                                     0);
                }
            }

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



static void UpdateMeshInstance(flecs::iter iter, kaki::MeshFilter* meshFilters) {
    for(auto i : iter) {

        auto mesh =  flecs::entity(iter.world(), meshFilters[i].mesh).get<kaki::Mesh>();
        auto gltf = flecs::entity(iter.world(), meshFilters[i].mesh).get_object(flecs::ChildOf);

        std::vector<kaki::internal::MeshInstance::Primitive> prims(mesh->primitives.size());
        for(int i = 0; i < mesh->primitives.size(); i++) {
            prims[i].indexOffset = mesh->primitives[i].indexOffset;
            prims[i].vertexOffset = mesh->primitives[i].vertexOffset;
            prims[i].indexCount = mesh->primitives[i].indexCount;
        }

        iter.entity(i).set<kaki::internal::MeshInstance>(gltf, kaki::internal::MeshInstance{
            .primitives = prims,
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

                uint64_t meshRef, albedoRef, normalRef, mrRef, aoRef;
                archive(meshRef, albedoRef, normalRef, mrRef, aoRef);

                filters[i].mesh = data->entityRefs[meshRef];
                filters[i].albedo = data->entityRefs[albedoRef];
                filters[i].normal = data->entityRefs[normalRef];
                filters[i].metallicRoughness = data->entityRefs[mrRef];
                filters[i].ao = data->entityRefs[aoRef];
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
            .member("albedo", ecs_id(ecs_entity_t))
            .member("normal", ecs_id(ecs_entity_t))
            .member("metallicRoughness", ecs_id(ecs_entity_t))
            .member("ao", ecs_id(ecs_entity_t));

    createGlobals(world);
    world.system<VkGlobals>("Render system").kind(flecs::OnStore).each(render);

    world.trigger<MeshFilter>().event(flecs::OnSet).iter(UpdateMeshInstance);
}
