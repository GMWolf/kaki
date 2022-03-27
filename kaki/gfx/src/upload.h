//
// Created by felix on 19/03/2022.
//

#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "allocator.h"


namespace kaki {

    struct LinearUploadBuffer {
        LinearAllocator linearAlloc;

        VkBuffer buffer;
        VmaAllocation bufferAllocation;
        void* map;

        void init(VmaAllocator allocator, size_t size);

        void reset();
        void* alloc(size_t size, size_t align);
    };

}