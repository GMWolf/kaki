//
// Created by felix on 18/04/22.
//

#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

namespace kaki::ecs {

    using id_t = uint64_t;

    inline id_t idComponent(id_t id) {
        return id & ((1ull << 32ull) - 1);
    }

    inline id_t idObject(id_t id) {
        return id >> 32ull;
    }

    inline id_t idEntity(id_t id) {
        return id & ((1ull << 32ull) - 1);
    }

    inline id_t idGeneration(id_t id) {
        return id >> 32ull;
    }

    struct Type {
        inline Type(std::initializer_list<id_t> l) : components(l){
            std::sort(components.begin(), components.end());
        }

        std::vector<id_t> components;
    };

    inline auto operator<=>(const Type& a, const Type& b) {
        return a.components <=> b.components;
    }

    inline auto operator==(const Type& a, const Type& b) {
        return a.components == b.components;
    }

    inline id_t relation(id_t component, id_t object) {
        return idEntity(component) | (idEntity(object) << 32ull);
    }

}