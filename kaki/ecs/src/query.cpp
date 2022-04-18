//
// Created by felix on 18/04/22.
//

#include <query.h>
#include <algorithm>

namespace kaki::ecs {
    bool queryMatch(const Query &query, const Type &type) {
        return std::ranges::all_of(query.components, [&type](id_t c) {
            return std::ranges::find(type.components, c) != type.components.end();
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