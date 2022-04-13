//
// Created by felix on 13/04/2022.
//

#pragma once

#include <functional>

namespace kaki {

    struct Job {
        std::function<void(void)> function;
    };


}