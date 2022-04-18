//
// Created by felix on 18/04/22.
//

#pragma once

#include <atomic>
#include <cstdint>
#include "component.h"
#include "type.h"

namespace kaki::ecs {

    struct Chunk {
        Type type;
        size_t capacity;
        size_t size;
        std::vector<void*> components;
    };

    struct Archetype {
        Type type;
        std::vector<id_t> entities;
        std::vector<Chunk*> chunks;
    };

    struct EntityRecord {
        Chunk* chunk;
        size_t row;
        uint32_t generation;
    };

    struct Registry {
        std::vector<Archetype> archetypes;
        std::vector<uint32_t> freeIds;
        std::vector<EntityRecord> records;

        id_t create(const Type& type, std::string_view name = {});
        void destroy(id_t id);
        id_t registerComponent(const ComponentInfo& component, std::string_view name = {});

        void* get(id_t entity, id_t component, size_t size);

        template<class T>
        T* get(id_t entity, id_t component = ComponentTrait<T>::id) {
            return static_cast<T*>(get(entity, component, sizeof(T)));
        }

        template<class T>
        id_t registerComponent(const std::string_view name = {}) {
            assert(ComponentTrait<T>::id == 0);
            id_t id = registerComponent(componentInfo<T>(), name);
            ComponentTrait<T>::id = id;
            return id;
        }

        Registry();
    };

}