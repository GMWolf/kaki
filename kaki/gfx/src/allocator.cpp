//
// Created by felix on 19/03/2022.
//

#include <cassert>
#include "allocator.h"

void kaki::LinearAllocator::reset() {
    head = 0;
}

size_t kaki::LinearAllocator::alloc(size_t size, size_t align) {
    size_t a = (head + (align - 1)) & -align;
    assert( a + size <= this->size );
    head = a + size;
    return a;
}
