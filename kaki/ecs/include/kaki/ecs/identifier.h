//
// Created by felix on 18/04/22.
//

#pragma once

#include "registry.h"
#include <string>

namespace kaki::ecs {

    struct Identifier {
        std::string name;
    };

    extern id_t identifierId;

}