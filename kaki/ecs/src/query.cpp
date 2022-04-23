//
// Created by felix on 18/04/22.
//

#include <query.h>
#include <algorithm>

namespace kaki::ecs {

    static bool componentMatch(ComponentType a, ComponentType b) {
        return (a.component == b.component)
        && ((a.relationObject == b.relationObject) || (a.relationObject == 0) || (b.relationObject == 0));
    }

    bool queryMatch(const Query &query, const Type &type) {
        return std::ranges::all_of(query.components, [&type](ComponentType c) {
            return std::ranges::find_if(type.components, [c](ComponentType a){ return componentMatch(a, c); } ) != type.components.end();
        })
        &&
        std::ranges::none_of(query.exclude, [&type](ComponentType c) {
            return std::ranges::find_if(type.components, [c](ComponentType a){ return componentMatch(a, c); } ) != type.components.end();
        });
    }

    QueryResult query(Registry &registry, const Query &query) {
        QueryIterator begin;
        begin.query = query;
        begin.archetypeIterator = registry.archetypes.begin();
        begin.archetypeEnd = registry.archetypes.end();

        while(begin.archetypeIterator != begin.archetypeEnd && !queryMatch(query, begin.archetypeIterator->type)) {
            begin.archetypeIterator++;
        }

        if (begin.archetypeIterator == begin.archetypeEnd) {
            begin.chunkIterator = {};
        } else {
            begin.chunkIterator = begin.archetypeIterator->chunks.begin();
        }

        QueryIterator end;
        end.query = query;
        end.archetypeIterator = registry.archetypes.end();
        end.archetypeEnd = registry.archetypes.end();
        end.chunkIterator = {};

        return {
                .beginIterator = begin,
                .endIterator = end
        };

    }

}