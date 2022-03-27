//
// Created by felix on 19/03/2022.
//

#include "upload.h"

namespace kaki {


    void LinearUploadBuffer::reset() {
        linearAlloc.reset();
    }

    void *LinearUploadBuffer::alloc(size_t size, size_t align) {
        size_t offset = linearAlloc.alloc(size, align);
        return reinterpret_cast<std::byte*>( map ) + offset;
    }

    void LinearUploadBuffer::init(VmaAllocator allocator, size_t size) {

        linearAlloc.size = size;
        linearAlloc.head = 0;

        VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        };

        VmaAllocationCreateInfo vmaAllocInfo {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaAllocInfo, &buffer, &bufferAllocation, nullptr);
        vmaMapMemory(allocator, bufferAllocation, &map);
    }
}