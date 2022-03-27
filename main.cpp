#include <iostream>

#include <flecs.h>

#include <kaki/gfx.h>
#include <kaki/window.h>
#include <kaki/asset.h>
#include <kaki/transform.h>
#include <kaki/core.h>
#include <kaki/physics.h>

struct Control{};
struct Rotate{};
struct Foo{};
struct Shoot{};

int main() {
    flecs::world world;

    world.import<kaki::core>();
    world.import<kaki::windowing>();

    auto window = world.entity("window").set<kaki::Window>(kaki::Window{
        .width = 1920,
        .height = 1080,
        .title = "Test kaki app",
    }).set<kaki::Input>({});

    world.import<kaki::gfx>();
    world.import<kaki::Physics>();

    kaki::initPhysics(world);

    auto package = kaki::loadPackage(world, "testpackage.json").add<Foo>();

    //auto scene = world.entity("scene").is_a(package.lookup("SciFiHelmet::Scene")).add<Foo>();
    //scene.lookup("Camera").add<Control>();
    //auto sfh = scene.lookup("SciFiHelmet").add<Rotate>();
    //sfh.get_mut<kaki::Transform>()->position = {2, 1, 0};
    //sfh.get_mut<kaki::Transform>()->scale = 0.25;

    //auto dhscene = world.entity("dhscene").is_a(package.lookup("DamagedHelmet::Scene")).add<Foo>();
    ////dhscene.lookup("damagedHelmet").add<Rotate>();
    //dhscene.lookup("damagedHelmet").get_mut<kaki::Transform>()->position = {-2, 1, 0};
    //dhscene.lookup("damagedHelmet").get_mut<kaki::Transform>()->scale = 0.25;

    //dhscene.lookup("damagedHelmet").set(kaki::RigidBody{
    //        .mass = 1,
    //        .extent = glm::vec3(1,1,1),
    //});

    //auto sponza = world.entity("sponza").is_a(package.lookup("Sponza::Sponza")).add<Foo>();
    //auto temple = world.entity("temple").is_a(package.lookup("SunTemple::Scene")).add<Foo>();

    world.entity("Camera").set(kaki::Transform{
        .position = glm::vec3(-5, 1.7, 0),
        .scale = 1,
        .orientation = glm::quatLookAt(glm::vec3(1, 0, 0), glm::vec3(0,1,0)),
    }).set(kaki::Camera {
        .fov = glm::radians(90.0f),
    }).add<Control>().add<Shoot>();

    world.entity("floor").is_a(package.lookup("objects::Scene::Plane")).set(kaki::RigidBody{
            .mass = 0.0f,
            .extent = glm::vec3(40, 1, 40),
    }).get_mut<kaki::Transform>()->position.z = -0.5f;

    auto cubePrefab = world.entity().add(flecs::Prefab).is_a(package.lookup("objects::Scene::Cube")).set(kaki::RigidBody{
            .mass = 3.0f,
            .extent = glm::vec3(0.5),
    });

    for(int n = 0; n < 3; n++ ) {

        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                for (int i = 0; i < 8; i++) {

                    world.entity().is_a(package.lookup("objects::Scene::Cube")).set(kaki::RigidBody{
                            .mass = 3.0f,
                            .extent = glm::vec3(0.5),
                    }).get_mut<kaki::Transform>()->position = glm::vec3(0.51 * x + 8 * n, 0.1 + i * 0.51, 0.51 * y);

                }
            }
        }
    }

    //world.entity().is_a(cubePrefab).set<kaki::Transform>(kaki::Transform{
    //    .position = glm::vec3(0, 5, 0),
    //    .scale = 1,
    //    .orientation = glm::quat(glm::vec3(0,0,0)),
    //});


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
        if (input->keyDown('R')) {
            d += s * glm::vec3(0, 1, 0);
        }
        if (input->keyDown('F')) {
            d += s * glm::vec3(0, -1, 0);
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

    world.system<kaki::Transform>().term<Shoot>().kind(flecs::OnUpdate).each([&](flecs::entity e, kaki::Transform& t) {
        auto* input = window.get<kaki::Input>();

        if (input->keyPressed(kaki::KEYS::SPACE)) {
            auto b = e.world().entity().is_a(package.lookup("objects::Scene::Ball")).set(kaki::RigidBody{
                    .mass = 3.0f,
                    .sphere = true,
                    .extent = glm::vec3(0.3f / 2.0f),
            }).set(kaki::Transform{
                .position = t.position + glm::vec3(0, -0.5, 0),
                .scale = 1,
                .orientation = t.orientation,
            });

            e.world().entity().set(kaki::Impulse{
                .target = b,
                .impulse = t.orientation * glm::vec3(0,0.1,1) * 50.0f,
            });
        }

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
