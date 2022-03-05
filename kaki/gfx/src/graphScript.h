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


    inline void renderWorld(flecs::world& world, VkCommandBuffer cmd) {

        auto& vk = *world.get_mut<VkGlobals>();
        auto& geometry = vk.geometry;
        auto pipeline = world.lookup("testpackage::pipeline").get<kaki::Pipeline>();

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

            world.filter_builder<kaki::MeshFilter, kaki::internal::MeshInstance>()
                    .term<kaki::Transform>().set(flecs::Self | flecs::SuperSet, flecs::ChildOf)
                    .build().iter([&](flecs::iter& it, kaki::MeshFilter* filters, kaki::internal::MeshInstance* meshInstances) {

                auto transforms = it.term<kaki::Transform>(3);

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
                        glm::mat4 proj{};
                        glm::vec3 viewPos{}; float pad0{};
                        kaki::Transform transform;
                        glm::vec3 light{}; float pad1{};
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
    }

    inline RenderGraph graphScript(VkGlobals& vk) {
        RenderGraphBuilder graph;

        auto depthImage = graph.image({
            .size = vk.swapchain.extent,
            .format = VK_FORMAT_D32_SFLOAT,
        });

        //Draw world
        graph.pass(renderWorld)
        .colorClear(DISPLAY_IMAGE_INDEX, {0.0f, 0.0f, 0.0f, 1.0f})
        .depthClear(depthImage, {0.0f, 0});

        //Draw Imgui
        graph.pass([](flecs::world& world, VkCommandBuffer cmd) {
            auto& vk = *world.get_mut<VkGlobals>();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
        })
        .color(DISPLAY_IMAGE_INDEX);

        return graph.build(vk);
    }

}