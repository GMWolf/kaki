#include <flecs.h>

#include "kaki/gfx.h"
#include "kaki/window.h"

int main() {

    flecs::world world;

    world.import<kaki::windowing>();
    world.set<kaki::Window>(kaki::Window{
        .width = 640,
        .height = 480,
    });
    world.import<kaki::gfx>();

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
