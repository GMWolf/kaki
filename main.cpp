#include <iostream>

#include <flecs.h>

#include <kaki/gfx.h>
#include <kaki/window.h>
#include <kaki/asset.h>
#include <kaki/transform.h>

struct Control{};

int main() {

    flecs::world world;

    world.import<kaki::windowing>();

    auto window = world.entity("window").set<kaki::Window>(kaki::Window{
        .width = 1280,
        .height = 720,
        .title = "Test kaki app",
    }).set<kaki::Input>({});
    world.import<kaki::gfx>();

    auto package = kaki::loadPackage(world, "testpackage.json");

    auto camera = world.entity("camera");
    camera.set(kaki::Camera{
        .fov = glm::radians(75.0f),
    }).set(kaki::Transform {
        .position = {0,0,5},
        .scale = 1,
        .orientation = glm::quatLookAt(glm::vec3{0,0,-1}, glm::vec3{0,1,0}),
    });

    world.entity().set<kaki::MeshFilter>(kaki::MeshFilter{
        .mesh = package.lookup("SciFiHelmet_gltf::SciFiHelmet"),
        .albedo = package.lookup("SciFiHelmet_gltf::SciFiHelmet_BaseColor"),
        .normal = package.lookup("SciFiHelmet_gltf::SciFiHelmet_Normal"),
        .metallicRoughness = package.lookup("SciFiHelmet_gltf::SciFiHelmet_MetallicRoughness"),
        .ao = package.lookup("SciFiHelmet_gltf::SciFiHelmet_AmbientOcclusion"),
    }).set(kaki::Transform{
        .position = {0,0,0},
        .scale = 1,
        .orientation = {},
    }).add<Control>();

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
