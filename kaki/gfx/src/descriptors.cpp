//
// Created by felix on 27/02/2022.
//

#include "descriptors.h"
#include <numeric>
#include <algorithm>

void kaki::fillDescSetWrites(kaki::VkGlobals &vk, VkDescriptorSet vkSet, kaki::DescriptorSet &descSetInfo,
                             std::span<ShaderInput> shaderInputs, std::span<VkWriteDescriptorSet> writes,
                             std::span<VkDescriptorImageInfo> imageInfos) {

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

void kaki::updateDescSets(kaki::VkGlobals &vk, std::span<VkDescriptorSet> descSets, const kaki::Pipeline &pipeline,
                          std::span<ShaderInput> shaderInputs) {

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
