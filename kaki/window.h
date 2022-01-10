//
// Created by felix on 30-12-21.
//

#ifndef KAKI_WINDOW_H
#define KAKI_WINDOW_H

#include <vulkan/vulkan.h>
#include "flecs.h"

namespace kaki {

    struct Window {
        int width {};
        int height {};

        void* handle{};

        [[nodiscard]] bool shouldClose() const;
        VkSurfaceKHR createSurface(VkInstance instance) const;
    };

    struct Input {
        static const uint32_t keyCount = 349;
        uint64_t frame = 0;
        uint64_t keyPressedFrame[keyCount];
        uint64_t keyReleasedFrame[keyCount];

        [[nodiscard]] inline bool keyDown(uint32_t keyCode) const {
            return keyReleasedFrame[keyCode] < keyPressedFrame[keyCode];
        }

        [[nodiscard]] inline bool keyUp(uint32_t keyCode) const {
            return !keyDown(keyCode);
        }
    };

    void pollEvents();
    double getTime();

    struct windowing {
        explicit windowing(flecs::world& world);
    };

}
#endif //KAKI_WINDOW_H
