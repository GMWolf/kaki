//
// Created by felix on 12/01/2022.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/vec2.hpp>
#include <flecs.h>
#include <kaki/asset.h>

namespace kaki {

    struct Image
    {
        VkImage image;
        VmaAllocation allocation;
        VkImageView view;
    };

    void imageLoadHandler(flecs::iter iter, kaki::Asset* assets);
}