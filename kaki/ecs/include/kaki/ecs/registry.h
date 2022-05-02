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
        EntityId registerComponent(const ComponentInfo& component, std::string_view name = {}, EntityId parent = {});

        void add(EntityId entity, ComponentType component, const void* ptr = nullptr);

        template<class T>
        void add(EntityId entity, const T& t) {
            add(entity, ComponentTrait<T>::id, static_cast<const void*>(&t));
        }

        void* get(EntityId entity, ComponentType component, size_t size);

        template<class T>
        T* get(EntityId entity, ComponentType component = ComponentTrait<T>::id) {
            return static_cast<T*>(get(entity, component, sizeof(T)));
        }

        Chunk* createChunk(const Type& type, size_t entityCount);

        template<class T>
        EntityId registerComponent(const std::string_view name = {}, EntityId parent = {}) {
            assert(ComponentTrait<T>::id.component == 0);
            EntityId id = registerComponent(componentInfo<T>(), name, parent);
            ComponentTrait<T>::id.component = id.id;
            return id;
        }

        EntityId lookup(const char* string, EntityId root = {0, 0});

        Registry();
    };

}