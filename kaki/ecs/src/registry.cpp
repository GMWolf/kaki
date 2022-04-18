//
// Created by felix on 18/04/22.
//
#include <registry.h>
#include <identifier.h>

namespace kaki::ecs {

    id_t identifierId;
    id_t componentId;

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

    static int typeGetComponentIndex(const Type &type, id_t component) {
        for (int index = 0; index < type.components.size(); index++) {
            if (type.components[index] == component) {
                return index;
            }
        }
        return -1;
    }

    static void *getComponent(Registry &registry, id_t entity, id_t component, size_t componentSize) {
        id_t entityIndex = idEntity(entity);
        auto record = registry.records[entityIndex];

        auto index = typeGetComponentIndex(record.chunk->type, component);
        if (index < 0) {
            return nullptr;
        }

        return static_cast<char *>(record.chunk->components[index]) + (record.row * componentSize);
    }

    static Chunk *createChunk(Registry &registry, const Type &type, size_t capacity) {
        auto *chunk = new Chunk{
                .type = type,
                .capacity = capacity,
                .size = 0,
                .components = std::vector<void *>(type.components.size()),
        };

        for (int i = 0; i < type.components.size(); i++) {
            auto *component = static_cast<ComponentInfo *>(getComponent(registry, idComponent(type.components[i]), componentId,
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

    id_t Registry::create(const Type &inType,  std::string_view name) {
        Type type = inType;
        if(!name.empty()) {
            type.components.push_back(identifierId);
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

        // construct components
        {
            for( uint i = 0; i < chunk->type.components.size(); i++ )
            {
                id_t t = chunk->type.components[i];
                auto *component = static_cast<ComponentInfo *>(getComponent(*this, idComponent(t), componentId, sizeof(ComponentInfo)));
                if (component) {
                    component->constructFn(((std::byte*)chunk->components[i]) + component->size * row, 1);
                }
            }
        }

        id_t entity = entityId | (generation << 32ull);

        if (!name.empty()) {
            get<Identifier>(entity, identifierId)->name = name;
        }
        return entity;
    }

    void Registry::destroy(id_t id) {

        auto record = records[id];

        // destruct components
        {
            for( id_t t : record.chunk->type.components )
            {
                auto *component = static_cast<ComponentInfo *>(getComponent(*this, idComponent(t), componentId, sizeof(ComponentInfo)));
                if ( component ) {
                    component->destructFn(&record.chunk->components[record.row], 1);
                }
            }
        }

        freeIds.push_back(idEntity(id));
        records[idEntity(id)].chunk = nullptr;
        records[idEntity(id)].row = 0;
        records[idEntity(id)].generation++;
    }

    static void bootstrap(Registry &registry) {
        componentId = 0ull | (1ull << 32ull);
        Type componentType{componentId};

        auto archetype = registry.archetypes.emplace_back(Archetype{
                .type = componentType,
        });

        size_t capacity = 1024;

        auto chunk = archetype.chunks.emplace_back(new Chunk{
                .type = componentType,
                .capacity = capacity,
                .size = 1,
                .components {
                        malloc(capacity * sizeof(ComponentInfo))
                },
        });

        auto *component = new(chunk->components[0]) ComponentInfo;
        *component = componentInfo<ComponentInfo>();

        registry.records.push_back(EntityRecord{
                .chunk = chunk,
                .row = 0
        });
    }

    Registry::Registry() {
        bootstrap(*this);
        identifierId = registerComponent<Identifier>();
    }

    id_t Registry::registerComponent(const ComponentInfo &component, const std::string_view name) {
        id_t id = 0;
        if (name.empty()) {
            id = create({componentId});
        } else {
            id = create({componentId, identifierId});
        }

        ComponentInfo* componentInfo = get<ComponentInfo>(id, componentId);
        *componentInfo = component;

        if (!name.empty()) {
            Identifier *i = get<Identifier>(id, identifierId);
            i->name = name;
        }

        return id;
    }

    void *Registry::get(id_t entity, id_t component, size_t size) {
        assert(entity != 0);
        assert(component != 0);

        if (records[idEntity(entity)].generation != idGeneration(entity))
        {
            return nullptr;
        }

        return getComponent(*this, entity, component, size);
    }

}