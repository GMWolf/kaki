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
        std::vector<id_t> ids;
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
        std::vector<id_t> freeIds;
        std::vector<EntityRecord> records;

        EntityId create(const Type& type, std::string_view name = {});
        void destroy(EntityId id);
        EntityId registerComponent(const ComponentInfo& component, std::string_view name = {});

        void add(EntityId entity, ComponentType component, void* ptr = nullptr);

        template<class T>
        void add(EntityId entity) {
            add(entity, ComponentTrait<T>::id);
        }

        void* get(EntityId entity, ComponentType component, size_t size);

        template<class T>
        T* get(EntityId entity, ComponentType component = ComponentTrait<T>::id) {
            return static_cast<T*>(get(entity, component, sizeof(T)));
        }

        template<class T>
        EntityId registerComponent(const std::string_view name = {}) {
            assert(ComponentTrait<T>::id.component == 0);
            EntityId id = registerComponent(componentInfo<T>(), name);
            ComponentTrait<T>::id.component = id.id;
            return id;
        }

        EntityId lookup(const char* string, EntityId root = {0, 0});

        Registry();
    };

}