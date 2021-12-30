//
// Created by felix on 30-12-21.
//

#ifndef KAKI_WINDOW_H
#define KAKI_WINDOW_H

#include <vulkan/vulkan.h>
#include "flecs.h"

namespace kaki {

    struct Window {
        int width;
        int height;

        void* handle{};

        bool shouldClose() const;
        VkSurfaceKHR createSurface(VkInstance instance) const;
    };

    void pollEvents();
    double getTime();

    struct windowing {
        windowing(flecs::world& world);
    };

}
#endif //KAKI_WINDOW_H
