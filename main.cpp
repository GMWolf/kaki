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
    world.set<kaki::Window>(kaki::Window{
        .width = 640,
        .height = 480,
    });
    world.set<kaki::Input>({});
    world.import<kaki::gfx>();

    world.entity("vertex_shader").set<kaki::Asset>({"shader.vert.shd"}).add<kaki::asset::Shader>();
    world.entity("fragment_shader").set<kaki::Asset>({"shader.frag.shd"}).add<kaki::asset::Shader>();

    world.entity("pipeline").set<kaki::Asset>({"pipeline.json"}).add<kaki::asset::Pipeline>();


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
        auto* input = entity.world().get<kaki::Input>();

        if (input->keyDown('d')) {
            rect.pos.x += entity.delta_time() * 10;
        }

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
