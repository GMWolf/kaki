//
// Created by felix on 27/02/2022.
//

#include "descriptors.h"
#include <numeric>
#include <algorithm>

namespace kaki {

    static void fillDescSetWrites(VkDescriptorSet vkSet, const kaki::DescriptorSet &descSetInfo,
                                  std::span<ShaderInput> shaderInputs, std::span<VkWriteDescriptorSet> writes,
                                  std::span<VkDescriptorImageInfo> imageInfos,
                                  std::span<VkDescriptorBufferInfo> bufferInfos) {

        for (int i = 0; i < descSetInfo.bindings.size(); i++) {
            const auto &name = descSetInfo.bindingNames[i];
            auto input = std::ranges::find_if(shaderInputs, [name](ShaderInput &input) { return input.name == name; });
            assert(input != shaderInputs.end());

            switch (descSetInfo.bindings[i].descriptorType) {
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    imageInfos[i] = {
                            .sampler = input->sampler,
                            .imageView = input->imageView,
                            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    };
                    break;
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    bufferInfos[i] = VkDescriptorBufferInfo{
                            .buffer = input->buffer,
                            .offset = 0,
                            .range = VK_WHOLE_SIZE,
                    };
                    break;
                default:
                    assert(false);
            }

            writes[i] = VkWriteDescriptorSet{
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = vkSet,
                    .dstBinding = descSetInfo.bindings[i].binding,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = descSetInfo.bindings[i].descriptorType,
            };
        }
    }

    void addDescSetWrites(DescSetWriteCtx& ctx, VkDescriptorSet vkSet, const kaki::DescriptorSet &descSetInfo, std::span<ShaderInput> shaderInputs) {

        const size_t writeCount = descSetInfo.bindings.size();

        ctx.descSetWrites.resize(ctx.descSetWrites.size() + writeCount);
        ctx.imageInfos.resize(ctx.imageInfos.size() + writeCount);
        ctx.bufferInfos.resize(ctx.bufferInfos.size() + writeCount);

        fillDescSetWrites(vkSet, descSetInfo, shaderInputs,
                          std::span(ctx.descSetWrites).last(writeCount),
                          std::span(ctx.imageInfos).last(writeCount),
                          std::span(ctx.bufferInfos).last(writeCount));

    }

    static void WriteResInfoPtrs(DescSetWriteCtx& ctx) {
        for(int i = 0; i < ctx.descSetWrites.size(); i++) {
            ctx.descSetWrites[i].pImageInfo = &ctx.imageInfos[i];
            ctx.descSetWrites[i].pBufferInfo = &ctx.bufferInfos[i];
        }
    }

    void updateDescSets(VkGlobals &vk, DescSetWriteCtx &ctx) {
        WriteResInfoPtrs( ctx);
        vkUpdateDescriptorSets(vk.device, ctx.descSetWrites.size(), ctx.descSetWrites.data(), 0, nullptr);
    }

    void updateDescSets(VkGlobals &vk, std::span<VkDescriptorSet> descSets, const Pipeline &pipeline,
                              std::span<ShaderInput> shaderInputs) {

        size_t writeCount = std::accumulate(pipeline.descriptorSets.begin(), pipeline.descriptorSets.end(), 0,
                                            [](size_t a, const DescriptorSet &set) {
                                                return a + set.bindings.size();
                                            });

        DescSetWriteCtx ctx;
        ctx.reserve(writeCount);

        for (int i = 0; i < pipeline.descriptorSets.size(); i++) {
            auto vkSet = descSets[i];
            auto set = pipeline.descriptorSets[i];

            addDescSetWrites(ctx, vkSet, set, shaderInputs);
        }

        updateDescSets( vk, ctx );
    }

    void DescSetWriteCtx::reserve(size_t count) {
        bufferInfos.reserve(count);
        imageInfos.reserve(count);
        descSetWrites.reserve(count);
    }
}