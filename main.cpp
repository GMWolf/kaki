#include <iostream>

#include <flecs.h>

#include <kaki/gfx.h>
#include <kaki/window.h>
#include <kaki/asset.h>
#include <kaki/transform.h>
#include "kaki/core.h"

struct Control{};

int main() {

    flecs::world world;

    world.import<kaki::core>();

    world.import<kaki::windowing>();
    auto window = world.entity("window").set<kaki::Window>(kaki::Window{
        .width = 1280,
        .height = 720,
        .title = "Test kaki app",
    }).set<kaki::Input>({});
    world.import<kaki::gfx>();

    auto package = kaki::loadPackage(world, "testpackage.json");


    auto camera = world.entity("Camera").is_a(package.lookup("Camera"));
    world.entity().is_a(package.lookup("SciFiHelmet"))
        .add<Control>();



    world.system<kaki::Transform>().term<Control>().each([&](flecs::entity entity, kaki::Transform& transform) {
        auto* input = window.get<kaki::Input>();

        if (input->keyDown('D')) {
            transform.position.x -= entity.delta_time() * 10;
        }
        if (input->keyDown('A')) {
            transform.position.x += entity.delta_time() * 10;
        }
        if (input->keyDown('W')) {
            transform.position.z += entity.delta_time() * 10;
        }
        if (input->keyDown('S')) {
            transform.position.z -= entity.delta_time() * 10;
        }
    });

    world.set(flecs::rest::Rest{});

    double lastFrameTime = kaki::getTime();

    while(!window.get<kaki::Window>()->shouldClose()) {
        double currentFrameTime = kaki::getTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        world.progress(static_cast<float>(deltaTime));

        kaki::pollEvents();
    }

    return 0;
}
