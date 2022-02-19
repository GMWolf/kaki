//
// Created by felix on 12/01/2022.
//

#include "image.h"
#include <ktxvulkan.h>
#include "vk.h"

void kaki::loadImages(flecs::iter iter, AssetData* data, void* pimages) {


    auto* images = static_cast<Image*>(pimages);


    const VkGlobals* vk = iter.world().get<VkGlobals>();

    ktxVulkanDeviceInfo kvdi;
    ktxVulkanDeviceInfo_Construct(&kvdi, vk->device.physical_device, vk->device, vk->queue, vk->cmdPool, nullptr);

    for(auto i : iter) {
        ktxTexture2* kTexture;
        KTX_error_code result = ktxTexture2_CreateFromMemory(data[i].data.data(), data[i].data.size(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
        assert (result == KTX_SUCCESS);

        if (ktxTexture2_NeedsTranscoding(kTexture)) {
            ktxTexture2_TranscodeBasis(kTexture, KTX_TTF_BC7_RGBA, 0);
        }

        ktxVulkanTexture texture;
        result = ktxTexture2_VkUploadEx(kTexture, &kvdi, &texture,
                                       VK_IMAGE_TILING_OPTIMAL,
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        assert (result == KTX_SUCCESS);

        ktxTexture_Destroy(ktxTexture(kTexture));

        VkImageSubresourceRange imageRange {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = texture.levelCount,
                .baseArrayLayer = 0,
                .layerCount = texture.layerCount * (texture.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY ? 6 : 1),
        };

        VkImageViewCreateInfo viewInfo {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = texture.image,
                .viewType = texture.viewType,
                .format = texture.imageFormat,
                .components = VkComponentMapping {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = imageRange,
        };

        VkImageView view;
        vkCreateImageView(vk->device, &viewInfo, nullptr, &view);

        images[i] = kaki::Image{
                .image = texture.image,
                .view = view
        };
    }


    ktxVulkanDeviceInfo_Destruct(&kvdi);
}
