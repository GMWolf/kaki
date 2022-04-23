//
// Created by felix on 18/04/22.
//

#include <kaki/ecs.h>
#include "kaki/ecs/childof.h"

struct A {
    int a;
};

int main() {

    kaki::ecs::Registry registry;

    kaki::ecs::EntityId idA = registry.registerComponent<A>("A");
    kaki::ecs::EntityId idInt = registry.registerComponent<int>("int");
    auto childOf = kaki::ecs::ComponentTrait<kaki::ecs::ChildOf>::id;

    auto b = registry.create({kaki::ecs::ComponentType(idA)}, "B");
    registry.get<A>(b)->a = 42;

    auto c = registry.create({kaki::ecs::ComponentType(idA), kaki::ecs::ComponentType(childOf, b) }, "C");
    registry.get<A>(c)->a = 43;

    printf("TEST 1:\n");
    for(auto chunk : kaki::ecs::query(registry, kaki::ecs::Query {
            .components = {kaki::ecs::ComponentType(idA), kaki::ecs::ComponentType(childOf, b)},
    })) {
        for(auto[a] : kaki::ecs::ChunkView<A>(chunk, kaki::ecs::ComponentType(idA)))
        {
            printf("%d\n", a.a);
        }
    };

    printf("%u\n", registry.lookup("C").id);
    printf("%u\n", registry.lookup("B::C").id);

    return 0;
}