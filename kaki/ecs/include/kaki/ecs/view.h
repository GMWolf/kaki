//
// Created by felix on 18/04/22.
//

#pragma once

#include "type.h"
#include <tuple>

namespace kaki::ecs {

    template<class... T>
    struct ChunkIterator {
        std::tuple<T*...> ptr;
        int idx;

        std::tuple<T&...> operator*() noexcept {
            return std::forward_as_tuple(std::get<T*>(ptr)[idx]...);
        }

        ChunkIterator operator+(size_t i) const noexcept {
            return ChunkIterator(std::get<T*>(ptr)..., idx + i);
        }

        ChunkIterator& operator++() noexcept {
            idx++;
            return *this;
        }

        bool operator==(const ChunkIterator& o) const {
            return ((std::get<0>(ptr) + idx) == (std::get<0>(o.ptr)+o.idx));
        }

        auto operator<=>(const ChunkIterator& o) const {
            return (std::get<0>(ptr) + idx <=> std::get<0>(o.ptr)+o.idx);
        }
    };


    void* chunkFindPtr(Chunk& chunk, id_t id) {
        for(size_t index = 0; index < chunk.type.components.size(); index++) {
            if (chunk.type.components[index] == id) {
                return chunk.components[index];
            }
        }
        return nullptr;
    }

    template<class A, class B>
    struct type_identity_unpacker {
        using type = A;
    };
    template<class A, class B>
    using type_identity_unpacker_t = typename type_identity_unpacker<A, B>::type;

    template<class... T>
    struct ChunkView {
        ChunkIterator<T...> beginIterator;
        ChunkIterator<T...> endIterator;

        explicit ChunkView(Chunk& chunk, type_identity_unpacker_t<id_t, T>... ids) {
            std::tuple<T*...> ptrs(static_cast<T*>(chunkFindPtr(chunk, ids))...);
            beginIterator.ptr = ptrs;
            beginIterator.idx = 0;
            endIterator.ptr = ptrs;
            endIterator.idx = chunk.size;
        }

        ChunkIterator<T...> begin() {
            return beginIterator;
        }

        ChunkIterator<T...> end() {
            return endIterator;
        }


    };

}