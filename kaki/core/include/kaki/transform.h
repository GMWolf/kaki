//
// Created by felix on 16/01/2022.
//

#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kaki {

    struct Transform {
        glm::vec3 position {};
        float scale {1};
        glm::quat orientation {};

        [[nodiscard]] Transform inverse() const;
        [[nodiscard]] glm::mat4 matrix() const;
    };

}