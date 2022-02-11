#include <iostream>

#include <flecs.h>

#include <kaki/gfx.h>
#include <kaki/window.h>
#include <kaki/asset.h>
#include <kaki/transform.h>

struct Control{};

int main() {

    flecs::world world;

    world.set(flecs::rest::Rest{});

    world.import<kaki::windowing>();
    //auto window = world.entity("window").set<kaki::Window>(kaki::Window{
    //    .width = 640,
    //    .height = 480,
    //    .title = "Test kaki app",
    //}).set<kaki::Input>({});
    world.plecs_from_file("../game.plecs");
    auto window = world.lookup("window");
    world.import<kaki::gfx>();

    auto mainAssets = kaki::loadAssets(world, "assets.json");

    // Print out assets in main assets
    world.filter_builder()
        .term<kaki::Asset>()
        .term(flecs::ChildOf, mainAssets)
        .build()
        .each([](flecs::entity e) {
            std::cout << "-" << e.path() << std::endl;
            e.world().filter_builder()
                .term(flecs::ChildOf, e)
                .build().each([](flecs::entity e){
                   std::cout << "~" << e.path() << std::endl;
                });
    });

    auto camera = world.entity("camera");
    camera.set(kaki::Camera{
        .fov = glm::radians(90.0f),
    }).set(kaki::Transform {
        .position = {0,0,-5},
        .scale = 1,
        .orientation = glm::quatLookAt(glm::vec3{0,0,1}, glm::vec3{0,1,0}),
    });

    world.entity().set<kaki::MeshFilter>(kaki::MeshFilter{
        .mesh = mainAssets.lookup("SciFiHelmet::SciFiHelmet"),
        .image = mainAssets.lookup("SciFiHelmet_BaseColor"),
    }).set(kaki::Transform{
        .position = {0,0,0},
        .scale = 1,
        .orientation = {},
    }).add<Control>();

    world.entity().set<kaki::MeshFilter>(kaki::MeshFilter{
            .mesh = mainAssets.lookup("untitled::Cube"),
            .image = mainAssets.lookup("kaki"),
    }).set(kaki::Transform{
            .position = {0,-1,0},
            .scale = 1,
            .orientation = {},
    });

    world.entity().set<kaki::MeshFilter>(kaki::MeshFilter{
            .mesh = mainAssets.lookup("untitled::Cube1"),
            .image = mainAssets.lookup("kaki"),
    }).set(kaki::Transform{
            .position = {2,-0.2,2},
            .scale = 1.2,
            .orientation = {},
    });


    world.system<kaki::Transform>().term<Control>().each([](flecs::entity entity, kaki::Transform& transform) {
        auto* input = entity.world().lookup("window").get<kaki::Input>();

        if (input->keyDown('D')) {
            transform.position.x += entity.delta_time() * 10;
        }
        if (input->keyDown('A')) {
            transform.position.x -= entity.delta_time() * 10;
        }
        if (input->keyDown('W')) {
            transform.position.z += entity.delta_time() * 10;
        }
        if (input->keyDown('S')) {
            transform.position.z -= entity.delta_time() * 10;
        }

    });

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
