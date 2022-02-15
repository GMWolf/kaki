//
// Created by felix on 12/01/2022.
//

#include "image.h"
#include <ktxvulkan.h>
#include "vk.h"
#include "membuf.h"

void* kaki::loadImages(flecs::entity &parent, size_t count, std::span<uint8_t> data) {

    membuf buf(data);
    std::istream bufStream(&buf);
    cereal::BinaryInputArchive archive(bufStream);

    const VkGlobals* vk = parent.world().get<VkGlobals>();

    ktxVulkanDeviceInfo kvdi;
    ktxVulkanDeviceInfo_Construct(&kvdi, vk->device.physical_device, vk->device, vk->queue, vk->cmdPool, nullptr);

    auto images = static_cast<kaki::Image*>(malloc(count * sizeof(kaki::Image)));
    std::vector<uint8_t> buffer;

    for(int i = 0; i < count; i++) {
        new (&images[i]) kaki::Image();

        archive(buffer);

        ktxTexture* kTexture;
        KTX_error_code result = ktxTexture_CreateFromMemory(buffer.data(), buffer.size(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
        assert (result == KTX_SUCCESS);

        ktxVulkanTexture texture;
        result = ktxTexture_VkUploadEx(kTexture, &kvdi, &texture,
                                       VK_IMAGE_TILING_OPTIMAL,
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        assert (result == KTX_SUCCESS);

        ktxTexture_Destroy(kTexture);

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

    return static_cast<void*>(images);
}
