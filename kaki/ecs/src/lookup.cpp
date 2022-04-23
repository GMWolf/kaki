//
// Created by felix on 23/04/22.
//

#include "registry.h"
#include "query.h"
#include <cstring>
#include <identifier.h>
#include <childof.h>
#include <view.h>

namespace kaki::ecs {

    EntityId Registry::lookup(const char* name, EntityId root) {

        char* str = strdupa(name);
        char* token = strtok( str, "::" );

        EntityId entity = root;

        while(token != nullptr) {
            kaki::ecs::Query q;
            if (!entity.isNull()) {
                q = kaki::ecs::Query().all<Identifier>().all(ComponentType(ComponentTrait<ChildOf>::id, entity));
            } else {
                q = kaki::ecs::Query().all<Identifier>().none<ChildOf>();
            }

            entity.id = 0;
            entity.generation = 0;
            for(auto chunk : query(*this, q)) {
                for (const auto& [id, iden] : ChunkView<EntityId, Identifier>( chunk )) {
                    if (iden.name == token) {
                        entity = id;
                        break;
                    }
                }
                if (!entity.isNull()) break;
            }

            if (entity.isNull()) {
                break;
            }

            token = strtok(nullptr, "::");
        }


        return entity;
    }

}