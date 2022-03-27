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
#include "entity_tree.h"
#include "graphScript.h"
#include "renderpass.h"
#include "material.h"

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

    {
        VkDescriptorPoolSize descriptorPoolSize[]{
                VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 4096,
                },
                VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        .descriptorCount = 1024,
                },
        };

        VkDescriptorPoolCreateInfo descPoolInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 4096,
                .poolSizeCount = std::span(descriptorPoolSize).size(),
                .pPoolSizes = descriptorPoolSize,
        };

        for(auto & descriptorPool : vk.descriptorPools) {
            vkCreateDescriptorPool(vk.device, &descPoolInfo, nullptr, &descriptorPool);
        }
    }

    {
        VkDescriptorPoolSize descriptorPoolSize[]{
                VkDescriptorPoolSize{
                        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .descriptorCount = 1024,
                },
        };

        VkDescriptorPoolCreateInfo descPoolInfo{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1024,
                .poolSizeCount = std::span(descriptorPoolSize).size(),
                .pPoolSizes = descriptorPoolSize,
        };

        vkCreateDescriptorPool(vk.device, &descPoolInfo, nullptr, &vk.staticDescPool);
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

        VkSamplerCreateInfo samplerCreateInfoUint{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .magFilter = VK_FILTER_NEAREST,
                .minFilter = VK_FILTER_NEAREST,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .minLod = 0,
                .maxLod = VK_LOD_CLAMP_NONE,
        };

        vkCreateSampler(vk.device, &samplerCreateInfoUint, nullptr, &vk.uintSampler);
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
    //io.ConfigFlags |= ImGuiConfigFlags_;
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

    VkFormat colorFormats[] = {vk.swapchain.image_format};
    ImGui_ImplVulkan_Init(&init_info, createCompatRenderPass(vk.device, colorFormats, {}));

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

    vk.script = kaki::graphScript;
    vk.graph = graphScript(vk);

    kaki::allocGeometryBuffer(vk.allocator, vk.geometry, 9 * 1024 * 1024, 3 * 1024 * 1024);

    for(uint i = 0; i < vk.framesInFlight; i++)
    {
        VkBufferCreateInfo bufferInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = sizeof(kaki::DrawInfo) * vk.maxDrawCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        };

        VmaAllocationCreateInfo vmaAllocInfo {
                .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
                .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        vmaCreateBuffer(vk.allocator, &bufferInfo, &vmaAllocInfo, &vk.drawInfoBuffer[i], &vk.drawInfoAlloc[i], nullptr);
        vmaMapMemory(vk.allocator, vk.drawInfoAlloc[i], (void**)&vk.drawInfos[i]);
    }

    for(auto & i : vk.uploadBuffer) {
        i.init(vk.allocator, vk.stagingBufferSize);
    }

    {
        VkBufferCreateInfo bufferInfo {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = sizeof(kaki::Transform) * vk.maxDrawCount,
                .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        VmaAllocationCreateInfo vmaAllocInfo {
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        vmaCreateBuffer(vk.allocator, &bufferInfo, &vmaAllocInfo, &vk.transformBuffer, &vk.transformAlloc, nullptr);

        VmaVirtualBlockCreateInfo blockCreateInfo = {};
        blockCreateInfo.size = bufferInfo.size;

        vmaCreateVirtualBlock(&blockCreateInfo, &vk.transformBlock);
    }


    world.set<kaki::VkGlobals>(vk);
    return true;
}

void recreate_swapchain(kaki::VkGlobals& vk) {
    vk.swapchain.destroy_image_views(vk.imageViews);
    createSwapChain(vk);
}


static void UploadTransforms(kaki::VkGlobals& vk) {

    std::vector<VkBufferMemoryBarrier> barriers;


    vk.transformAllocQuery.iter([&](flecs::iter iter, kaki::TransformGpuAddress* address){

        if (address->count < iter.count()) {
            if (address->count != 0) {
                vmaVirtualFree(vk.transformBlock, address->allocation);
            }
            VmaVirtualAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.size = sizeof(kaki::Transform) * iter.count();
            allocCreateInfo.alignment = alignof(kaki::Transform);
            size_t offset;
            vmaVirtualAllocate(vk.transformBlock, &allocCreateInfo, &address->allocation, &offset);
            address->offset = offset / sizeof(kaki::Transform);
            address->count = iter.count();

            for(int i = 1; i < iter.count(); i++) {
                address[i].offset = address[0].offset + i;
                address[i].count = address[0].count - i;
            }
        } else {
            iter.skip();
        }

    });

    int count = 0;

    vk.transformUploadQuery.iter([&](flecs::iter iter, const kaki::Transform* transforms, const kaki::TransformGpuAddress* addresses) {

        if (iter.changed()) {
            count++;
            const kaki::TransformGpuAddress &address = addresses[0];

            void *staging = vk.uploadBuffer[vk.currentFrame].alloc(iter.count() * sizeof(kaki::Transform),
                                                                     alignof(kaki::Transform));
            memcpy(staging, transforms, sizeof(kaki::Transform) * iter.count());

            VkBufferCopy region{
                    .srcOffset = static_cast<VkDeviceSize>(std::distance(
                            (std::byte *) vk.uploadBuffer[vk.currentFrame].map, (std::byte *) staging)),
                    .dstOffset = address.offset * sizeof(kaki::Transform),
                    .size = sizeof(kaki::Transform) * iter.count(),
            };

            vkCmdCopyBuffer(vk.cmd[vk.currentFrame], vk.uploadBuffer[vk.currentFrame].buffer, vk.transformBuffer, 1, &region);

            VkBufferMemoryBarrier barrier{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                    .dstAccessMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                    .srcQueueFamilyIndex = vk.device.get_queue_index(vkb::QueueType::graphics).value(),
                    .dstQueueFamilyIndex = vk.device.get_queue_index(vkb::QueueType::graphics).value(),
                    .buffer = vk.transformBuffer,
                    .offset = region.dstOffset,
                    .size = region.size,
            };

            barriers.push_back(barrier);

        }
    });

    vkCmdPipelineBarrier(vk.cmd[vk.currentFrame], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                         0, 0, nullptr, barriers.size(), barriers.data(), 0, nullptr);

    printf("count: %d\n", count);
}

static void render(const flecs::entity& entity, kaki::VkGlobals& vk) {

    auto world = entity.world();

    vkWaitForFences(vk.device, 1, &vk.cmdBufFence[vk.currentFrame], true, UINT64_MAX);

    vkResetDescriptorPool(vk.device, vk.descriptorPools[vk.currentFrame], 0);
    vkResetCommandBuffer(vk.cmd[vk.currentFrame], 0);

    vk.uploadBuffer[vk.currentFrame].reset();

    uint32_t imageIndex;
    auto acquire = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.imageAvailableSemaphore[vk.currentFrame], nullptr, &imageIndex);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR || acquire == VK_SUBOPTIMAL_KHR ){
        vkDeviceWaitIdle(vk.device);
        kaki::destroyGraph(vk, vk.graph);
        recreate_swapchain (vk);
        vk.graph = vk.script(vk);
        return;
    } else if (acquire != VK_SUCCESS) {
        fprintf(stderr, "Error acquiring swapchain image.\n");
        return;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    kaki::entityTree(world);
    ImGui::Render();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    vkBeginCommandBuffer(vk.cmd[vk.currentFrame], &beginInfo);

    UploadTransforms(vk);

    kaki::runGraph(world, vk.graph, imageIndex, vk.cmd[vk.currentFrame]);
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
        auto gltfEntity = flecs::entity(iter.world(), meshFilters[i].mesh).get_object(flecs::ChildOf);

        auto gltf = gltfEntity.get<kaki::Gltf>();

        iter.entity(i).set(kaki::internal::MeshInstance {
            .indexOffset = static_cast<uint32_t>(mesh->primitives[meshFilters[i].primitiveIndex].indexOffset + gltf->meshAlloc.indexOffset),
            .vertexOffset = static_cast<uint32_t>(mesh->primitives[meshFilters[i].primitiveIndex].vertexOffset + gltf->meshAlloc.vertexOffset),
            .indexCount = mesh->primitives[meshFilters[i].primitiveIndex].indexCount,
        } );
    }
}


namespace kaki {
    Pipeline createPipeline(const VkGlobals &vk, flecs::entity scope, std::span<uint8_t> pipelineData);
}
kaki::gfx::gfx(flecs::world &world) {
    auto This = world.module<gfx>();
    flecs::doc::set_brief(This, "Rendering module for kaki");

    world.component<kaki::Mesh>();
    world.component<kaki::Image>();
    world.component<kaki::Pipeline>();
    world.component<kaki::Gltf>();
    world.component<kaki::MeshFilter>();
    world.component<kaki::Material>();

    world.component<kaki::Camera>()
            .member<float>("yfov");

    world.component<kaki::DependsOn>().add(flecs::Acyclic);

    auto gltfLoader = world.entity("GltfLoader").set<ComponentAssetHandler, Gltf>({
        .load = loadGltfs,
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
    }).add<DependsOn>(gltfLoader);

    auto imageLoader = world.entity("ImageLoader").set<ComponentAssetHandler, Image>({
        .load = loadImages,
    });

    auto pipelineLoader = world.entity("PipelineLoader").set<ComponentAssetHandler, Pipeline>({
        .load = [](flecs::iter iter, AssetData* data, void* ppipelines) {

            const VkGlobals& vk = *iter.world().get<VkGlobals>();

            auto pipelines = static_cast<Pipeline*>(ppipelines);

            for(auto i : iter) {
                auto parent = iter.entity(i).get_object(flecs::ChildOf);
                pipelines[i] = kaki::createPipeline(vk, parent, data[i].data);
            }

        }
    });

    world.entity("MeshFilterLoader").set<ComponentAssetHandler, MeshFilter>({
        .load = [](flecs::iter iter, AssetData* data, void* pfilters) {
            auto filters = static_cast<MeshFilter*>(pfilters);

            for(auto i : iter) {
                membuf buf(data[i].data);
                std::istream bufStream(&buf);
                cereal::BinaryInputArchive archive(bufStream);

                uint32_t primitiveIndex;
                uint64_t meshRef, materialRef;
                archive(meshRef, primitiveIndex, materialRef);

                filters[i].mesh = data->entityRefs[meshRef];
                filters[i].primitiveIndex = primitiveIndex;
                filters[i].material = data->entityRefs[materialRef];
            }

        }
    });

    world.entity("MaterialLoader").set<ComponentAssetHandler, Material>({
        .load = kaki::loadMaterials,
    }).add<DependsOn>(imageLoader).add<DependsOn>(pipelineLoader);

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
            .member(ecs_id(ecs_entity_t), "mesh")
            .member<uint32_t>("index")
            .member(ecs_id(ecs_entity_t), "material");

    createGlobals(world);

    auto& vk = *world.get_mut<VkGlobals>();
    vk.transformUploadQuery = world.query_builder<const Transform, const TransformGpuAddress>()
            .instanced()
            .arg(1).owned(true)
            .arg(2).owned(true)
            .build();

    vk.transformAllocQuery = world.query_builder<TransformGpuAddress>()
            .arg(1).owned(true)
            .build();

    world.component<Transform>().add(flecs::With, world.component<TransformGpuAddress>());

    world.system<VkGlobals>("Render system").kind(flecs::OnStore).each(render);

    world.trigger<MeshFilter>().event(flecs::OnSet).iter(UpdateMeshInstance);
}
