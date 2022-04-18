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

    fprintf(stdout, "%lu\n", idA);

    auto b = registry.create({idA, idInt});
    registry.get<A>(b, idA).a = 42;

    b = registry.create({idA, idInt, tag});
    registry.get<A>(b, idA).a = 43;

    for(auto chunk : kaki::ecs::query(registry, kaki::ecs::Query {
        .components = {idA, idInt, tag},
    })) {
        for(auto[a] : kaki::ecs::ChunkView<A>(chunk, idA))
        {
            printf("%d\n", a.a);
        }
    };

    fprintf(stdout, "\n");

    for(auto chunk : kaki::ecs::query(registry, kaki::ecs::Query {
            .components = {0, kaki::ecs::identifierId},
    })) {
        for(auto[c, iden] : kaki::ecs::ChunkView<kaki::ecs::ComponentInfo, kaki::ecs::Identifier>(chunk, 0, kaki::ecs::identifierId))
        {
            printf("%s, %zu\n", iden.name.c_str(), c.size);
        }
    };

    fprintf(stdout, "\n");

    return 0;
}