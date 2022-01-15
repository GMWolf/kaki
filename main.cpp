#include <flecs.h>

#include "kaki/gfx.h"
#include "kaki/window.h"
#include "kaki/asset.h"
#include "kaki/shader.h"
#include "kaki/pipeline.h"

int main() {

    flecs::world world;

    world.set(flecs::rest::Rest{});

    world.import<kaki::windowing>();
    auto window = world.entity("window").set<kaki::Window>(kaki::Window{
        .width = 640,
        .height = 480,
    }).set<kaki::Input>({});
    world.import<kaki::gfx>();

    //world.entity("vertex_shader").set(kaki::Asset{"shader.vert.shd"});
    //ecs_plecs_from_file(world.c_ptr(), "../asset.plecs");

    kaki::loadAssets(world, "assets.json");

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

    world.system<kaki::Rectangle>().each([](flecs::entity entity, kaki::Rectangle& rect) {
        auto* input = entity.world().lookup("window").get<kaki::Input>();

        if (input->keyDown('D')) {
            rect.pos.x += entity.delta_time() * 10;
        }
        if (input->keyDown('A')) {
            rect.pos.x -= entity.delta_time() * 10;
        }
        if (input->keyDown('S')) {
            rect.pos.y += entity.delta_time() * 10;
        }
        if (input->keyDown('W')) {
            rect.pos.y -= entity.delta_time() * 10;
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
