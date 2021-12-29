#include <cstdio>

#include <flecs.h>
#include <GLFW/glfw3.h>



static void glfwErrorCallback(int error, const char *description) {
    fprintf(stderr, "GLFW error: %s\n", description);
}

int main() {

    flecs::world world;

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize glfw.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, "window", nullptr, nullptr);

    if (!window) {
        fprintf(stderr, "Failed to create glfw window.\n");
        return 1;
    }

    double lastFrameTime = glfwGetTime();

    while(!glfwWindowShouldClose(window)) {
        double currentFrameTime = glfwGetTime();
        double deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;

        world.progress(static_cast<float>(deltaTime));

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
