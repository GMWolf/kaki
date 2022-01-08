#include <flecs.h>

#include "kaki/gfx.h"
#include "kaki/window.h"

int main() {

    flecs::world world;

    world.set(flecs::rest::Rest{});

    world.import<kaki::windowing>();
    world.set<kaki::Window>(kaki::Window{
        .width = 640,
        .height = 480,
    });
    world.import<kaki::gfx>();

    auto camera = world.entity("camera");
    camera.set<kaki::Camera>(kaki::Camera{
        .x = 0,
        .y = 0,
        .width = 64,
        .height = 48,
    });

    world.entity().set<kaki::Rectangle>(kaki::Rectangle{
        .pos = {2, 2},
        .color = {1, 1, 1},
    });

    world.entity().set<kaki::Rectangle>(kaki::Rectangle{
            .pos = {10, 4},
            .color = {1, 1, 1},
    });

    auto window = world.get<kaki::Window>();
    double lastFrameTime = kaki::getTime();

    while(!window->shouldClose()) {
        double currentFrameTime = kaki::getTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        world.progress(static_cast<float>(deltaTime));

        kaki::pollEvents();
    }

    return 0;
}
