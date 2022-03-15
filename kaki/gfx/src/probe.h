//
// Created by felix on 13/03/2022.
//

#pragma once

#include <glm/glm.hpp>

namespace kaki {

    struct Probe {
        glm::vec3 centre;

        struct {
            glm::vec3 min;
            glm::vec3 max;
        } bbox;

    };

}