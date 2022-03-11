//
// Created by felix on 27/02/2022.
//

#pragma once

#include "render_graph.h"
#include "vk.h"
#include <kaki/gfx.h>
#include "pipeline.h"
#include <kaki/transform.h>
#include "gltf.h"
#include "descriptors.h"

namespace kaki {

    inline static glm::mat4 perspective(float vertical_fov, float aspect_ratio, float n, float f, glm::mat4 *inverse = nullptr)
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


    inline void renderWorld(flecs::world& world, VkCommandBuffer cmd, std::span<VkImageView> images) {

        auto& vk = *world.get_mut<VkGlobals>();
        auto& geometry = vk.geometry;
        auto pipeline = world.lookup("testpackage::visibility").get<kaki::Pipeline>();

        std::vector<VkDescriptorSetLayout> descSetLayouts;
        for(auto& descSet : pipeline->descriptorSets) {
            descSetLayouts.push_back(descSet.layout);
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);



        VkDeviceSize offsets[4]{0, 0, 0, 0};
        vkCmdBindVertexBuffers(vk.cmd[vk.currentFrame], 0, 4, geometry.vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vk.cmd[vk.currentFrame], geometry.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        world.each([&](const kaki::Camera& camera, const kaki::Transform& cameraTransform) {

            float aspect = (float)vk.swapchain.extent.width / (float)vk.swapchain.extent.height;
            glm::mat4 proj = perspective(camera.fov, aspect, 0.01f, 100.0f);
            glm::mat4 view = cameraTransform.inverse().matrix();
            glm::mat4 viewProj = proj * view;

            uint32_t drawIndex = 0;

            world.filter_builder<kaki::MeshFilter, kaki::internal::MeshInstance>()
                    .term<kaki::Transform>().set(flecs::Self | flecs::SuperSet, flecs::ChildOf)
                    .build().iter([&](flecs::iter& it, kaki::MeshFilter* filters, kaki::internal::MeshInstance* meshInstances) {

                auto transforms = it.term<kaki::Transform>(3);

                for(auto i : it) {
                    kaki::internal::MeshInstance& mesh = meshInstances[i];
                    auto material = flecs::entity(world, filters[i].material).get<kaki::Material>();
                    auto albedoImage = flecs::entity(world, material->albedo).get<kaki::Image>();
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
                                {"albedoTexture", albedoImage->view, vk.sampler},
                                {"positions", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.positionBuffer},
                                {"normals", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.normalBuffer},
                                {"tangents", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.tangentBuffer},
                                {"texcoords", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.texcoordBuffer},
                                {"drawInfoBuffer", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.drawInfoBuffer[vk.currentFrame]},
                        };

                        updateDescSets(vk, descriptorSets, *pipeline, inputs);

                        for(uint32_t d = 0; d < pipeline->descriptorSets.size(); d++) {
                            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                    pipeline->pipelineLayout,
                                                    pipeline->descriptorSets[d].index, 1, &descriptorSets[d], 0, nullptr);
                        }


                    }

                    struct Pc {
                        glm::mat4 proj{};
                        uint drawIndex{};
                    } pc;

                    pc.proj = viewProj;
                    pc.drawIndex = drawIndex;
                    //pc.light = -glm::normalize(glm::vec3(0, -1, 0.5));

                    vk.drawInfos[vk.currentFrame][drawIndex] = {
                            .transform = transform,
                            .indexOffset = mesh.indexOffset,
                            .vertexOffset = mesh.vertexOffset,
                            .material = filters[i].material,
                    };

                    vkCmdPushConstants(cmd, pipeline->pipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                       sizeof(Pc), &pc);


                    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, mesh.indexOffset, mesh.vertexOffset, 0);

                    drawIndex++;
                }

            });
        });
    }

    inline static void shade(flecs::world& world, VkCommandBuffer cmd, std::span<VkImageView> images) {

        auto& vk = *world.get_mut<VkGlobals>();
        auto& geometry = vk.geometry;

        world.each([&](flecs::entity materialEntity, Material& material) {
            auto albedoImage = flecs::entity(world, material.albedo).get<kaki::Image>();
            auto normalImage = flecs::entity(world, material.normal).get<kaki::Image>();
            auto metallicRoughnessImage = flecs::entity(world, material.metallicRoughness).get<kaki::Image>();
            auto aoImage = flecs::entity(world, material.ao).get<kaki::Image>();
            auto emissiveImage = flecs::entity(world, material.emissive).get<kaki::Image>();

            ShaderInput inputs[]{
                    {"albedoTexture", albedoImage->view, vk.sampler},
                    {"normalTexture", normalImage ? normalImage->view : VK_NULL_HANDLE, vk.sampler},
                    {"metallicRoughnessTexture", metallicRoughnessImage ? metallicRoughnessImage->view : VK_NULL_HANDLE, vk.sampler},
                    {"aoTexture", aoImage ? aoImage->view : VK_NULL_HANDLE, vk.sampler},
                    {"emissiveTexture", emissiveImage? emissiveImage->view : VK_NULL_HANDLE, vk.sampler},
                    {"positions", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.positionBuffer},
                    {"normals", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.normalBuffer},
                    {"tangents", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.tangentBuffer},
                    {"texcoords", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.texcoordBuffer},
                    {"drawInfoBuffer", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.drawInfoBuffer[vk.currentFrame]},
                    {"visbuffer", images[0], vk.uintSampler},
                    {"indices", VK_NULL_HANDLE, VK_NULL_HANDLE, vk.geometry.indexBuffer},
            };

            auto pipeline = world.lookup("testpackage::shade").get<kaki::Pipeline>();

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

            std::vector<VkDescriptorSetLayout> descSetLayouts;
            for(auto& descSet : pipeline->descriptorSets) {
                descSetLayouts.push_back(descSet.layout);
            }
            std::vector<VkDescriptorSet> descriptorSets(descSetLayouts.size());
            VkDescriptorSetAllocateInfo descAlloc{
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .descriptorPool = vk.descriptorPools[vk.currentFrame],
                    .descriptorSetCount = static_cast<uint32_t>(descSetLayouts.size()),
                    .pSetLayouts = descSetLayouts.data(),
            };
            vkAllocateDescriptorSets(vk.device, &descAlloc, descriptorSets.data());

            updateDescSets(vk, descriptorSets, *pipeline, inputs);
            for(uint32_t d = 0; d < pipeline->descriptorSets.size(); d++) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        pipeline->pipelineLayout,
                                        pipeline->descriptorSets[d].index, 1, &descriptorSets[d], 0, nullptr);
            }

            struct Pc {
                glm::mat4 proj{};
                glm::vec3 viewPos; uint drawId;
                glm::vec3 light; float pad1;
                glm::vec2 winSize;
                uint32_t material;
            } pc;

            world.each([&](const kaki::Camera& camera, const kaki::Transform& cameraTransform) {
                float aspect = (float) vk.swapchain.extent.width / (float) vk.swapchain.extent.height;
                 glm::mat4 proj = perspective(camera.fov, aspect, 0.01f, 100.0f);
                glm::mat4 view = cameraTransform.inverse().matrix();
                pc.proj = proj * view;
                pc.viewPos = cameraTransform.position;
            });

            pc.winSize = {vk.swapchain.extent.width, vk.swapchain.extent.height};
            pc.light = -glm::normalize(glm::vec3(0, -1, 0.5));
            pc.material = materialEntity.id();

            vkCmdPushConstants(cmd, pipeline->pipelineLayout,
                               VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(Pc), &pc);
            vkCmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
        });
    }

    inline RenderGraph graphScript(VkGlobals& vk) {
        RenderGraphBuilder graph;

        auto depthImage = graph.image({
            .size = vk.swapchain.extent,
            .format = VK_FORMAT_D32_SFLOAT,
        });

        auto visbuffer = graph.image({
            .size = vk.swapchain.extent,
            .format = VK_FORMAT_R32G32_UINT,
        });

        // Draw v buffer
        graph.pass(renderWorld)
        .color(visbuffer)
        .depthClear(depthImage, {0.0f, 0});

        // Shade
        graph.pass(shade)
        .colorClear(DISPLAY_IMAGE_INDEX, VkClearColorValue{.uint32 = {0, 0, 0, 0}})
        .imageRead(visbuffer);

        //Draw Imgui
        graph.pass([](flecs::world& world, VkCommandBuffer cmd, std::span<VkImageView> images) {
            auto& vk = *world.get_mut<VkGlobals>();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        })
        .color(DISPLAY_IMAGE_INDEX);

        return graph.build(vk);
    }

}