//
// Created by felix on 18/04/22.
//

#include <kaki/ecs.h>

struct A {
    int a;
};

int main() {

    kaki::ecs::Registry registry;

    auto idA = registry.registerComponent<A>("A");
    auto idInt = registry.registerComponent<int>("int");
    auto tag = registry.create({});

    auto b = registry.create({idA, idInt});
    registry.get<A>(b)->a = 42;

    auto c = registry.create({idA, idInt, kaki::ecs::relation(tag, b)});
    registry.get<A>(c)->a = 43;

    for(auto chunk : kaki::ecs::query(registry, kaki::ecs::Query {
        .components = {idA, idInt, kaki::ecs::relation(tag, b)},
    })) {
        for(auto[a] : kaki::ecs::ChunkView<A>(chunk, idA))
        {
            printf("%d\n", a.a);
        }
    };

    fprintf(stdout, "\n");

    for(auto chunk : kaki::ecs::query(registry, kaki::ecs::Query {
            .components = {kaki::ecs::identifierId},
    })) {
        for(auto[iden] : kaki::ecs::ChunkView<kaki::ecs::Identifier>(chunk))
        {
            printf("%s\n", iden.name.c_str());
        }
    };

    fprintf(stdout, "\n");

    return 0;
}