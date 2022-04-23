//
// Created by felix on 18/04/22.
//

#pragma once

#include "type.h"
#include "registry.h"
#include <vector>

namespace kaki::ecs {

    struct Query {
        std::vector<ComponentType> components;
        std::vector<ComponentType> exclude;

        template<class...T>
        Query& all();

        Query& all(ComponentType id);

        template<class...T>
        Query& none();
    };

    template<class...T>
    Query& Query::all() {
        components.insert(components.end(), {ComponentType(ComponentTrait<T>::id)...});
        return *this;
    }

    template<class...T>
    Query& Query::none() {
        exclude.insert(exclude.end(), {ComponentType(ComponentTrait<T>::id)...});
        return *this;
    }

    inline Query &Query::all(ComponentType id) {
        components.push_back(id);
        return *this;
    }

    bool queryMatch(const Query& query, const Type& type);

    struct QueryIterator {
        Query query;
        std::vector<Archetype>::iterator archetypeIterator;
        std::vector<Archetype>::iterator archetypeEnd;
        std::vector<Chunk*>::iterator chunkIterator;

        inline bool operator==(const QueryIterator& other) const {
            return archetypeIterator == other.archetypeIterator
                   && chunkIterator == other.chunkIterator;
        }

        inline bool operator!=(const QueryIterator& other) const {
            return !this->operator==(other);
        }

        inline QueryIterator& operator++() {
            chunkIterator++;
            if (chunkIterator == archetypeIterator->chunks.end()) {
                do {
                    archetypeIterator++;
                } while(archetypeIterator != archetypeEnd && !queryMatch(query, archetypeIterator->type));
                chunkIterator = archetypeIterator == archetypeEnd ? typeof(chunkIterator){} : archetypeIterator->chunks.begin();
            }
            return *this;
        }

        inline Chunk& operator*() const {
            return **chunkIterator;
        }

    };

    struct QueryResult {
        QueryIterator beginIterator;
        QueryIterator endIterator;

        QueryIterator begin() {
            return beginIterator;
        }

        QueryIterator end() {
            return endIterator;
        }
    };

    QueryResult query(Registry& registry, const Query& query);
}