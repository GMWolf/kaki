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

static void createWindow(flecs::iter it, kaki::Window* window) {
    assert(it.count() == 1);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* handle = glfwCreateWindow(window->width, window->height, "window", nullptr, nullptr);



    if (!window) {
        fprintf(stderr, "Failed to create glfw window.\n");
        exit(1);
    }

    window[0].handle = handle;
}

static void destroyWindow(flecs::iter it, kaki::Window* window) {
    assert(it.count() == 1);

    glfwDestroyWindow((GLFWwindow*)window->handle);
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

    world.trigger<Window>().event(flecs::OnSet).iter(createWindow);
    world.trigger<Window>().event(flecs::OnRemove).iter(destroyWindow);

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
