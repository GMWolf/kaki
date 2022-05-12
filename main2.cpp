//
// Created by felix on 24/04/22.
//

#include <kaki/ecs.h>
#include <kaki/window.h>
#include <kaki/gfx.h>
#include <kaki/asset.h>
#include <kaki/package.h>
#include <kaki/core.h>

int main() {

    kaki::ecs::Registry registry;

    registry.create({},"kaki");

    kaki::registerWindowingModule(registry);
    kaki::registerGfxModule(registry);
    kaki::registerCoreModule(registry);

    auto windowEntity = registry.create({}, "window");

    registry.add(windowEntity, kaki::Window{
        .width = 1280,
        .height = 720,
        .title = "Kaki",
    });

    kaki::initWindow( registry.get<kaki::Window>( windowEntity ));

    auto renderer = registry.create({}, "Renderer");

    kaki::initRenderEntity(registry, renderer);

    kaki::loadPackage(registry, "testpackage.json");

    while( !registry.get<kaki::Window>( windowEntity )->shouldClose() )
    {
        kaki::render(registry, renderer);

        kaki::pollEvents();
    }

    return 0;
}