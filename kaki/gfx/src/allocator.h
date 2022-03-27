//
// Created by felix on 19/03/2022.
//

#pragma once

#include <cstddef>

namespace kaki {

    struct LinearAllocator {
        size_t size;
        size_t head;

        void reset();
        size_t alloc(size_t size, size_t align);
    };

}