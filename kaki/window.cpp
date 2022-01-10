//
// Created by felix on 30-12-21.
//

#include "window.h"

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

VkSurfaceKHR kaki::Window::createSurface(VkInstance instance) const {
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, (GLFWwindow*)handle, nullptr, &surface);
    return surface;
}

bool kaki::Window::shouldClose() const{
    return glfwWindowShouldClose((GLFWwindow*)handle);
}

struct WindowWorldRef {
    flecs::world_t* world;
    flecs::entity_t e;
};

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    WindowWorldRef* e = static_cast<WindowWorldRef*>(glfwGetWindowUserPointer(window));

    flecs::entity entity(e->world, e->e);
    assert(entity.has<kaki::Window>());

    if (entity.has<kaki::Input>()) {
        kaki::Input *input = entity.get_mut<kaki::Input>();

        switch (action) {
            case GLFW_PRESS:
                input->keyPressedFrame[key] = input->frame;
                break;
            case GLFW_RELEASE:
                input->keyReleasedFrame[key] = input->frame;
                break;
            default:
                break;
        }
    }
}

static void createWindow(flecs::entity e, kaki::Window& window) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* handle = glfwCreateWindow(window.width, window.height, "window", nullptr, nullptr);

    if (!handle) {
        fprintf(stderr, "Failed to create glfw window.\n");
        exit(1);
    }

    window.handle = handle;
    glfwSetWindowUserPointer(handle, new WindowWorldRef{
        .world = e.world().c_ptr(),
        .e = e.id(),
    });

    glfwSetKeyCallback(handle, key_callback);
}

static void destroyWindow(flecs::entity entity, kaki::Window& window) {
    delete static_cast<WindowWorldRef*>(glfwGetWindowUserPointer((GLFWwindow*)window.handle));
    glfwDestroyWindow((GLFWwindow*)window.handle);
}

kaki::windowing::windowing(flecs::world &world) {
    world.module<windowing>();

    glfwSetErrorCallback([](int error, const char *description) {
        fprintf(stderr, "GLFW error: %s\n", description);
    });

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize glfw.\n");
        exit(1);
    }

    world.system<Input>().kind(flecs::OnLoad).each([](flecs::entity entity, Input& input) {
        input.frame++;
    });

    world.trigger<Window>().event(flecs::OnSet).each(createWindow);
    world.trigger<Window>().event(flecs::OnRemove).each(destroyWindow);

    world.atfini([](ecs_world_t* world, void*){
        glfwTerminate();
    }, nullptr);
}

void kaki::pollEvents() {
    glfwPollEvents();
}

double kaki::getTime() {
    return glfwGetTime();
}
