//
// Created by felix on 18/04/22.
//
#include <registry.h>
#include <identifier.h>
#include "childof.h"

namespace kaki::ecs {

    ComponentType identifierType {};
    ComponentType componentType {};

    static Archetype *findArchetype(Registry &registry, const Type &type) {
        for (Archetype &archetype: registry.archetypes) {
            if (archetype.type == type) {
                return &archetype;
            }
        }
        return nullptr;
    }

    static Archetype &getArchetype(Registry &registry, const Type &type) {
        Archetype *archetype = findArchetype(registry, type);
        if (!archetype) {
            archetype = &registry.archetypes.emplace_back(Archetype{
                    .type = type,
                    .entities = {},
                    .chunks = {},
            });
        }
        return *archetype;
    }

    static int typeGetComponentIndex(const Type &type, ComponentType component) {
        for (int index = 0; index < type.components.size(); index++) {
            if (type.components[index] == component) {
                return index;
            }
        }
        return -1;
    }

    static void *getComponent(Registry &registry, id_t entityId, ComponentType component, size_t componentSize) {
        auto record = registry.records[entityId];

        auto index = typeGetComponentIndex(record.chunk->type, component);
        if (index < 0) {
            return nullptr;
        }

        return static_cast<char *>(record.chunk->components[index]) + (record.row * componentSize);
    }

    static Chunk *createChunk(Registry &registry, const Type &type, size_t capacity) {
        auto *chunk = new Chunk {
                .type = type,
                .capacity = capacity,
                .size = 0,
                .components = std::vector<void *>(type.components.size()),
                .ids = std::vector<id_t>(capacity),
        };

        for (int i = 0; i < type.components.size(); i++) {
            auto *component = static_cast<ComponentInfo *>(getComponent(registry, type.components[i].component, componentType,
                                                                    sizeof(ComponentInfo)));
            if (component) {
                size_t componentSize = component->size;
                chunk->components[i] = malloc(capacity * componentSize);
            } else {
                chunk->components[i] = nullptr;
            }
        }

        return chunk;
    }

    static Chunk *getFreeChunk(Registry &registry, Archetype &archetype, const Type &type) {
        for (auto chunk: archetype.chunks) {
            if (chunk->size < chunk->capacity) {
                return chunk;
            }
        }

        size_t capacity = 1024;
        return archetype.chunks.emplace_back(createChunk(registry, type, capacity));
    }

    EntityId Registry::create(const Type &inType,  std::string_view name) {
        Type type = inType;
        type.components.push_back(ComponentTrait<EntityId>::id);
        if(!name.empty()) {
            type.components.push_back(identifierType);
            std::sort(type.components.begin(), type.components.end());
        }

        Archetype &archetype = getArchetype(*this, type);
        Chunk *chunk = getFreeChunk(*this, archetype, type);

        id_t entityId = 0;
        id_t generation = 0;
        size_t row = chunk->size++;
        if (freeIds.empty()) {
            generation = 1;
            records.push_back(EntityRecord {
                    .chunk = chunk,
                    .row = row,
                    .generation = static_cast<uint32_t>(generation),
            });
            entityId = records.size() - 1;
        } else {
            entityId = freeIds.back();
            freeIds.pop_back();

            records[entityId].chunk = chunk;
            records[entityId].row = row;
            generation = records[entityId].generation;
        }

        EntityId entity {
            .id = entityId,
            .generation = generation,
        };

        // construct components
        {
            for( uint i = 0; i < chunk->type.components.size(); i++ )
            {
                ComponentType t = chunk->type.components[i];
                auto *component = static_cast<ComponentInfo *>(getComponent(*this, t.component, componentType, sizeof(ComponentInfo)));
                if (component) {
                    component->constructFn(((std::byte*)chunk->components[i]) + component->size * row, 1);
                }
                if (t == ComponentTrait<EntityId>::id) {
                    *(static_cast<EntityId*>(chunk->components[i]) + row) = entity;
                }
            }
        }

        chunk->ids[row] = entityId;

        if (!name.empty()) {
            get<Identifier>(entity, identifierType)->name = name;
        }
        return entity;
    }

    static void chunkDeleteEntry(Registry& registry, Chunk& chunk, size_t row ) {
        assert( row < chunk.size );

        // Move last entity component to row, destruct last entity components
        for( size_t i = 0; i < chunk.type.components.size(); i++ )
        {
            ComponentType t = chunk.type.components[i];
            auto *component = static_cast<ComponentInfo*>( getComponent(registry, t.component, componentType, sizeof(ComponentInfo)));
            if ( component ) {
                auto data = static_cast<std::byte*>(chunk.components[i]);
                component->moveAssignAndDestructFn( data + component->size * row, data + component->size * chunk.size, 1);
            }
        }

        id_t lastId = chunk.ids[chunk.size - 1];
        chunk.ids[row] = lastId;
        registry.records[lastId].row = row;

        chunk.size--;
    }

    void Registry::destroy(EntityId id) {
        auto record = records[id.id];

        assert(record.chunk->ids[id.id] = record.chunk->ids[record.row]);

        chunkDeleteEntry(*this, *record.chunk, record.row);

        freeIds.push_back(id.id);
        records[id.id].chunk = nullptr;
        records[id.id].row = 0;
        records[id.id].generation++;
    }

    static void bootstrap(Registry &registry) {

        // Add null entity
        registry.records.push_back(EntityRecord {
            .chunk = nullptr,
            .row = 0,
            .generation = 0,
        });

        // Add component component entity
        componentType.component = 1;
        componentType.relationObject = 0;

        auto archetype = registry.archetypes.emplace_back(Archetype{
                .type = {componentType},
        });

        size_t capacity = 1024;

        auto chunk = archetype.chunks.emplace_back(new Chunk{
                .type = {componentType},
                .capacity = capacity,
                .size = 2,
                .components {
                        malloc(capacity * sizeof(ComponentInfo))
                },
        });

        auto *component = new(chunk->components[0]) ComponentInfo;
        *component = componentInfo<ComponentInfo>();

        registry.records.push_back( EntityRecord{
                .chunk = chunk,
                .row = 0
        });

        ComponentTrait<ComponentInfo>::id.component = 1;
        ComponentTrait<ComponentInfo>::id.relationObject = 0;


        // Add entity component entity
        ComponentType entityComponentType;
        entityComponentType.component = 2;
        entityComponentType.relationObject = 0;

        component = new(&static_cast<ComponentInfo*>(chunk->components[0])[1]) ComponentInfo;
        *component = componentInfo<EntityId>();

        registry.records.push_back( EntityRecord {
            .chunk = chunk,
            .row = 1,
        });

        ComponentTrait<EntityId>::id.component = 2;
        ComponentTrait<EntityId>::id.relationObject = 0;
    }

    Registry::Registry() {
        bootstrap(*this);
        registerComponent<ChildOf>();
        identifierType = ComponentType(registerComponent<Identifier>());
    }

    EntityId Registry::registerComponent(const ComponentInfo &component, const std::string_view name, EntityId parent) {
        EntityId id;

        Type type {ComponentType(componentType)};
        if (!name.empty()) {
            type.components.push_back(ComponentTrait<Identifier>::id);
        }
        if (!parent.isNull()) {
            type.components.push_back(ComponentType(ComponentTrait<ChildOf>::id, parent));
        }
        std::sort(type.components.begin(), type.components.end());

        id = create( type );

        ComponentInfo* componentInfo = get<ComponentInfo>(id, componentType);
        *componentInfo = component;

        if (!name.empty()) {
            Identifier *i = get<Identifier>(id, ComponentTrait<Identifier>::id);
            i->name = name;
        }

        return id;
    }

    void *Registry::get(EntityId entity, ComponentType component, size_t size) {
        assert(entity.id != 0);
        assert(component.component != 0);

        if (records[entity.id].generation != entity.generation)
        {
            return nullptr;
        }

        return getComponent(*this, entity.id, component, size);
    }

    void Registry::add(EntityId entity, ComponentType component, const void* ptr) {

        auto& record = records[entity.id];

        Type newType = record.chunk->type;
        newType.components.push_back(component);
        std::sort(newType.components.begin(), newType.components.end());

        Archetype& archetype = getArchetype(*this, newType);

        auto newChunk = getFreeChunk(*this, archetype, newType);

        auto newRow = newChunk->size++;

        for(size_t i = 0; i < newChunk->components.size(); i++) {
            ComponentType t = newChunk->type.components[i];
            auto *info = static_cast<ComponentInfo *>(getComponent(*this, t.component, componentType,
                                                                            sizeof(ComponentInfo)));
            if (info) {
                auto to = ((std::byte *) newChunk->components[i]) + info->size * newRow;
                if (t == component) {
                    if (ptr != nullptr) {
                        info->copyUninitFn(to, ptr, 1);
                    } else {
                        info->destructFn(to, 1);
                    }
                } else {
                    auto from = ((std::byte *) record.chunk->components[i]) + info->size * record.row;
                    info->moveUninitFn( to, from, 1);
                }
            }
        }

        chunkDeleteEntry(*this, *record.chunk, record.row);

        record.chunk = newChunk;
        record.row = newRow;
    }

}