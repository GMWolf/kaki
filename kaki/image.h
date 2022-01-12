//
// Created by felix on 12/01/2022.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/vec2.hpp>

namespace kaki {

    struct Image
    {
        glm::ivec2 size;

        VkImage image;
        VmaAllocation allocation;
    };

    template<class Archive>
    void serialize(Archive& archive, Image& image) {
        archive(image.size.x, image.size.y);
    }
}