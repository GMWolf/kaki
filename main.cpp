#include <iostream>

#include <flecs.h>

#include <kaki/gfx.h>
#include <kaki/window.h>
#include <kaki/asset.h>
#include <kaki/transform.h>
#include "kaki/core.h"

struct Control{};
struct Rotate{};
struct Foo{};
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

    auto package = kaki::loadPackage(world, "testpackage.json").add<Foo>();
    auto scene = world.entity("scene").is_a(package.lookup("SciFiHelmet::Scene")).add<Foo>();
    scene.lookup("Camera").add<Control>();
    scene.lookup("SciFiHelmet").add<Rotate>();

    auto dhscene = world.entity("dhscene").is_a(package.lookup("DamagedHelmet::Scene")).add<Foo>();
    dhscene.lookup("damagedHelmet").add<Rotate>();

    auto sponza = world.entity("sponza").is_a(package.lookup("Sponza::Sponza")).add<Foo>();

    world.system<kaki::Transform>("Control system").term<Control>().each([&](flecs::entity entity, kaki::Transform& transform) {
        auto* input = window.get<kaki::Input>();

        glm::vec3 d(0);
        float r = 0;
        float s = 2 * entity.delta_time();
        if (input->keyDown('D')) {
            d += s * glm::vec3(1, 0, 0);
        }
        if (input->keyDown('A')) {
            d += s * glm::vec3(-1, 0, 0);
        }
        if (input->keyDown('W')) {
            d += s * glm::vec3(0, 0, 1);
        }
        if (input->keyDown('S')) {
            d += s * glm::vec3(0, 0, -1);
        }
        if (input->keyDown('E')) {
            r += 1 * entity.delta_time();
        }
        if (input->keyDown('Q')) {
            r -=1 * entity.delta_time();
        }

        transform.orientation *= glm::quat(glm::vec3(0, r, 0));

        transform.position += transform.orientation * d;

    });

    world.system<kaki::Transform>("Rotate system").term<Rotate>().each([&](flecs::entity entity, kaki::Transform& transform) {
        transform.orientation = glm::quat(glm::vec3(0, entity.delta_time() * 0.5, 0)) * transform.orientation;
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
