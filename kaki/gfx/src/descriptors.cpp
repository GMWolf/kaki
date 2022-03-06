//
// Created by felix on 27/02/2022.
//

#include "descriptors.h"
#include <numeric>
#include <algorithm>

namespace kaki {

    static void fillDescSetWrites(kaki::VkGlobals &vk, VkDescriptorSet vkSet, kaki::DescriptorSet &descSetInfo,
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
                            .sampler = vk.sampler,
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
                    .pImageInfo = &imageInfos[i],
                    .pBufferInfo = &bufferInfos[i],
            };
        }
    }

    void updateDescSets(VkGlobals &vk, std::span<VkDescriptorSet> descSets, const Pipeline &pipeline,
                              std::span<ShaderInput> shaderInputs) {

        size_t writeCount = std::accumulate(pipeline.descriptorSets.begin(), pipeline.descriptorSets.end(), 0,
                                            [](size_t a, const DescriptorSet &set) {
                                                return a + set.bindings.size();
                                            });;

        std::vector<VkWriteDescriptorSet> descSetWrites(writeCount);
        std::vector<VkDescriptorImageInfo> imageInfos(writeCount);
        std::vector<VkDescriptorBufferInfo> bufferInfos(writeCount);

        std::span<VkWriteDescriptorSet> descSetWriteRange = descSetWrites;
        std::span<VkDescriptorImageInfo> imageInfoRange = imageInfos;
        std::span<VkDescriptorBufferInfo> bufferInfoRange = bufferInfos;

        for (int i = 0; i < pipeline.descriptorSets.size(); i++) {
            auto vkSet = descSets[i];
            auto set = pipeline.descriptorSets[i];

            fillDescSetWrites(vk, vkSet, set, shaderInputs, descSetWriteRange, imageInfoRange, bufferInfoRange);
            descSetWriteRange = descSetWriteRange.subspan(set.bindings.size(),
                                                          descSetWriteRange.size() - set.bindings.size());
            imageInfoRange = imageInfoRange.subspan(set.bindings.size(), imageInfoRange.size() - set.bindings.size());
            bufferInfoRange = bufferInfoRange.subspan(set.bindings.size(),
                                                      bufferInfoRange.size() - set.bindings.size());
        }

        vkUpdateDescriptorSets(vk.device, writeCount, descSetWrites.data(), 0, nullptr);
    }

}