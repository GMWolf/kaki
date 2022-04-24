//
// Created by felix on 24/04/22.
//

#include <kaki/ecs.h>
#include <kaki/window.h>
#include <kaki/gfx.h>

int main() {

    kaki::ecs::Registry registry;

    kaki::registerWindowingModule(registry);
    kaki::registerGfxModule(registry);

    auto windowEntity = registry.create({}, "window");

    registry.add(windowEntity, kaki::Window{
        .width = 1280,
        .height = 720,
        .title = "Kaki",
    });

    kaki::initWindow( registry.get<kaki::Window>( windowEntity ));

    auto renderer = registry.create({}, "Renderer");

    kaki::initRenderEntity(registry, renderer);

    while( !registry.get<kaki::Window>( windowEntity )->shouldClose() )
    {
        kaki::render(registry, renderer);

        kaki::pollEvents();
    }

    return 0;
}